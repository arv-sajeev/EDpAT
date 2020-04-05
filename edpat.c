/*
 * 
 *
SPDX-License-Identifier: BSD-3-Clause-Clear
https://spdx.org/licenses/BSD-3-Clause-Clear.html#licenseText

Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted (subject to the limitations in the disclaimer below) provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of Arvind Sajeev nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "edpat.h"
#include "scripts.h"
#include "testcase.h"
#include "variable.h"
#include "packet.h"
#include "print.h"
#include "EthPortIO.h"

char CurrentTestCaseId[MAX_TESTCASE_ID_LEN+1];
EDPAT_TEST_RESULT CurrentTestResult	= EDPAT_TEST_RESULT_UNKNOWN;
int PacketReceiveTimeout = PKT_RECEIVE_TIMEOUT;
EDPAT_BOOL EnableBroadcastPacketFiltering =  EDPAT_TRUE;
EDPAT_BOOL SyntaxCheckOnly = EDPAT_FALSE;
EDPAT_BOOL PromiscuousModeEnabled = EDPAT_FALSE;

unsigned char PktBuf[MAX_PKT_SIZE];


/**********************
 *
 *
 *   TestScriptProcess()
 *
 *   Read test script line-by-line and process.
 *
 *   Arguments 		:	 fileName - INPUT. Name of the test script file to be processsed.
 *					
 *   Return:		-	 EDPAT_SUCCESS or EDPAT_FAILED
 *
 *
 *
 **********************/

EDPAT_RETVAL TestScriptProcess(const char *fileName)
{
	EDPAT_RETVAL retVal;
	FILE *fp;
	int lineLen;
	int statementLen;
	static char testScriptStatement[MAX_SCRIPT_LINE_LEN];
	
	// Open the file handle for the script file and add details to the scriptinfo table 
	fp = ScriptOpen(fileName);
	if (NULL == fp)
	{
		return EDPAT_FAILED;
	};

	retVal = 1;
	// Keep reading statements from the opened filepointer 
	while ( 0 < (statementLen = ScriptReadStatement(fp,testScriptStatement)))
	{
		//Substitute each occurence of a variable with its coressponding value
		retVal = ScriptSubstituteVariables(testScriptStatement);
		if (0 > retVal)
		{
			CurrentTestResult = EDPAT_TEST_RESULT_SKIPPED;
			continue;
		}

		// If test case fails or is faulty skip until the next testcase ID 
		if (	( '@' != testScriptStatement[0] ) && (( EDPAT_TEST_RESULT_SKIP ==  CurrentTestResult) || ( EDPAT_TEST_RESULT_FAILED ==  CurrentTestResult)))
		{
			// if test case is not passed. Skip to next teste spec
			continue;
		}

		switch(testScriptStatement[0])
		{
			case '#':	// include file
				retVal = ScriptIncludeFile(testScriptStatement);
				break;
			case '@':	// test case ID
				retVal = GetTestCaseID(testScriptStatement);
				break;
			case '<':	// Receive packet
			case '>':	// Send packet

				if ( EDPAT_TEST_RESULT_UNKNOWN ==  CurrentTestResult)
				{	
					//Set to passed when you see the first send case or receive case
					CleanupLastTestExecution();
					CurrentTestResult = EDPAT_TEST_RESULT_PASSED;
				}
				if (EDPAT_TRUE != SyntaxCheckOnly)
				{
					retVal = PacketProcess(testScriptStatement);
				}
				else
				{
					retVal = EDPAT_SUCCESS;
				}
				break;
			case '$':	// assign value to variable
				retVal = VariableStoreValue(testScriptStatement);
				break;
			default:
				ScriptErrorMsgPrint("Unknown statement '%c(%d)'",
					testScriptStatement[0],
					testScriptStatement[0]);
				retVal = EDPAT_FAILED;
				
				break;

		}
		if ( EDPAT_SUCCESS != retVal)
		{
			CurrentTestResult = EDPAT_TEST_RESULT_SKIPPED;
		}
	};
	ScriptClose(fp);
	return retVal;
}

