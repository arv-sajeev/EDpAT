/* SPDX-License-Identifier: BSD-3-Clause-Clear
 * https://spdx.org/licenses/BSD-3-Clause-Clear.html#licenseText
 * 
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
 * All rights reserved.
 */


#ifndef __PRINT_H__
#define __PRINT_H__	1


#define ExecErrorMsgPrint(s...) lexecErrorPrint(__FILE__,__LINE__,__FUNCTION__,s)

int  InitMsgPrint(FILE *logFp, FILE *reportFp, FILE *errFp);

void VerboseMsgEnable(void);
void VerboseStringPrint( const char *format, ...);
void VerbosePacketPrint(const void *pkt, const int pktLen);
void VerbosePacketMaskPrint(const void *pkt, const int pktLen);
void VerbosePacketHeaderPrint(const void *pkt);
void PrintUsageInfo(const char *exeName);

void ScriptErrorMsgPrint( const char *format, ...);
void lexecErrorPrint( const char *srcFileName,
                const int lineNo,
                const char *funcName,
                const char *format, ...);
void MsgTimestampEnable(void);

void TestCaseFinalResultPrint(void);
void TestCaseStringPrint( const char *format, ...);
void TestCasePacketPrint(const void *pkt, const int pktLen);
void TestCasePacketHeaderPrint(const void *pkt);

#endif
