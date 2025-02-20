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
#include "print.h"
#include "EthPortIO.h"


/* packets to be send or expected to be receved are specifed in hex
   in the input script. This will be converted to binary and stored 
   into 'SpecifiedPkt' array. In addition to hex value, the packet
   specification can contain special charactes like '*' and '?' etc.
   theyn will be indicated using SpecifiedPktMask[] array defined below
   as follows
	- if a hex value is specified, the specified value is stored in
	  SpecifiedPkt[i] and MASK_EXACT is stored in SpecifiedPktMask[i]
	  which indicates that the valie need to be send or received as is.
	- if '*' is specified, zero will be stored in SpecifiedPkt[i] and
	  MASK_SKIP is stored in SpecifiedPktMask[i] indicating that this 
	  byte needs to be skipped while comparing with a receved packet.
	  '*' is not valid for send packet.
	- if '?' is specifed, it need to be followed by a number K(e.g. ?32)
 	  which indicate that this byte needs to be replaced with Kth byte
	  of previously received packet. Hence zero will be stored in
	  SpecifiedPkt[i] and K is stored in SpecifiedPktMask[i].
	  '?' is not valid while specifing expected packet.
*/


typedef enum { OP_UNKNOWN, OP_SEND, OP_RECEIVE } OPERATION;

static OPERATION Operation = OP_UNKNOWN;
static char EthPortName[MAX_ETH_PORT_NAME_LEN+1];
static int	BytesInSpecifiedPkt;
static int	BytesInRecvPkt;
static int 	cs_array_siz;
static unsigned char RecvPkt[MAX_PKT_SIZE];
static unsigned char SpecifiedPkt[MAX_PKT_SIZE];


struct check_sum_mask	{
	unsigned int pos;
	unsigned int start;
	unsigned int end;
};

struct check_sum_mask cs_arr[MAX_CS_SIZE];
signed short SpecifiedPktMask[MAX_PKT_SIZE];

#define	MASK_EXACT	(-1)
#define	MASK_SKIP	(-2)
#define MASK_CS  	(-3)


/******************
 *
 *
 *	CheckUnexpectedPackets()
 *	
 *	After the testcase has passed check if there are any other
 *	unexpected packets left in the queue
 *	It uses receive any to read any packets left in the queue
 *	
 *	Arguments	:	void, also sets the CurrentTestResult to
 *				appropriate value
 *
 *
 *******************/

void CheckUnexpectedPackets(void)
{
	EDPAT_RETVAL retVal = EDPAT_SUCCESS;
	char portName[MAX_ETH_PORT_NAME_LEN+1];
	unsigned char pkt[MAX_PKT_SIZE];
	int pktLen = MAX_PKT_SIZE;

	while(EDPAT_SUCCESS == retVal)
	{
		pktLen = MAX_PKT_SIZE;
		retVal = EthPortReceiveAny(portName, pkt, &pktLen);
		if (EDPAT_SUCCESS == retVal)
		{
			CurrentTestResult = EDPAT_TEST_RESULT_FAILED;
			TestCaseStringPrint(
				"Unexpected message of len %d received "
				"from  '%s'", pktLen,portName);
			TestCasePacketHeaderPrint(pkt);
			TestCasePacketPrint(pkt, pktLen);
		}
	};
	return;
}

/*********************
 *
 *	packetRead
 *
 *	Parse the details from the test statement, set Operation ethport
 *	and process special characters
 *
 *	Arguments	:	in -	Test statement under consideration 
 *
 *	Return 		:	EDPAT_RETVAL
 *
 *
 * ********************/