/**********************
 *
 *	main()
 *	Processes the command line parameters and sets the required flags
 *	Handles all setting up and cleaning up functions
 *
 **********************/


int main(int argc, char *argv[])
{
        char *scriptFileName;
        char *fileName;
	EDPAT_RETVAL retVal;
	int c;

	FILE *logFileFp = stdout;
	FILE *reportFileFp = stdout;
	FILE *errorFileFp = stderr;

	
	// Print license and other info
	printf("Ethernet Data Plan Application Tester(EDPaT).\n");
	printf("Build %s %s.\n", __DATE__,__TIME__);
	printf(LICENSE_PROMPT);

	//Extract the different flags and commandline parameters
	while ((c = getopt (argc, argv, "fhpstvw:")) != -1)
	{
		switch (c)
		{
			case 'f':
				EnableBroadcastPacketFiltering = EDPAT_FALSE;
				break;
			case 'h':
				PrintUsageInfo(argv[0]);
				exit(0);
			case 'p':
				PromiscuousModeEnabled = EDPAT_TRUE;
				break;
			case 's':
				SyntaxCheckOnly = EDPAT_TRUE;
				break;
			case 't':
				MsgTimestampEnable();
				break;
			case 'v':
				VerboseMsgEnable();
				break;
			case 'w':
				PacketReceiveTimeout = atoi(optarg);
				if (0 >= PacketReceiveTimeout)
				{
					printf("\nERROR: Invalid timeout value "
							"for '-w' option");
					PrintUsageInfo(argv[0]);
					exit(EXIT_FAILURE);
				}
				break;
			case ':':
				printf("\nERROR: '-w' option need a value");
				PrintUsageInfo(argv[0]);
				exit(EXIT_FAILURE);
			case '?':
				printf("\nERROR: unknown option '%c'",optopt);
				PrintUsageInfo(argv[0]);
				exit(EXIT_FAILURE);
			default:
				if (isprint (optopt))
				{
				   printf("\nERROR: Unknown option `-%c'.\n",
						optopt);
				}
				else
				{
				   printf("\nERROR: Unknown option "
						"character `\\x%x'.\n",
						optopt);
				}
				PrintUsageInfo(argv[0]);
				exit(EXIT_FAILURE);
		}
	}
	
	// Check if mandatory args are missing
	if (optind >= argc)
	{
		printf("\nERROR: Mandatory parameter "
				"<input-script> is missing.\n");
		PrintUsageInfo(argv[0]);
		exit(EXIT_FAILURE);
	}
	scriptFileName = argv[optind];
	fileName = "stdout";


	if ((optind + 1) < argc)
	{
		// Open Log file 
		fileName = argv[optind+1];
		logFileFp = fopen(fileName,"a");
		errorFileFp = logFileFp; 
		if (NULL == logFileFp)
		{
			printf("\nERROR: Failed to open log file '%s'\n",
					fileName);
			exit(EXIT_FAILURE);
		}
	}
	printf("## Logs written to '%s'\n",fileName);

	// Open file descriptors
	fileName = "stdout";
	if ((optind + 2) < argc)
	{
		// Report Log file 
		fileName = argv[optind+2];
		reportFileFp = fopen(fileName,"a");
		if (NULL == reportFileFp)
		{
			printf("\nERROR: Failed to open report file '%s'\n",
					fileName);
			exit(EXIT_FAILURE);
		}
	}

	printf("## Report written to '%s'\n",fileName);

	// This prints the timestamp and also passees the filepointers to the respective print functions in print.c
	retVal = InitMsgPrint(logFileFp,reportFileFp,errorFileFp);
	if (EDPAT_SUCCESS != retVal)
	{
		exit(1);
	}

	TestScriptProcess(scriptFileName);

	/* Print result of last test specification */
	CleanupLastTestExecution();

	EthPortCloseAll();
	if( logFileFp != stdout)
	{
		fclose(logFileFp);
	}
	if( reportFileFp != stdout)
	{
		fclose(reportFileFp);
	}
        exit(0);
}


/* End of file */
