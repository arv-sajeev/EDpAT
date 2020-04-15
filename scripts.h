/* SPDX-License-Identifier: BSD-3-Clause-Clear
 * https://spdx.org/licenses/BSD-3-Clause-Clear.html#licenseText
 * 
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
 * All rights reserved.
 */


#ifndef __SCRIPTS_H__
#define __SCRIPTS_H__ 1

FILE	*ScriptOpen(const char *fileName);
void	 ScriptClose(FILE *fp);
int	 ScriptReadStatement(FILE *fp, char *statement);
EDPAT_RETVAL	 ScriptIncludeFile(const char *line);
int	 ScriptSubstituteVariables(char *line);
void	 ScriptBacktracePrint(FILE *fp);

#endif
