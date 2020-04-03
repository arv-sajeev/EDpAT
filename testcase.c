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

void CleanupLastTestExecution(void)
{
	CheckUnexpectedPackets();
	if (EDPAT_TEST_RESULT_UNKNOWN != CurrentTestResult)
	{
		TestCaseFinalResultPrint();
	}
	return;
}

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


EDPAT_RETVAL GetTestCaseID(const char *in)
{
        char *p;
        char *testCaseId;
        int i,len;
	static char line[MAX_SCRIPT_STATEMENT_LEN];

	strcpy(line,in);

        testCaseId = strtok(&line[1]," ");
        if (NULL == testCaseId)
        {
                ScriptErrorMsgPrint("Test case ID expected");
                return EDPAT_FAILED;
        }

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
	
	/* Print result of privious test case and clean-up */
	CleanupLastTestExecution();

	// initialize the result to passed to begin with
	CurrentTestResult = EDPAT_TEST_RESULT_PASSED;

        strcpy(CurrentTestCaseId,testCaseId);
	TestCaseStringPrint("######### TEST CASE = %s #########",
				CurrentTestCaseId);
	TestCaseStringPrint("TIME %s",getTime());
	
        return EDPAT_SUCCESS;
}

