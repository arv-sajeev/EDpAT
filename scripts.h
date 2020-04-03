#ifndef __SCRIPTS_H__
#define __SCRIPTS_H__ 1

FILE	*ScriptOpen(const char *fileName);
void	 ScriptClose(FILE *fp);
int	 ScriptReadStatement(FILE *fp, char *statement);
EDPAT_RETVAL	 ScriptIncludeFile(const char *line);
int	 ScriptSubstituteVariables(char *line);
void	 ScriptBacktracePrint(FILE *fp);

#endif
