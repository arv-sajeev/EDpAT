#ifndef __VARIABLE_H__
#define __VARIABLE_H__	1

EDPAT_RETVAL VariableStoreValue(const char *testScriptStatement);
char *VariableGetValue(const char *varName);
void VaribalePrintValues(void);

#endif
