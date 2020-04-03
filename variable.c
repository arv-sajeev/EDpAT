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
 *   Arguments : 
 *	testScriptStatement - INPUT. A null terminated string which a the 
 *				Test script statement in format
 *					$<varName>=<varVal>
 *   Return:	- EDPAT_SUCESS or EDPAT_FAULED
 *
 ********/

EDPAT_RETVAL VariableStoreValue(const char *testScriptStatement)
{
        char tmp[MAX_SCRIPT_STATEMENT_LEN];
        char *varName;
        char *varValue;
        int i;

	/* check syntax and seperate varible name and its value from statment */
        strcpy(tmp,&testScriptStatement[1]);
        varName = tmp;
        varValue = strchr(tmp,'=');
        if (NULL == varValue)
        {
                ScriptErrorMsgPrint("Expecting the format $vaname=value");
                return EDPAT_FAILED;
        };
        varValue[0] = 0; // null teminate varName;

        varValue++; // skil '='

        TrimStr(varName);
        TrimStr(varValue);

        /* check variable is already exiting. If yes, overwrite value */
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
                ScriptErrorMsgPrint("Too many varible(%d) defined."
			"only %d is allowed",
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
	VaribalePrintValues();
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
 ********/
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
 *   VaribalePrintValues()
 *
 *   Print variable name and its value on the screent.
 *
 *   Arguments :
 *      None.
 *
 *   Return:
 *	None
 *
 ********/
void VaribalePrintValues(void)
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

