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
#include "variable.h"
#include "print.h"
#include "utils.h"

typedef struct {
        char    fileName[MAX_FILE_NAME_LEN];
        int     lineNo;
}  SCRIPT_INFO;

static int ScriptFileDepth = (-1);
static SCRIPT_INFO ScriptFileInfoTable[MAX_SCRIPT_FILE_DEPTH];
static char ScriptLine[MAX_SCRIPT_LINE_LEN];

int ScriptReadStatement(FILE *fp,char *scriptStatement)
{
	int stLen = 0;
	int i;
	EDPAT_BOOL wasLastCharSpace = EDPAT_TRUE;
	

	scriptStatement[0] = 0;
	while(NULL != fgets(ScriptLine,MAX_SCRIPT_LINE_LEN,fp))
	{
		ScriptFileInfoTable[ScriptFileDepth].lineNo++;
		for(i=0; (0 != ScriptLine[i]); i++) 
		{
			if (	( 32 > ScriptLine[i]) ||
				( 126 < ScriptLine[i])   )
			{
				// replace any control char with space
				ScriptLine[i] = ' ';
			}; 
			if ('!' == ScriptLine[i])
			{
				//  Skip rest of line after '!'
				if (EDPAT_TRUE != wasLastCharSpace)
				{
					scriptStatement[stLen] = ' ';
					stLen++;
					wasLastCharSpace = EDPAT_TRUE;
				}
				break;
			};
			if (';' == ScriptLine[i])
			{
				// end of statement marker ';' found
				if ( 0 == stLen)
				{
					// nothing int the statment. Skip it
					continue;
				}
				scriptStatement[stLen]=0;
				return stLen;
			};
			if (' ' == ScriptLine[i])
			{
				if ( EDPAT_FALSE == wasLastCharSpace) 
				{
					scriptStatement[stLen]
						= ScriptLine[i];
					stLen++;
				}
				wasLastCharSpace = EDPAT_TRUE;
			}
			else
			{
				scriptStatement[stLen] = ScriptLine[i];
				stLen++;
				wasLastCharSpace = EDPAT_FALSE;
			}
		}
	}
	scriptStatement[stLen]=0;
	return stLen;
}


FILE *ScriptOpen(const char *fileName)
{
	FILE *fp;

	if ( (MAX_SCRIPT_FILE_DEPTH - 1)  <= ScriptFileDepth)
	{
		ScriptErrorMsgPrint("Cyclic file inclusion");
		return EDPAT_FAILED;
	}


	fp = fopen(fileName,"r");

	if ( NULL == fp )
	{
		if (0 > ScriptFileDepth)
		{
			printf("ERROR: Failed to open script file '%s':"
				"%s(%d)\n",
				fileName,
				strerror(errno),
				errno);
		}
		else
		{
			ScriptErrorMsgPrint(
				"Failed to open script file '%s'",
				fileName);
		}
		return NULL;
	}

	ScriptFileDepth++;
	strcpy(ScriptFileInfoTable[ScriptFileDepth].fileName,fileName);
	ScriptFileInfoTable[ScriptFileDepth].lineNo=0;

	VerboseStringPrint("Opened script '%s' with Fp=%p",fileName,fp);
	return fp;

}

void ScriptClose(FILE *fp)
{
	fclose(fp);
	ScriptFileDepth--;
	VerboseStringPrint("Closed script with Fp=%p",fp);
}

EDPAT_RETVAL ScriptIncludeFile(const char *in)
{
        char *includeFileName;
	static char line[MAX_SCRIPT_STATEMENT_LEN];
	int i;
        int retVal;
	char *p = line;

	strcpy(line,&in[1]); // Skip 1st character '#'

        includeFileName = strtok(p," ");
        if (NULL == includeFileName)
        {
                ScriptErrorMsgPrint("Missing name of the file to be include");
                return EDPAT_FAILED;
        };

	// Validate file name
	for (i=0; 0 != includeFileName[i]; i++)
	{
		if (	(! isalnum(includeFileName[i])) &&
			( '.' != includeFileName[i]) &&
			( '_' != includeFileName[i]) &&
			( '/' != includeFileName[i]) )
		{
                	ScriptErrorMsgPrint("Invalid filename '%s'",
				includeFileName);
                	return EDPAT_FAILED;
		}
	}

        p = strtok(NULL," ");
        if (NULL != p)
	{
                ScriptErrorMsgPrint("Unexpected string '%s' after file name",p);
                return EDPAT_FAILED;
	}

        retVal = TestScriptProcess(includeFileName);
        return retVal;
}

void ScriptBacktracePrint(FILE *fp)
{
	int i;

	/* print script file name */
	for (i=ScriptFileDepth; i >=0;  i--)
	{
		fprintf(fp,"\tin %s at line %d\n",
			ScriptFileInfoTable[i].fileName,
			ScriptFileInfoTable[i].lineNo);
	}
	return;
}


int      ScriptSubstituteVariables(char *iline)
{
	int retVal;
	char *inPtr, *outPtr, *varPtr;
	char *varName, *varValue;
	int substituteCount = 0;
	char oline[MAX_SCRIPT_STATEMENT_LEN];

	// copy just the 1st byte to out buffer
        inPtr = &iline[1]; 
        oline[0]=iline[0]; 
        oline[1]=0;

        varPtr = strchr(inPtr,'$');
        while(NULL != varPtr)
        {
		substituteCount++;
		varPtr[0] =0;
		strcat(oline,inPtr);
		inPtr += (strlen(inPtr) + 1);

		// Read variable name
		varPtr++; 	// skip '$' character
		varName = strtok(varPtr," ");
		if (NULL ==  varName)
		{
			ExecErrorMsgPrint("Variable name not found");
			return -1;
		}

		varValue = VariableGetValue(varName);
		if (NULL == varValue)
		{
			ScriptErrorMsgPrint("Undefined variable '%s'",varName);
			return -1;
		}
		strcat(oline,varValue);
		strcat(oline," ");

		inPtr += (strlen(varName) + 1);

        	varPtr = strchr(inPtr,'$');
        }
	strcat(oline,inPtr);
	if (0 !=  substituteCount)
	{
		strcpy(iline,oline);
		retVal = ScriptSubstituteVariables(iline);
		if (0 > retVal) return -1;
	}
	return substituteCount;
}

