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


/***************************
 *
 *
 *	ScriptReadStatement()
 *
 *	Read a statement from the file provided
 *
 *	Arguments 	: 	fp - the input file handle, also keeps track of the current position of the read
 *				scriptStatement - the statement that is read is loaded to this string
 *				scriptStatement will have comments removed 
 *	Return 		:	the strlen or length of the statement read
 *
 *
 *
 * ************************/


int ScriptReadStatement(FILE *fp,char *scriptStatement)
{
	int stLen = 0;
	int i;
	EDPAT_BOOL wasLastCharSpace = EDPAT_TRUE;
	

	scriptStatement[0] = 0;

	// get each line
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
				// remove extra spaces jusst keep one if needed
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
				// store everything else into scriptStatement
				scriptStatement[stLen] = ScriptLine[i];
				stLen++;
				wasLastCharSpace = EDPAT_FALSE;
			}
		}
	}

	// null terminate string
	scriptStatement[stLen]=0;
	return stLen;
}


/***************
 *
 *	ScriptOpen()
 *
 * 	Opens the file with name given, checks if within MAX limit
 * 	Log the scriptfile depth, name and line no in the ScriptFileInfoTable
 *
 *	Arguments	:	fileName 	- 	Name of the file to be opened
 *	Return 		:	the file pointer received on opening said file
 *
 *
 ***************/
FILE *ScriptOpen(const char *fileName)
{
	FILE *fp;
	// check whether file inclusion depth is reached
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
	
	// Include the file in the scriptfileinfo table by adding its name and line no
	ScriptFileDepth++;
	strcpy(ScriptFileInfoTable[ScriptFileDepth].fileName,fileName);
	ScriptFileInfoTable[ScriptFileDepth].lineNo=0;

	VerboseStringPrint("Opened script '%s' with Fp=%p",fileName,fp);
	return fp;

}

/***************
 *
 * 	ScriptClose()
 *
 * 	Closes the file pointer provided 
 *	Updates te ScriptFileInfoTable
 *
 *	Arguments	:	fp	-	the input filepointer
 *
 * 	Return 		:	void
 *
 *
 * *************/

void ScriptClose(FILE *fp)
{
	fclose(fp);
	ScriptFileDepth--;
	VerboseStringPrint("Closed script with Fp=%p",fp);
}


/*****************************
 *
 * 	ScriptIncludeFile
 *	
 *	Validates and checks the filename in the statment provided and then evaluate the included file
 * 	
 * 	Arguments	:	in 	-	the current testcase statement under consideration 
 * 	Return 		:	EDPAT_REVAL saying whether the file was successfully included
 *
 *
 *****************************/

EDPAT_RETVAL ScriptIncludeFile(const char *in)
{
        char *includeFileName;
	static char line[MAX_SCRIPT_STATEMENT_LEN];
	int i;
        int retVal;
	char *p = line;

	strcpy(line,&in[1]); // Skip 1st character '#' and copy the rest to line
        includeFileName = strtok(p," ");	//Find include filename

	// Error handle
        if (NULL == includeFileName)
        {
                ScriptErrorMsgPrint("Missing name of the file to be include");
                return EDPAT_FAILED;
        }

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

	//Check if unexpected string is there after the include file name
        p = strtok(NULL," ");
        if (NULL != p)
	{
                ScriptErrorMsgPrint("Unexpected string '%s' after file name",p);
                return EDPAT_FAILED;
	}

	//Process the include file 
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

/*****************************
 *
 *	ScriptSubstituteVariables()
 *
 *	Substitute the variables in the test statement with the corressponding values in the VarList
 *
 *	Arguments	:	iline			-	the testcase statement that is currently under consideration
 *	Return		:	substituteCount		-	the number of substitution made
 *
 *
 * ***************************/

int  ScriptSubstituteVariables(char *iline)
{
	int retVal;
	char *inPtr, *outPtr, *varPtr;
	char *varName, *varValue;
	int substituteCount = 0;
	char oline[MAX_SCRIPT_STATEMENT_LEN];

        inPtr = &iline[1]; 

	//Copy just the first byte i.e the command to the output buffer
        oline[0]=iline[0]; 
        oline[1]=0;
	//Null terminate after operation 

	// Get position of the first occurence of a dollar
        varPtr = strchr(inPtr,'$');
        while(NULL != varPtr)
        {
		substituteCount++;
		// Set the dollar character as 0 
		varPtr[0] =0;
		strcat(oline,inPtr);
		//Copy string until the varPtr into the outline
		inPtr += (strlen(inPtr) + 1);

		// Read variable name
		varPtr++; 	// skip '$' character
		varName = strtok(varPtr," ");
		if (NULL ==  varName)
		{
			ExecErrorMsgPrint("Variable name not found");
			return -1;
		}

		// Check for value of variable
		varValue = VariableGetValue(varName);
		if (NULL == varValue)
		{
			ScriptErrorMsgPrint("Undefined variable '%s'",varName);
			return -1;
		}

		//Add value coressponding to the variable to output buffer
		strcat(oline,varValue);
		strcat(oline," ");

		inPtr += (strlen(varName) + 1);

		//Find next variable
        	varPtr = strchr(inPtr,'$');
        }

	//Concatenate the part left after the last variable inclued inthe script
	strcat(oline,inPtr);
	if (0 !=  substituteCount)
	{
		strcpy(iline,oline);
		retVal = ScriptSubstituteVariables(iline);
		if (0 > retVal) return -1;
	}
	return substituteCount;
}