static EDPAT_RETVAL packetRead(char *in)
{
	static char statement[MAX_SCRIPT_STATEMENT_LEN+1];
	char *token, *q;
	int i;
	EDPAT_RETVAL retVal;
	unsigned char byte;

	switch(in[0])
	{
		case '<':
			Operation = OP_RECEIVE;
			break;
		case '>':
			Operation = OP_SEND;
			break;
		default:
			ScriptErrorMsgPrint("Unexpected character '%c(%d)'",
				in[0],in[0]);
			return EDPAT_FAILED;
		
	}

	// Skip the     1st character Which will be operation
	strncpy(statement,&in[1],MAX_SCRIPT_STATEMENT_LEN-1);
	statement[MAX_SCRIPT_STATEMENT_LEN] = 0;
	//Copy the ethport name
	token = strtok(statement," ");
        if (NULL == token)
        {
                ScriptErrorMsgPrint("Missing Ethernet Port Name");
                return EDPAT_FAILED;
        };

	// Validate ethernet port name
	if (MAX_ETH_PORT_NAME_LEN <= strlen(token))
	{
		ScriptErrorMsgPrint("Ethernet Port Name '%s' too long. "
			"Need to be less than %d",
			token, MAX_ETH_PORT_NAME_LEN);
		return EDPAT_FAILED;
	}

	strncpy(EthPortName,token,MAX_ETH_PORT_NAME_LEN);
	EthPortName[MAX_ETH_PORT_NAME_LEN]=0;

	/* Open the port if not already open */
	retVal = EthPortOpen(EthPortName);
	if ( 0 >  retVal)
		return EDPAT_FAILED;

	strncpy(statement,&in[1],MAX_SCRIPT_STATEMENT_LEN-1);
	statement[MAX_SCRIPT_STATEMENT_LEN-1]=0;
	token = strtok(statement," ");

	BytesInSpecifiedPkt = 0;
	cs_array_siz = 0;
	while ((token = strtok(NULL," ")) != NULL)
	{
		switch(token[0])
		{	
			case '&':	// In case checksum field 
				if (OP_SEND == Operation){
					char *n1,*n2;
					n1 = token+1;
					n2 = strchr(token,'-');
					if (n2 == NULL)	{
						ScriptErrorMsgPrint("Expecting Checksum format"
								"&header_start-header_end");
					}
					n2[0] = 0;
					n2++;
					int hs = strtol(n1,&q,10);
					if (n1 ==  q){
						ScriptErrorMsgPrint(
						"'%s' is not an integer value",n1);
					}
					int he =  strtol(n2,&q,10);
					if (n2 == q){
						ScriptErrorMsgPrint(
						"'%s' is not an integer value",n2);
					}
					cs_arr[cs_array_siz].pos = BytesInSpecifiedPkt;
					cs_arr[cs_array_siz].start = hs;
					cs_arr[cs_array_siz].end = he;

					cs_array_siz++;

					SpecifiedPktMask[BytesInSpecifiedPkt] = MASK_CS;
					SpecifiedPkt[BytesInSpecifiedPkt++] = 0;
					SpecifiedPkt[BytesInSpecifiedPkt] = 0;
					SpecifiedPktMask[BytesInSpecifiedPkt] = MASK_CS;

					break;
				}
				VerboseStringPrint("Falling thorugh to the case '*', In receive mode we are not evaluating checksum");
					

			case '*':	// Do not compare
				if (OP_RECEIVE != Operation)
				{
					ScriptErrorMsgPrint(
					    "Invalid character '*'. It is "
					    "valid only in receive");
					return EDPAT_FAILED;
				}
				/* Set the byte as zero and mark in mask
				   as MASK_SKIP */
				SpecifiedPkt[BytesInSpecifiedPkt]  = 0;
				SpecifiedPktMask[BytesInSpecifiedPkt] =
						MASK_SKIP;
				break;


			case '?':	// copy from last received packet
				if (OP_SEND != Operation)
				{
					ScriptErrorMsgPrint(
					    "Invalid character '?'. It is "
					    "valid only in send packet");
					return EDPAT_FAILED;
				}
				// Copy the n value after ?
				byte = (unsigned char )
						strtol(&token[1],&q,10);
				if ( token == q)
				{
				    ScriptErrorMsgPrint(
					"'%s' is not a integer value",
					token);
				    return EDPAT_FAILED;
				};

				// Check if within limits
				if ( MAX_PKT_SIZE < BytesInSpecifiedPkt)
				{
				    ScriptErrorMsgPrint("Pkt too large");
				    return EDPAT_FAILED;
				};


				// Check for tail in the packet 
				if ((NULL != q) && (0 != q[0]))
				{
					ScriptErrorMsgPrint(
						"Invalid character "
						"at the end of '%s'",token);
					return EDPAT_FAILED;
				};

				//Set byte and mask
				SpecifiedPkt[BytesInSpecifiedPkt]  = 0;
				SpecifiedPktMask[BytesInSpecifiedPkt]=byte;
				break;
			default:
				//Default just copy the byte in
				byte = (unsigned char ) strtol(token,&q,16);
				if ( token == q)
				{
					ScriptErrorMsgPrint(
						"'%s' is not a hex value",
						token);
					return EDPAT_FAILED;
				};
				if ( MAX_PKT_SIZE < BytesInSpecifiedPkt)
				{
				       ScriptErrorMsgPrint("Pkt too large");
					return EDPAT_FAILED;
				};
				if ((NULL != q) && (0 != q[0]))
				{
					ScriptErrorMsgPrint(
						"Invalid character at the "
						"end of '%s'",token);
					return EDPAT_FAILED;
				};
				SpecifiedPkt[BytesInSpecifiedPkt]  = byte;
				SpecifiedPktMask[BytesInSpecifiedPkt] =
						MASK_EXACT;
		}
		BytesInSpecifiedPkt++;
	}

	if ( 0 >= BytesInSpecifiedPkt)
	{
		ScriptErrorMsgPrint("Zero byte packet is specified");
		return EDPAT_FAILED;
	}
	
	VerboseStringPrint("Read %d bytes from script for %s Eth Port '%s'",
			BytesInSpecifiedPkt,
			( (OP_RECEIVE == Operation) ?
				"receiving from" : "sending to "),
			EthPortName);
	VerbosePacketPrint(SpecifiedPkt, BytesInSpecifiedPkt);
	VerbosePacketMaskPrint(SpecifiedPktMask, BytesInSpecifiedPkt);

	return EDPAT_SUCCESS;
}

