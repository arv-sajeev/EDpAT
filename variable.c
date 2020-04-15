/* SPDX-License-Identifier: BSD-3-Clause-Clear
 * https://spdx.org/licenses/BSD-3-Clause-Clear.html#licenseText
 * 
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
 * All rights reserved.
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
 *   Arguments : testScriptStatement - 	INPUT. A null terminated string
 *			which a the Test script statement in format
 *			$<varName>=<varVal>
 *
 *   Return:	- EDPAT_SUCESS or EDPAT_FAILED
 *
 ***********************/

EDPAT_RETVAL VariableStoreValue(const char *testScriptStatement)
{
        char tmp[MAX_SCRIPT_STATEMENT_LEN+1];
        char *varName;
        char *varValue;
        int i;

	/* check syntax and seperate variable name and its value
	   from statment */
        strncpy(tmp,&testScriptStatement[1],MAX_SCRIPT_STATEMENT_LEN-1);
	tmp[MAX_SCRIPT_STATEMENT_LEN]=0;

        varName = tmp;
        varValue = strchr(tmp,'=');
        if (NULL == varValue)
        {
                ScriptErrorMsgPrint("Expecting the format $vaname=value");
                return EDPAT_FAILED;
        }
	/* cz this is the point where the = sign is found puttinh a 0
	   makes it two different strings */
        varValue[0] = 0; // null teminate varName;

        varValue++; // skil '='

        TrimStr(varName);
        TrimStr(varValue);

        /* check variable is already existing. If yes, overwrite value 
	   log this in logfile if in verbose mode */
	
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
                        VarList[i].varValue = malloc(strlen(varValue)+1);
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
        VarList[NextFreeVarListIndex].varName = malloc(strlen(varName)+1);
        VarList[NextFreeVarListIndex].varValue = malloc(strlen(varValue)+1);
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
 *   Arguments	:	varName - Name for the variable for which value
 *				  is needed.
 *
 *   Return	:	value of the variable. NULL if not found.
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
 *   Arguments 	:	void
 *
 *   Return	:	void
 *
 ***********************/
void VariablePrintValues(void)
{
	int i;
	static char msg[MAX_SCRIPT_STATEMENT_LEN+1];
	static char var[MAX_SCRIPT_LINE_LEN+1];

	strncpy(msg,"Variables Stored:",MAX_SCRIPT_STATEMENT_LEN);
	msg[MAX_SCRIPT_STATEMENT_LEN]=0;

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
			strncpy(msg,"Variables Stored(Next set):",
				MAX_SCRIPT_LINE_LEN);
			msg[MAX_SCRIPT_STATEMENT_LEN]=0;
			
		}
	}
	VerboseStringPrint(msg);
	return;
}

