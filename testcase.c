/* SPDX-License-Identifier: BSD-3-Clause-Clear
 * https://spdx.org/licenses/BSD-3-Clause-Clear.html#licenseText
 * 
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
 * All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "edpat.h"
#include "scripts.h"
#include "packet.h"
#include "print.h"


/***********************
 *	
 *	CleanupLastTestExecution()
 *
 *	It cleans the memory buffer and checks if any unwanted packets
 *	have been received
 *	
 *	Arguments 	:	void
 *	Return 		: 	void, but sets the CurrentTestResult
 *
 *
 * *********************/

void CleanupLastTestExecution(void)
{
	CheckUnexpectedPackets();
	if (EDPAT_TEST_RESULT_UNKNOWN != CurrentTestResult)
	{
		TestCaseFinalResultPrint();
	}
	return;
}

/*************************
 *
 *	getTime
 *
 *	Get a timestamp
 *
 *	Arguments	:	void
 *	Retrun		: 	timestamp in format	%Y-%m-%d %H:%M:%S
 *
 *
 * ***********************/

static const char *getTime(void)
{
        time_t t;
        struct tm* tm;
        static char str[MAX_TIMESTAMP_LEN];

        time(&t);
        tm = localtime(&t);
        strftime(str,MAX_TIMESTAMP_LEN,"%Y-%m-%d %H:%M:%S", tm);
        return str;
}

/*************************
 *
 *	GetTestCaseID
 *
 *	Searches for the tescaseID from the input statementn and sets
 *	CurrentTestCaseId and prints the entry message for the testcase
 *	
 *	Arguments	:	in 	- The input testcase statement
 *					  that is under consideration 
 *	Return		:	EDPAT_RETVAL	
 *
 *
 * ***********************/
EDPAT_RETVAL GetTestCaseID(const char *in)
{
        char *p;
        char *testCaseId;
        int i,len;
	static char line[MAX_SCRIPT_STATEMENT_LEN+1];
	
	// copy the testcase statement 

	strncpy(line,in,MAX_SCRIPT_LINE_LEN);
	line[MAX_SCRIPT_STATEMENT_LEN]=0;

        testCaseId = strtok(&line[1]," ");

	// Check for whether testcase ID exists
        if (NULL == testCaseId)
        {
                ScriptErrorMsgPrint("Test case ID expected");
                return EDPAT_FAILED;
        }
	//Matches the prescribed format
        len =strlen(testCaseId);
        for(i=0; i < len; i++)
        {
                if ((!isalnum(testCaseId[i])) && ('_' != testCaseId[i]))
                {
                	ScriptErrorMsgPrint("Invalid Test case ID",
				"Need to be one word which contains only "
				"alphanumeric and '_'");
                        return EDPAT_FAILED;
                }
        }

	//Is within the max size
        if (MAX_TESTCASE_ID_LEN <= len)
        {
                ScriptErrorMsgPrint("Testcase ID too long");
                return EDPAT_FAILED;
        }


        p = strtok(NULL," ");
        if (NULL != p)
	{
                ScriptErrorMsgPrint("Unexpected string '%s' "
			"after Test Case ID", p);
                return EDPAT_FAILED;
	}
	
	/* Print result of previous test case and clean-up */
	CleanupLastTestExecution();

	// initialize the result to passed to begin with
	CurrentTestResult = EDPAT_TEST_RESULT_PASSED;

        strncpy(CurrentTestCaseId,testCaseId,MAX_TESTCASE_ID_LEN);
	CurrentTestCaseId[MAX_TESTCASE_ID_LEN]=0;
	TestCaseStringPrint("######### TEST CASE = %s #########",
				CurrentTestCaseId);
	TestCaseStringPrint("TIME %s",getTime());
	
        return EDPAT_SUCCESS;
}