/**************************
 *
 * 	check_sum
 *
 * 	Used to calculate the IP checksum for bytes specified.
 * 	1's complement addition of 16 bit words
 *
 * 	Arguments	:	pkt	-	pointer to packet buffer
 * 				start	-	starting position of header
 * 				end	-	end position of header
 *
 * 	Return		:	16 bit check sum value
 *
 ***************************/

static short int check_sum(unsigned char *pkt,unsigned int start,unsigned int end)	{
	unsigned char *addr = pkt +start;
	if (start > end)	{
		ScriptErrorMsgPrint("start and end of header invalid");
		return EDPAT_FAILED;
	}
	unsigned int size = end - start;
	unsigned int csum = 0;
	unsigned int i = 0;
	// Add each word in the header (16 bit addition
	for (;i <= size;i += 2)	{
		// Making it 16 byte addition 
		csum += (addr[i] << 8) + addr[i+1]; 
	}
	//In the case that there are odd number of packets pad with zero byte
	if (size % 2 == 0)	{	
		csum += (addr[i]<<8) + 0x00;
	}
	// get rid of carry by folding 
	
	while (csum >> 16)	{
		csum = (csum & 0xFFFF) + (csum>>16);
	}

	unsigned short int ret = ~csum;
	VerboseStringPrint(
		"Calculated CheckSum for bytes [%d-%d] and got :: %x",
		start,end,ret);
	return ret;
}

/*****************************
 *
 *	packetSend
 *
 *	When Operation specified is OP_SEND, it send the specified packet to the 
 *	
 *	Arguments	:	void	 
 *
 *	Return 		:	EDPAT_RETVAL
 *
 *
 * ***************************/


static EDPAT_RETVAL packetSend(void)
{
	static unsigned char pkt[MAX_PKT_SIZE];
	int pktLen;
	EDPAT_RETVAL retVal;

	for(pktLen=0; pktLen < BytesInSpecifiedPkt; pktLen++)
	{
		// If it is the eaxact case with no special character
		if (MASK_EXACT == SpecifiedPktMask[pktLen])
		{
			pkt[pktLen] = SpecifiedPkt[pktLen];
		}
		//Special character usage 
		

		// For the case that the field is checksum 

		else
		{
			/* In ? <n> case mask is greater than 0 and
			  represents postion in previous received packet */
			if ( 0 <= SpecifiedPktMask[pktLen])
			{
				/* Check whether n is within bounds of the
				   previous packet */
				if (BytesInRecvPkt > SpecifiedPktMask[pktLen])
				{
					pkt[pktLen] =
					  RecvPkt[SpecifiedPktMask[pktLen]];
				}
				else
				{
					ScriptErrorMsgPrint(
						"Copy position %d is beyond"
						" received bytes of %d.",
						SpecifiedPktMask[pktLen],
						BytesInRecvPkt);
					return EDPAT_FAILED;
				}

			}


			else if (SpecifiedPktMask[pktLen] == MASK_CS){
				VerboseStringPrint(
					"Encountered Checksum at %d",
					pktLen);
				continue;
			}

			else 
			{
				ScriptErrorMsgPrint("Unsupported mask");
				return EDPAT_FAILED;
			}
		}
	}

	/* Now that the full packet is formed compute check sum and fill
	   in various positions */
	for (int i = 0;i < cs_array_siz;i++){
		VerboseStringPrint("Found checksum at %d",cs_arr[i].pos);
		VerboseStringPrint("For %d",cs_arr[i].start,cs_arr[i].end);
	}

	for (int i = 0;i < cs_array_siz;i++){
		short int cs = check_sum(pkt,cs_arr[i].start,cs_arr[i].end);
		char byte1 = (char)((cs >> 8) & 0x00FF);
		char byte2 = (char)(cs & 0x00FF);
		pkt[cs_arr[i].pos] 	= byte1;
		pkt[cs_arr[i].pos + 1] 	= byte2;
	}

	//  Send the packet
	retVal = EthPortSend(EthPortName, pkt, pktLen);

	return retVal;
}


