#ifndef __PKTGEN_H__
#define __PKTGEN_H__ 1

#define MAX_PKT_SIZE		9000	// Max eth packet size
#define MAX_ETH_PORT_NAME_LEN	20	// Max lenth of Eth Interface aname
#define MAX_ETH_PORT_COUNT	10	// Max Eth interaces supported
#define MAX_FILE_NAME_LEN	50
#define MAX_SCRIPT_FILE_DEPTH	5
#define MAX_SCRIPT_STATEMENT_LEN	10000
#define MAX_SCRIPT_LINE_LEN	200
#define MAX_TESTCASE_ID_LEN	11
#define MAX_TIMESTAMP_LEN        40
#define MAX_VAR_COUNT		100

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
