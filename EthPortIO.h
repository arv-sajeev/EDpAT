/* SPDX-License-Identifier: BSD-3-Clause-Clear
 * https://spdx.org/licenses/BSD-3-Clause-Clear.html#licenseText
 * 
 * Copyright (c) 2020-1025 Arvind Sajeev (arvind.sajeev@gmail.com)
 * All rights reserved.
 */


#ifndef __PETHPORTIO_H__
#define __PETHPORTIO_H__ 1

int EthPortOpen(const char *ifName);
EDPAT_RETVAL EthPortCloseAll(void);
EDPAT_RETVAL EthPortReceive(const char *ifName,
			unsigned char *data, int *dataLen);
EDPAT_RETVAL EthPortReceiveAny(char *ifName,
			unsigned char *data, int *dataLen);
EDPAT_RETVAL EthPortSend(const char *ifName,
			unsigned char *data, const int dataLen);
EDPAT_RETVAL EthPortClearBuf(void);


#endif
