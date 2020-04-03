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


/***********************
 *   TestScriptProcess()
 *
 *   Read test script line-by-line and process.
 *
 *   Arguments : 
 *	fileName - INPUT. Name of the test script file to be processsed.
 *					
 *   Return:	- EDPAT_SUCESS or EDPAT_FAULED
 *
 ********/

EDPAT_RETVAL TestScriptProcess(const char *fileName)
{
	EDPAT_RETVAL retVal;
	FILE *fp;
	int lineLen;
	int statementLen;
	static char testScriptStatement[MAX_SCRIPT_LINE_LEN];

	fp = ScriptOpen(fileName);
	if (NULL == fp)
	{
		return EDPAT_FAILED;
	};

	retVal = 1;
	while ( 0 < (statementLen =
			ScriptReadStatement(fp,testScriptStatement)))
	{
		retVal = ScriptSubstituteVariables(testScriptStatement);
		if (0 > retVal)
		{
			CurrentTestResult = EDPAT_TEST_RESULT_SKIPPED;
			continue;
		}


		if (	( '@' != testScriptStatement[0] ) &&
			( EDPAT_TEST_RESULT_UNKNOWN !=  CurrentTestResult) &&
			( EDPAT_TEST_RESULT_PASSED !=  CurrentTestResult))
		{
			// if test case is passed. Skip to next teste spec
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
				ScriptErrorMsgPrint("Unknow statement '%c(%d)'",
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


int main(int argc, char *argv[])
{
        char *scriptFileName;
        char *fileName;
	EDPAT_RETVAL retVal;
	int c;

	FILE *logFileFp = stdout;
	FILE *reportFileFp = stdout;
	FILE *errorFileFp = stderr;


	printf("Ethernet Data Plan Application Tester(EDPaT).\n");
	printf("Build %s %s.\n", __DATE__,__TIME__);
	printf("Copyrigt (C) XXX. All rights reserved.\n");

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
		errorFileFp = logFileFp; // Error msg also written to lof file
		if (NULL == logFileFp)
		{
			printf("\nERROR: Failed to open log file '%s'\n",
					fileName);
			exit(EXIT_FAILURE);
		}
	}
	printf("## Logs written to '%s'\n",fileName);

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
