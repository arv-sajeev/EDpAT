/* SPDX-License-Identifier: BSD-3-Clause-Clear
 * https://spdx.org/licenses/BSD-3-Clause-Clear.html#licenseText
 * 
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
 * All rights reserved.
 */


#ifndef __VARIABLE_H__
#define __VARIABLE_H__	1

EDPAT_RETVAL VariableStoreValue(const char *testScriptStatement);
char *VariableGetValue(const char *varName);
void VariablePrintValues(void);

#endif
