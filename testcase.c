/*
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted (subject to the limitations in the disclaimer below) provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of Arvind Sajeev nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



/*
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
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
#include <time.h>

#include "edpat.h"
#include "scripts.h"
#include "packet.h"
#include "print.h"


/***********************
 *	
 *	CleanupLastTestExecution()
 *
 *	It cleans the memory buffer and checks if any unwanted packets have been received
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
	
	// copy the testcase statement 

	strcpy(line,in);
        testCaseId = strtok(&line[1]," ");

	// Check for whether testcase ID matches format or exists
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
	
	/* Print result of previous test case and clean-up */
	CleanupLastTestExecution();

	// initialize the result to passed to begin with
	CurrentTestResult = EDPAT_TEST_RESULT_PASSED;

        strcpy(CurrentTestCaseId,testCaseId);
	TestCaseStringPrint("######### TEST CASE = %s #########",
				CurrentTestCaseId);
	TestCaseStringPrint("TIME %s",getTime());
	
        return EDPAT_SUCCESS;
}

