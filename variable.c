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

#include "edpat.h"
#include "scripts.h"
#include "utils.h"
#include "variable.h"
#include "print.h"

typedef struct {
        char *varName;
        char *varValue;
}       VAR_INFO;

static VAR_INFO VarList[MAX_VAR_COUNT];
static int NextFreeVarListIndex = 0;

/***********************
 *   VariableStoreValue()
 *
 *   Store a variable and its value in VarList[] table
 *
 *   Arguments : testScriptStatement - 	INPUT. A null terminated string which a the 
 *					Test script statement in format
 *					$<varName>=<varVal>
 *
 *   Return:	- EDPAT_SUCESS or EDPAT_FAILED
 *
 ***********************/

EDPAT_RETVAL VariableStoreValue(const char *testScriptStatement)
{
        char tmp[MAX_SCRIPT_STATEMENT_LEN];
        char *varName;
        char *varValue;
        int i;

	/* check syntax and seperate varible name and its value from statment */
        strcpy(tmp,testScriptStatement);
        varName = tmp;
        varValue = strchr(tmp,'=');
        if (NULL == varValue)
        {
                ScriptErrorMsgPrint("Expecting the format $vaname=value");
                return EDPAT_FAILED;
        }
	// cz this is the point where the = sign is found puttinh a 0 makes it two different strings
        varValue[0] = 0; // null teminate varName;

        varValue++; // skil '='

        TrimStr(varName);
        TrimStr(varValue);

        // check variable is already existing. If yes, overwrite value  log this in logfile if in verbose mode
	
        for(i=0; i < NextFreeVarListIndex; i++)
        {
                if (0 == strcmp(varName,VarList[i].varName))
                {
			VerboseStringPrint("Variable '%s' value '%s' is "
				" replace with new value '%s'.",
                                varName,
                                VarList[i].varValue,
                                varValue);
                        free(VarList[i].varValue);
                        VarList[i].varValue = malloc(strlen(varValue));
                        strcpy(VarList[i].varValue,varValue);
                        return EDPAT_SUCCESS;
                }
        }
        // value does not exits. So create a new entry

	if (MAX_VAR_COUNT <= (NextFreeVarListIndex+1))
	{
                ScriptErrorMsgPrint("Too many variables %d defined."
			"only %d are allowed",
			NextFreeVarListIndex, MAX_VAR_COUNT);
			return EDPAT_FAILED;
	}
        VarList[NextFreeVarListIndex].varName = malloc(strlen(varName));
        VarList[NextFreeVarListIndex].varValue = malloc(strlen(varValue));
        strcpy(VarList[NextFreeVarListIndex].varName,varName);
        strcpy(VarList[NextFreeVarListIndex].varValue,varValue);
        NextFreeVarListIndex++;
	VerboseStringPrint("Variable '%s' with value '%s' added.",
			varName,varValue);
	VariablePrintValues();
        return EDPAT_SUCCESS;
}

/***********************
 *   VariableGetValue()
 *
 *   Get value of a varible from the table.
 *
 *   Arguments
 *      varName - INPUT. Name for the variable for which value is needed.
 *
 *   Return:
 *	value of the variable. NULL if not found.
 *
 **********************/
char *VariableGetValue(const char *varName)
{
	int i;
	for(i=0; i < NextFreeVarListIndex; i++)
	{
		if (0  == strcmp(VarList[i].varName,varName))
		{
			return VarList[i].varValue;
		}
	}
	return NULL;
}


/***********************
 *   VariablePrintValues()
 *
 *   Print variable name and its value on the screent.
 *
 *   Arguments :
 *      None.
 *
 *   Return:
 *	None
 *
 ***********************/
void VariablePrintValues(void)
{
	int i;
	static char msg[MAX_SCRIPT_STATEMENT_LEN];
	static char var[MAX_SCRIPT_LINE_LEN];

	strcpy(msg,"Variables Stored:");

	for(i=0; i < NextFreeVarListIndex; i++)
	{
		sprintf(var,"\n%s='%s'",
			VarList[i].varName,
			VarList[i].varValue);
		if ( MAX_SCRIPT_STATEMENT_LEN > (strlen(msg)+strlen(var)))
		{
			strcat(msg,var);
		}
		else
		{
			VerboseStringPrint(msg);
			strcpy(msg,"Variables Stored(Next set):");
		}
	}
	VerboseStringPrint(msg);
	return;
}

