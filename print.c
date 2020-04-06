/*
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
(subject to the limitations in the disclaimer below) provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the 
following disclaimer. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or other materials 
provided with the distribution. Neither the name of Arvind Sajeev nor the names of its contributors 
may be used to endorse or promote products derived from this software without specific prior written 
permission. NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <net/ethernet.h>


#include "edpat.h"
#include "scripts.h"
#include "print.h"

#define MAX_MSG_LEN		10000	// Max length of a message
#define MAX_CHAR_PER_LINE	80
#define BYTES_PER_LINE		10

static FILE *LogFp = NULL;
static FILE *ReportFp = NULL;
static FILE *ErrorFp = NULL;
static EDPAT_BOOL TimestampEnableFlag = EDPAT_FALSE;
static EDPAT_BOOL VerboseMsgEnableFlag = EDPAT_FALSE;
static char Msg[MAX_MSG_LEN];

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

static const char *getShortTime(void)
{
	time_t t;
	struct tm* tm;
	static char str[MAX_TIMESTAMP_LEN];
	
	time(&t);
	tm = localtime(&t);
	strftime(str,MAX_TIMESTAMP_LEN,"%M:%S|", tm);
	return str;
}

static void msgPrint(FILE *fp, const char *msg)
{
        int msgLen;
	int i;
	int lineLen;
	EDPAT_BOOL firstLineFlag = EDPAT_TRUE;
        static char buf[MAX_CHAR_PER_LINE+1];
	static char lMsg[MAX_MSG_LEN];
	char *linePregix = "## ";

	strcpy(lMsg,msg);
	char *pMsg;

	pMsg = strtok(lMsg,"\n");
	while( NULL != pMsg)
	{
        	msgLen = strlen(pMsg);
		if (fp == LogFp)
		{
			strcpy(buf,linePregix);
		}
		else
		{
			if (fp == ReportFp)
			{
				buf[0]=0;
			}
			else
			{
				strcpy(buf,"ERROR:");
			}
		}
		if ( EDPAT_TRUE == TimestampEnableFlag)
		{
			strcat(buf,getShortTime());
		}
		fprintf(fp,"%s",buf);
		lineLen=MAX_CHAR_PER_LINE-strlen(buf);
	        for (i=0; i < msgLen; )
	        {
			if( EDPAT_FALSE == firstLineFlag)
			{
				fprintf(fp,"   ");
			}
			else
			{
				firstLineFlag = EDPAT_FALSE;
			};
			if ((i+lineLen) > msgLen)
			{
				fprintf(fp,"%s\n",&pMsg[i]);
			} else
			{
				strncpy(buf,&pMsg[i],lineLen);
				buf[lineLen]=0;
				fprintf(fp,"%s\n",buf);
                	}
			i = i + lineLen;
			lineLen = (MAX_CHAR_PER_LINE-3);
		}
		linePregix = "   ";
		pMsg = strtok(NULL,"\n");
        }
        return;
}


/********************
 *
 *
 * 	pktPrint()
 * 
 * 	Print buffer content is hex format
 *	
 * 	Arguments	:	fp	- 	file handle to print
 *				p	- 	pointer to the buffer that needs to be printed
 *				pktLen	- 	length of the buffer to be printed.
 *
 *	Return 		: 	void
 *
 *
 *******************/

static void pktPrint(FILE *fp, const void *p, const int pktLen)
{
	int idx, j;
	unsigned char *pkt = (unsigned char *)p;

	fprintf(fp,"   ");
	for (idx=0; idx < pktLen; idx++)
	{
		if (0 == (idx % BYTES_PER_LINE))
		{
			fprintf(fp,"%04d:",idx);
		}

		fprintf(fp," %02X",pkt[idx]);

		if (9 == (idx % 10 ))
		{
			fprintf(fp," ");
			if ((BYTES_PER_LINE-1) == (idx  % BYTES_PER_LINE))
			{
				fprintf(fp," ");
				for(j=(idx-BYTES_PER_LINE); j < idx; j++)
				{
					if (isprint(pkt[j]))
					{
						fprintf(fp,"%c",pkt[j]);
					}
					else
					{
						fprintf(fp,".");
					}
				}
				fprintf(fp,"\n   ");
			}
		}
	}

	for (j=0; j < (BYTES_PER_LINE - (idx % BYTES_PER_LINE)); j++)
	{
		fprintf(fp,"   ");
		if (9 == (j % 10 )) fprintf(fp," ");
	}
	fprintf(fp,"  ");

	j = idx - (idx % BYTES_PER_LINE);
	int k = j;
	for(; j < idx; j++)
	{
		if (isprint(pkt[j]))
		{
			fprintf(fp,"%c",pkt[j]);
		}
		else
		{
			fprintf(fp,".");
		}
	}
	fprintf(fp,"\n");
	
	return;
}

