/*
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted (subject to the limitations in the disclaimer below) provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of Arvind Sajeev nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
