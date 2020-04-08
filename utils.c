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
#include "edpat.h"
#include "print.h"
#include "utils.h"

/********************
 *
 *   TrimStr()
 *
 *   Remove leading and trailing blanks from a string
 *
 *   Arguments:
 *	str	-	INPUT and OUTPUT. String to be trimmed is as in this
 *			argument. The trimmed output is return in the same
 *			argument.
 *   Return	-	None
 *
 ********************/

void TrimStr(char *str)
{
        char tmp[MAX_SCRIPT_STATEMENT_LEN];
        int len, i;

        strncpy(tmp,str,MAX_SCRIPT_STATEMENT_LEN);

        len = strlen(tmp);

        for(i=(len-1) ; i >= 0; i--)
        {
                if (' ' != tmp[i])
                        break;
        };
        tmp[i+1] = 0; // null terminate
        len = i;
        for(i=0 ; i < len; i++)
        {
                if (' ' != tmp[i])
                {
                        break;
                }
        };

        strncpy(str,&tmp[i],MAX_SCRIPT_STATEMENT_LEN);
        return;
}