/**************************
 *
 * 	ethHeaderPrint()
 *
 * 	Print ethernet header dest src mac addresses
 *
 * 	Arguments	:	fp	- 	file handle to print
 *				p	- 	pointer to ethernet raw packet
 *	Return 		: 	void
 *
 *************************/
static void ethHeaderPrint(FILE *fp, const void *p)
{
	struct ether_header *eh = (struct ether_header *) p;

	fprintf(fp, "   MAC#%02X:%02X:%02X:%02X:%02X:%02X -> "
		   "%02X:%02X:%02X:%02X:%02X:%02X\n",
		   eh->ether_shost[0],eh->ether_shost[1],eh->ether_shost[2],
		   eh->ether_shost[3],eh->ether_shost[4],eh->ether_shost[5],
		   eh->ether_dhost[0],eh->ether_dhost[1],eh->ether_dhost[2],
		   eh->ether_dhost[3],eh->ether_dhost[4],eh->ether_dhost[5]);
	return;
}


EDPAT_RETVAL  InitMsgPrint(FILE *logFp, FILE *reportFp, FILE *errFp)
{
	LogFp = logFp;
	ReportFp = reportFp;
	ErrorFp = errFp;
	fprintf(LogFp,"TIME %s\n",getTime());
	return EDPAT_SUCCESS;
}

void VerboseMsgEnable(void)
{
	VerboseMsgEnableFlag = EDPAT_TRUE;
	printf("## Verbose Enabled.\n");
	return;
}

void VerboseStringPrint( const char *format, ...)
{
	va_list ap;

	if ( EDPAT_TRUE != VerboseMsgEnableFlag)
	{
		return;
	}
	
	// construct error string from veriable argumnets
	va_start (ap, format);
	vsnprintf (Msg, MAX_MSG_LEN, format, ap);
	va_end (ap);
	
	msgPrint(LogFp,Msg);
	return;

}

void VerbosePacketPrint(const void *pkt, const int pktLen)
{
	if ( EDPAT_TRUE != VerboseMsgEnableFlag)
	{
		return;
	}
	pktPrint(LogFp, pkt, pktLen);
}

void VerbosePacketMaskPrint(const void *pkt, const int pktLen)
{
	int i;
	signed short *p = (signed short *)pkt;
	if ( EDPAT_TRUE != VerboseMsgEnableFlag)
	{
		return;
	}

	for(i=0; i < pktLen; i++)
	{
		if (0 == (i % BYTES_PER_LINE))
		{
			fprintf(LogFp,"\n         ");
		}
		switch(p[i])
		{
			case -1:
				fprintf(LogFp,".. ");
				break;
			case -2:
				fprintf(LogFp,"SK ");
				break;
			default:
				fprintf(LogFp,"%-3d",p[i]);

		}
	}
	fprintf(LogFp,"\n");
}

void VerbosePacketHeaderPrint(const void *pkt)
{
	if ( EDPAT_TRUE != VerboseMsgEnableFlag)
	{
		return;
	}
	ethHeaderPrint(LogFp, pkt);
}


void ScriptErrorMsgPrint( const char *format, ...)
{
	int i;
	va_list ap;

	strcpy(Msg,"Script Error: ");
	// construct error string from veriable argumnets
	va_start (ap, format);
	vsnprintf (&Msg[14], MAX_MSG_LEN, format, ap);
	va_end (ap);
	
	msgPrint(LogFp,Msg);

	/* print script file name */
	ScriptBacktracePrint(LogFp);
	return;

}
void lexecErrorPrint( const char *srcFileName,
                const int lineNo,
                const char *funcName,
                const char *format, ...)
{
	va_list ap;
	int errMsgLen;

        sprintf(Msg,"%s|%d|%s|err=%s(%d) ",
                        srcFileName,
                        lineNo,
                        funcName,
			strerror(errno),
                        errno);
	errMsgLen = strlen(Msg);
	
	// construct error string from veriable argumnets
	va_start (ap, format);
	vsnprintf (&Msg[errMsgLen], (MAX_MSG_LEN-errMsgLen), format, ap);
	va_end (ap);
	
	msgPrint(ErrorFp,Msg);
	return;
}

