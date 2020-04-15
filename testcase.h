/* SPDX-License-Identifier: BSD-3-Clause-Clear
 * https://spdx.org/licenses/BSD-3-Clause-Clear.html#licenseText
 * 
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
 * All rights reserved.
 */

#ifndef __TESTCASE_H__
#define __TESTCASE_H__ 1

EDPAT_RETVAL GetTestCaseID(const char *line);
void CleanupLastTestExecution(void);

#endif
