/* SPDX-License-Identifier: BSD-3-Clause-Clear
 * https://spdx.org/licenses/BSD-3-Clause-Clear.html#licenseText
 * 
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
 * All rights reserved.
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
        char tmp[MAX_SCRIPT_STATEMENT_LEN+1];
        int len, i;

        strncpy(tmp,str,MAX_SCRIPT_STATEMENT_LEN);
	tmp[MAX_SCRIPT_STATEMENT_LEN]=0;

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

        strcpy(str,&tmp[i]);
        return;
}