void MsgTimestampEnable(void)
{
	TimestampEnableFlag = EDPAT_TRUE;
	printf("## Timestamp Enabled.\n");
}
/**********************************
 *
 *	TestCaseFinalResultPrint()
 *
 *	It prints the result after running PacketProcess() or CleanupLastTestExecution
 *	
 *	Arguments	:	void, but uses CurrentTestResult and CurrentTestCaseId
 *
 *	Return		:	void
 *
 *
 * ********************************/

void TestCaseFinalResultPrint(void)
{
	char result[MAX_TESTCASE_ID_LEN+15];
	switch(CurrentTestResult)
	{
		case EDPAT_TEST_RESULT_FAILED:
			sprintf(result,"%s\t-> FAILED",CurrentTestCaseId);
			break;
		case EDPAT_TEST_RESULT_PASSED:
			sprintf(result,"%s\t-> PASSED",CurrentTestCaseId);
			break;
		case EDPAT_TEST_RESULT_SKIPPED:
			sprintf(result,"%s\t-> SKIPPED",CurrentTestCaseId);
			break;
		case EDPAT_TEST_RESULT_UNKNOWN:
		default:
			ExecErrorMsgPrint("Unknown test result %d "
				"for Test ID '%s'",
				CurrentTestResult,CurrentTestCaseId);
			return;
	}

	printf("%s\n",result);
	if (LogFp != stdout)
	{
		msgPrint(LogFp,result);
	}
	if (ReportFp != stdout)
	{
		msgPrint(ReportFp,result);
	}
	return;
}

void TestCaseStringPrint( const char *format, ...)
{
	va_list ap;

	// construct error string from variable argumnets
	va_start (ap, format);
	vsnprintf (Msg, MAX_MSG_LEN, format, ap);
	va_end (ap);
	
	msgPrint(LogFp,Msg);
	return;

}

void TestCasePacketPrint(const void *pkt, const int pktLen)
{
	pktPrint(LogFp, pkt, pktLen);
}

void TestCasePacketHeaderPrint(const void *pkt)
{
	ethHeaderPrint(LogFp, pkt);
}


/*****************
 *
 * 	PrintUsageInfo
 *
 * 	prints usage info
 *	
 *	Arguments	: 	exeName
 *	Return 		: void
 *
 ******************/

void PrintUsageInfo(const char *exeName)
{
	printf("\nUsage: ");
	printf(
		"%s [-f] [-h] [-s] [-t] [-v] [-w <waittimeout>] <input-script> [<logfile> [<reportfile>]]\n",
		exeName);

	printf("\n\t-f\t- Do not filter broadcast packets. ");
	printf("\n\t\t  All IPv6 packets and ARP,LLDP,IGMP,DHCP,SSDP and MDNS");
	printf("\n\t\t  are discarded/filtered by default.");
	printf("\n\t\t  This flag disable this filtering.");
	printf("\n\t-h\t- Help. Display usage info and exit.");
	printf("\n\t-p\t- Enable promiscuous mode. Default is disabled");
	printf("\n\t-s\t- Syntax checking only. Do not execute test");
	printf("\n\t-t\t- Enable timestamping of entries in <logfile>");
	printf("\n\t-v\t- Enable verbose mode in <logfile>");
	printf("\n\t-w\t- Timeout period while waiting for receiving packet.");
	printf("\n\t\t  <waittimeout> is specified in seconds.");
	printf("\n\t\t  If not specified, %d seconds is assumed.",
			PacketReceiveTimeout);
	printf("\n<input-script>\t- input test script file. "
			"Mandatory parameter");
	printf("\n<logfile>\t- output file for test logs. "
			"If omitted, 'stdout' is used"
			"\n\t\t  If a file already exists, appended to it. "
			"\n\t\t  Else, create a new file");
	printf("\n<reportfile>\t- output file for test report. "
			"If omitted, 'stdout' is used"
			"\n\t\t  If a file already exists, appended to it. "
			"\n\t\t  Else, create a new file\n");
}



