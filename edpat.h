/*
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted (subject to the limitations in the disclaimer below) provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of Arvind Sajeev nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef __PKTGEN_H__
#define __PKTGEN_H__ 1

#define MAX_PKT_SIZE		9000	// Max eth packet size
#define MAX_ETH_PORT_NAME_LEN	20	// Max lenth of Eth Interface aname
#define MAX_ETH_PORT_COUNT	10	// Max Eth interaces supported
#define MAX_FILE_NAME_LEN	50
#define MAX_SCRIPT_FILE_DEPTH	5
#define MAX_SCRIPT_STATEMENT_LEN	9000
#define MAX_SCRIPT_LINE_LEN	200
#define MAX_TESTCASE_ID_LEN	11
#define MAX_TIMESTAMP_LEN        40
#define MAX_VAR_COUNT		100
#define MAX_CS_SIZE		10
#define LICENSE_PROMPT "Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)\nAll rights reserved\n\n"
// timeout period while wating from reading pkt from interface.
#define PKT_RECEIVE_TIMEOUT	3	

typedef enum {
	EDPAT_FALSE	= 0,
	EDPAT_TRUE	= 1,
	}	EDPAT_BOOL;

typedef enum {
	EDPAT_SUCCESS	= 1,
	EDPAT_FAILED	= 0,
	EDPAT_NOTFOUND	= (-1)
	}	EDPAT_RETVAL;

typedef enum {
	EDPAT_TEST_RESULT_FAILED	= 0,
	EDPAT_TEST_RESULT_PASSED	= 1,
	EDPAT_TEST_RESULT_SKIPPED	= 2,
	EDPAT_TEST_RESULT_UNKNOWN	= 3
	}	EDPAT_TEST_RESULT;


extern char CurrentTestCaseId[];
extern EDPAT_TEST_RESULT CurrentTestResult;
extern int PacketReceiveTimeout;
extern EDPAT_BOOL EnableBroadcastPacketFiltering;
extern EDPAT_BOOL SyntaxCheckOnly;
extern EDPAT_BOOL PromiscuousModeEnabled;

int TestScriptProcess(const char *fileName);

#endif