/*********************************
 *
 *	packetReceive 
 *
 * 	Receives packets and checks according to the current testcase
 *	statement
 *	Arguments	:	void
 *	Return 		: 	void
 *
 *
 * ******************************/

static EDPAT_RETVAL packetReceive(void)
{
	int pktLen,i;
	EDPAT_RETVAL retVal;

	BytesInRecvPkt = sizeof(RecvPkt);
	
	//	Wait to receive packet from ethport for given waitperiod
	retVal = EthPortReceive(EthPortName, RecvPkt, &BytesInRecvPkt);
	if (EDPAT_SUCCESS != retVal)
	{
		if (EDPAT_NOTFOUND == retVal)
		{
			CurrentTestResult = EDPAT_TEST_RESULT_FAILED;
			TestCaseStringPrint("Timeout waiting for Packet "
				"at '%s'",EthPortName);

			TestCaseStringPrint( "Expected packet. Len=%d",
						BytesInSpecifiedPkt);
			TestCasePacketPrint(SpecifiedPkt,
					BytesInSpecifiedPkt);
			return EDPAT_SUCCESS;
		}
		return EDPAT_FAILED;
	}

	// Check if length is atleast as much as specified
	if ( BytesInSpecifiedPkt > BytesInRecvPkt)
	{
		pktLen = BytesInRecvPkt;
	}

	// If its more truncate the rest since there maybe padding
	else
	{
		pktLen = BytesInSpecifiedPkt;
	}

	for(i=0; i < pktLen; i++)
	{
		switch(SpecifiedPktMask[i])
		{
			case MASK_SKIP:
				break;
			case MASK_EXACT:
				if (RecvPkt[i] != SpecifiedPkt[i])
				{
					CurrentTestResult =
						EDPAT_TEST_RESULT_FAILED;
					TestCaseStringPrint(
						"Missmatch at byte %d of "
						"Packet received on "
						"Eth Port'%s'",
						i,EthPortName);
					TestCaseStringPrint(
						"Expected packet. Len=%d",
						BytesInSpecifiedPkt);
					TestCasePacketPrint(SpecifiedPkt,
						BytesInSpecifiedPkt);
					TestCaseStringPrint(
						"Actual packet received. "
						" Len=%d",
						BytesInRecvPkt);
					TestCasePacketPrint(RecvPkt,
						BytesInRecvPkt);
					return EDPAT_SUCCESS;
				}
				break;
			default:
					return EDPAT_FAILED;
				break;
		}
	}

	/* Padding is used to make it a minimum size of  60 it its more
	   than that, the packet is just wrong and not bigger than
	   ecpected due to padding */
	if ( (60 < BytesInRecvPkt ) &&
		(BytesInSpecifiedPkt != BytesInRecvPkt))
	{
		CurrentTestResult = EDPAT_TEST_RESULT_FAILED;
		TestCaseStringPrint("Number of bytes received at '%s' "
			"does not match. Epected=%d, actual=%d",
			EthPortName,
			BytesInSpecifiedPkt,
			BytesInRecvPkt);
		TestCaseStringPrint("Expected packet. Len=%d",
			BytesInSpecifiedPkt);
		TestCasePacketPrint(SpecifiedPkt, BytesInSpecifiedPkt);
		TestCaseStringPrint("Actual packet received. Len=%d",
			BytesInRecvPkt);
		TestCasePacketPrint(RecvPkt,BytesInRecvPkt);
		return EDPAT_SUCCESS;
	}
	BytesInRecvPkt = pktLen;

	return EDPAT_SUCCESS;
}

/*************************
 *
 *	PacketProcess
 *
 *	Processes each packet test case calling receive or send based on
 *	operation, calls packetRead to process 
 *	Special chars in packet and set up the ethernet ports required
 *
 *	Arguments:	in -	the testcase under consideration 
 *
 *	Return 		 EDPAT_RETVAL
 *
 *************************/

EDPAT_RETVAL PacketProcess(char *in)
{
	EDPAT_RETVAL retVal;

	retVal = packetRead(in);

	if( EDPAT_SUCCESS != retVal)
	{
		return EDPAT_FAILED;
	}

	switch(Operation)
	{
		case OP_SEND:
			retVal = packetSend();
			break;
		case OP_RECEIVE:
			retVal = packetReceive();
			break;
		default:
			ScriptErrorMsgPrint(
				"Zero byte packet is specified");
			retVal = EDPAT_FAILED;
	}
	return retVal;
}
