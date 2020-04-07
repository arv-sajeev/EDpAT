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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <mqueue.h>
#include "edpat.h"
#include "print.h"

#define MAC_ADDR_LEN	6

typedef struct {
	char		portName[MAX_ETH_PORT_NAME_LEN];
	char		mqName[MAX_FILE_NAME_LEN];
	unsigned char	macAddr[MAC_ADDR_LEN];
	int		ethPortSocketFd;
	int		ifIndex;
	mqd_t		mqRcvFd;
	mqd_t		mqSendFd;
	pthread_t	pThread;
} ETH_PORT_INFO;

static ETH_PORT_INFO EthPortInfoTable[MAX_ETH_PORT_COUNT];
static int EthPortCount = 0;
static EDPAT_BOOL ArrayInitFlag = EDPAT_FALSE;
static int MqMsgSize = 0;


/***********************
 *
 *   ethPortIdxFindByName()
 *
 *   Get array index of the table for the given EthPort Name
 *
 *   Arguments 		: 	portName	- name of port whose index is to be found
 *			
 *   Return		:	if >= 0 Array index or EDPAT_NOTFOUND
 *
 ********/
static EDPAT_RETVAL ethPortIdxFindByName(const char *portName)
{
	int i;
	for (i=0; i < EthPortCount; i++)
	{
		if (0 == strcmp(portName,EthPortInfoTable[i].portName))
		{
			return i;
		}
	}
	return EDPAT_NOTFOUND;
}


/***********************
 *   ethPortClose()
 *
 *   Close the port.
 *
 *   Arguments : 
 *	p	-	INPUT. pointer to port info table record.
 *		
 *   Return:	-	None
 *
 ********/
static void ethPortClose(ETH_PORT_INFO *p)
{
	if ((-1) != p->pThread)
	{
		pthread_cancel(p->pThread);
		p->pThread = (-1);
		VerboseStringPrint(
			"Receiver thread stopped for %s", p->portName);
	}
	if ( 0 <= p->ethPortSocketFd)
	{
		close(p->ethPortSocketFd);
		p->ethPortSocketFd = (-1);
		VerboseStringPrint("Ethernet Port '%s' closed", p->portName);
	}
	if ( 0 <= p->mqRcvFd)
	{
		mq_close(p->mqRcvFd);
		mq_unlink(p->mqName);
		p->mqRcvFd = (-1);
	}
	if ( 0 <= p->mqSendFd)
	{
		mq_close(p->mqSendFd);
		p->mqSendFd = (-1);
	}

	p->portName[0] = 0;
	p->mqName[0] = 0;
	p->ifIndex = (-1);
	return ;
}



/***********************
 *   isPacketToBeFiltered()
 *
 *   Inspect a receved packet and check whether it need to be filtered
 *
 *   Arguments : 
 *	pkt	- INPUT. pointer to the packet buffer.
 *	pktLen	- INPUT. leghth of packet.
 *
 *   Return:
 *	EDPAT_TRUE	- packet need to be filtered/discarded
 *	EDPAT_FALSE	- packet should not be filtered
 *
 ********/
static EDPAT_BOOL isPacketToBeFiltered(unsigned char *pkt,int pktLen)
{
	unsigned short ethType;
	unsigned short srcPort;
	unsigned short dstPort;

	if (EDPAT_TRUE != EnableBroadcastPacketFiltering)
	{
		// filtering disabled. Do not do anything.
		return EDPAT_FALSE;
	}

	ethType = ((pkt[12] << 8) | pkt[13]);
	switch(ethType)
	{
		case 0x0806:	// ARP
			VerboseStringPrint(
				"Received ARP packet filtered");
			return EDPAT_TRUE;
		case 0x88CC:	//LLDP
			VerboseStringPrint(
				"Received LLDP packet filtered");
			return EDPAT_TRUE;
		case 0x0800:	// IPv4
			switch(pkt[23]) //based on protocol
			{
			   case 0x02:	// IGMP
				VerboseStringPrint(
					    "Received IGMP packet filtered");
				return EDPAT_TRUE;
			   case 0x11:	// UDP
				srcPort = ((pkt[34] << 8) | pkt[35]);	
				dstPort = ((pkt[36] << 8) | pkt[37]);
				switch(srcPort)
				{
				   case 0x0043:	// DHCP
				   case 0x0044:	// DHCP
				      VerboseStringPrint(
					"Received DHCP packet filtered");
				      return EDPAT_TRUE;
				   case 0xd7e8:	// SSDP
				   case 0x076c:	// SSDP
				      VerboseStringPrint(
					"Received SSDP packet filtered");
				      return EDPAT_TRUE;
				   case 0x14e9:	// MDNS
				      VerboseStringPrint(
					"Received MDNS packet filtered");
				      return EDPAT_TRUE;
				}
				switch(dstPort)
				{
				   case 0x0043:	// DHCP
				   case 0x0044:	// DHCP
				      VerboseStringPrint(
					"Received DHCP packet filtered");
				return EDPAT_TRUE;
				   case 0xd7e8:	// SSDP
				   case 0x076c:	// SSDP
				      VerboseStringPrint(
					"Received SSDP packet filtered");
				return EDPAT_TRUE;
				   case 0x14e9:	// MDNS
				      VerboseStringPrint(
					"Received MDNS packet filtered");
				return EDPAT_TRUE;
			   	}
			}
			break;
		case 0x86dd:	// IPv6
			VerboseStringPrint("Received IPv6 packet filtered");
			return EDPAT_TRUE;
		
	}
	return EDPAT_FALSE;
}


/***********************
 *   ReceiverPthreadFunc()
 *
 *   This function will run as a child thread. It will read any packet
 *   receved on the eth port and put it in message queue so that the
 *   message will not be lost.
 *
 *   Arguments : 
 *	vargp	-  INPUT. Point to the port into table.
 *
 *   Return:	- None
 *
 ********/
static void *ReceiverPthreadFunc(void *vargp)
{
	ETH_PORT_INFO *p = (ETH_PORT_INFO *) vargp;
	unsigned char	pkt[MAX_PKT_SIZE];
	size_t	pktLen = 10;
	EDPAT_RETVAL	retVal;
	int	lastEthErrno = 0;
	int	lastMqErrno = 0;
	struct sockaddr_ll addr={0};
	socklen_t	addrLen;

	addr.sll_family=AF_PACKET;
	addr.sll_ifindex=p->ifIndex;
	addr.sll_halen=ETHER_ADDR_LEN;
	addr.sll_protocol=htons(ETH_P_ALL);
	memcpy(addr.sll_addr,pkt,ETHER_ADDR_LEN);


	while(1) // do for ever
	{
		addrLen = sizeof(addr);
		// Receive packet formo given port
		pktLen = recvfrom(p->ethPortSocketFd,
				pkt,sizeof(pkt),
                		0,
				(struct sockaddr*)&addr,&addrLen);

		if (0 > pktLen)
		{
			if (errno != lastEthErrno)
			{
				// print the error message only once
				ExecErrorMsgPrint("recvfrom() failed "
					"while receiving packet from '%s'",
					p->portName);
				lastEthErrno = errno;
			}
			continue;
		}

		if (0 == pktLen)
		{
			VerboseStringPrint("Empty packet receieved");
			continue;
		}
		// If Promiscuous drop packets not addressed to the MAC of the ethernet port
		if ((PromiscuousModeEnabled) && 0 != memcmp(pkt,p->macAddr,MAC_ADDR_LEN) )
		{
			VerboseStringPrint("Packet not addressed to '%s' "
						"is discarded",p->portName);
			continue;
		}
		
		// Drop packets if it is to be filtered

		retVal = isPacketToBeFiltered(pkt,pktLen);
		if (EDPAT_TRUE == retVal)
		{
			// discard the packt as it needs to be fintered.

			continue;
		}
		
		//  Enqueue the packt to the message queue
		retVal = mq_send(p->mqSendFd,pkt,pktLen,0);
		if ( 0 > retVal)
		{
			if (errno != lastMqErrno)
			{
				// print the error message only once
				ExecErrorMsgPrint("mq_send() failed "
					"while sending packet to '%s'",
					p->mqName);
				lastMqErrno = errno;
			}
		}

	}
}


/***********************
 *   EthPortOpen()
 *
 *   Open the specifed ethernet port. It will open the eth port.
 *   In addtion it will open the message queue and start the recever thead
 *   to receve incomming packets.
 *
 *   Arguments : 
 *	portName	- INPUT.  Name of the port to be opned.
 *
 *					$<varName>=<varVal>
 *   Return:		
 *	>= 0	- port opened sucessfully. The return value is the index
 *		  to port into array
 *	< 0	- failed to open port.
 *
 *********************************/

int EthPortOpen(const char *portName)
{
	int socketOpt;
	struct ifreq portOpts;
	size_t portNameLen;
	char mqName[MAX_FILE_NAME_LEN];
	int portNameLenAllowed;
	EDPAT_RETVAL retVal;
	int portIdx;
	struct mq_attr mqAttr;
	
	// Just do this the first time clear te full table
	if (EDPAT_FALSE == ArrayInitFlag)
	{
		for (portIdx=0; portIdx < MAX_ETH_PORT_COUNT; portIdx++)
		{
			EthPortInfoTable[portIdx].portName[0] = 0;
			EthPortInfoTable[portIdx].mqName[0] = 0;
			EthPortInfoTable[portIdx].ethPortSocketFd = -1;
			EthPortInfoTable[portIdx].ifIndex = -1;
			EthPortInfoTable[portIdx].mqRcvFd = -1;
			EthPortInfoTable[portIdx].mqSendFd = -1;
			EthPortInfoTable[portIdx].pThread = -1;
		}
		ArrayInitFlag = EDPAT_TRUE;
		VerboseStringPrint("Initialized Ethernet port data structures");
	}

	VerboseStringPrint("Opening Ethernet port '%s'",portName);
	/* Check the interface is already opened */
	portIdx = ethPortIdxFindByName(portName);

	if (EDPAT_NOTFOUND != portIdx)
	{
		VerboseStringPrint("Interface '%s'  was aleady opened",
			portName);
		return portIdx;
	}
	// Save name
	strcpy(EthPortInfoTable[EthPortCount].portName,portName);
	// Stoer socketfd
	EthPortInfoTable[EthPortCount].ethPortSocketFd =
			socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
	if (0 > EthPortInfoTable[EthPortCount].ethPortSocketFd)
	{
		ExecErrorMsgPrint("socket(%s) failed. "
			"Re-execute with root privilage",portName);
		return	-1;
	}
	
	VerboseStringPrint("Socket opened for outgoing packets of '%s'",
			portName);
	/* Get ethernet port index number */
	portNameLen=strlen(portName);

	// Lowest is the allowed interface name length
	portNameLenAllowed = sizeof(portOpts.ifr_name);
	if (portNameLenAllowed > MAX_ETH_PORT_NAME_LEN)
		portNameLenAllowed = MAX_ETH_PORT_NAME_LEN;

	if (	portNameLen > portNameLenAllowed )
	{
		ScriptErrorMsgPrint("Ethernet Port name '%s' too long. "
					"Max allowed is %d",
					portName,portNameLenAllowed);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}
	strcpy(portOpts.ifr_name,portName);

	/* Get the port details using the ioctl interface */
        if (0 > ioctl(EthPortInfoTable[EthPortCount].ethPortSocketFd,
			SIOCGIFHWADDR,&portOpts))
	{
		ExecErrorMsgPrint("ioctl(SIOCGIFHWADDR) failed "
				"for Eth Port '%s'",
				portName);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}

	// It is ARP protocol hardware
	if (portOpts.ifr_hwaddr.sa_family!=ARPHRD_ETHER)
	{
		ExecErrorMsgPrint("Interface in not  Ethernet. "
				"%d is reported as interface type",
				portOpts.ifr_hwaddr.sa_family);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}
	
	// store the MAC we got to the table
	memcpy(EthPortInfoTable[EthPortCount].macAddr,
		portOpts.ifr_hwaddr.sa_data,MAC_ADDR_LEN);
	VerboseStringPrint("MAC address of '%s' is "
		"%02X:%02X:%02X:%02X:%02X:%02X",
		EthPortInfoTable[EthPortCount].portName,
		EthPortInfoTable[EthPortCount].macAddr[0],
		EthPortInfoTable[EthPortCount].macAddr[1],
		EthPortInfoTable[EthPortCount].macAddr[2],
		EthPortInfoTable[EthPortCount].macAddr[3],
		EthPortInfoTable[EthPortCount].macAddr[4],
		EthPortInfoTable[EthPortCount].macAddr[5]);
/*
		(unsigned int) EthPortInfoTable[EthPortCount].macAddr[0],
		(unsigned int) EthPortInfoTable[EthPortCount].macAddr[1],
		(unsigned int) EthPortInfoTable[EthPortCount].macAddr[2],
		(unsigned int) EthPortInfoTable[EthPortCount].macAddr[3],
		(unsigned int) EthPortInfoTable[EthPortCount].macAddr[4],
		(unsigned int) EthPortInfoTable[EthPortCount].macAddr[5]);
*/
	// Get the interface index using ioctl
        if (0 > ioctl(EthPortInfoTable[EthPortCount].ethPortSocketFd,
			SIOCGIFINDEX,&portOpts))
	{
		ExecErrorMsgPrint("ioctl(SIOCGIFINDEX) failed "
				"for Eth Port '%s'",
				portName);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}
	EthPortInfoTable[EthPortCount].ifIndex = portOpts.ifr_ifindex;

	VerboseStringPrint("Interface Index of '%s' is %d", portName,
		EthPortInfoTable[EthPortCount].ifIndex);
	/* Set interface to promiscuous mode */
	strcpy(portOpts.ifr_name, portName);
	if (0 > ioctl(EthPortInfoTable[EthPortCount].ethPortSocketFd,
			SIOCGIFFLAGS, &portOpts))
	{
		ExecErrorMsgPrint("ioctl(SIOCGIFFLAGS) failed "
			"for Eth Port '%s'", portName);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}

	if (EDPAT_TRUE ==PromiscuousModeEnabled)
	{
		portOpts.ifr_flags |= IFF_PROMISC;
		VerboseStringPrint("Enabled promiscuous mode for '%s'",
			portName);
	}
	else
	{
		portOpts.ifr_flags &= ~IFF_PROMISC;
		VerboseStringPrint("Disabled promiscuous mode for '%s'",
			portName);
	}
	if (0 > ioctl(EthPortInfoTable[EthPortCount].ethPortSocketFd,
	SIOCSIFFLAGS, &portOpts))
	{
		ExecErrorMsgPrint("ioctl(SIOCSIFFLAGS) failed for "
			"Eth Port '%s'",portName);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}

	/* Allow the socket to be reused,
	   incase connection is closed prematurely */
	if (0 > setsockopt(EthPortInfoTable[EthPortCount].ethPortSocketFd,
			SOL_SOCKET, SO_REUSEADDR, &socketOpt, sizeof socketOpt))
	{
		ExecErrorMsgPrint("setsockopt(SO_REUSEADDR) failed for "
			"Eth Port '%s'",portName);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}

	VerboseStringPrint("Socket Re-use option set for '%s'",portName);
        /* Bind to device */
        if (0 > setsockopt(EthPortInfoTable[EthPortCount].ethPortSocketFd,
		SOL_SOCKET,
                SO_BINDTODEVICE, portName, IFNAMSIZ-1))
        {
		ScriptErrorMsgPrint("Ethernet Port name '%s' does not exists",
			portName);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
        }

	VerboseStringPrint("Socket binding successful for '%s'",portName);


	//Open Message queue for communcation between reciever thread and ethportread/receive
	//First open the receive FD from which teh packets are dequeued from MQ
	sprintf(mqName,"/mq.%s.%d",portName,getpid());
	strcpy(EthPortInfoTable[EthPortCount].mqName,mqName);

	EthPortInfoTable[EthPortCount].mqRcvFd =
		mq_open(mqName, O_CREAT | O_RDONLY, 0644, NULL);
	if ( 0 > EthPortInfoTable[EthPortCount].mqRcvFd)
	{
		ExecErrorMsgPrint("mq_open(%s) failed",mqName);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}
	VerboseStringPrint(
		"Mq opened for de-queuing incoming packets from '%s'.",
		portName);

	/* Find the Message queue size. 
	   Configired at system level in /proc/sys/fs/mqueue/msgsize_max
	   or per message queue using mq_setattr(). Default value is 8192*/
	if (0 == MqMsgSize)
	{
		retVal = mq_getattr(EthPortInfoTable[EthPortCount].mqRcvFd,
					&mqAttr);
		if (0 > retVal)
		{
			ExecErrorMsgPrint("mq_getattr(%s) failed",mqName);
			ethPortClose(&EthPortInfoTable[EthPortCount]);
			return -1;
		}
		MqMsgSize = mqAttr.mq_msgsize;
		VerboseStringPrint("Mq msgsize is detected as  %d",MqMsgSize);
	}

	// Open the send FD thoruggh which the receiver thread loads packes to the MQ
	EthPortInfoTable[EthPortCount].mqSendFd =
			mq_open(mqName, O_WRONLY|O_NONBLOCK);
	if ( 0 > EthPortInfoTable[EthPortCount].mqSendFd)
	{
		ExecErrorMsgPrint("mq_open(%s) failed",mqName);
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}
	VerboseStringPrint(
		"Mq opened for queuing incoming packets from '%s'",
		portName);

	// Start a receiving thread for each PORT
	retVal  = pthread_create( &EthPortInfoTable[EthPortCount].pThread,
			NULL,
			ReceiverPthreadFunc,
			&EthPortInfoTable[EthPortCount]);

	if ( 0 > retVal)
	{
		ExecErrorMsgPrint("pthread() failed");
		ethPortClose(&EthPortInfoTable[EthPortCount]);
		return -1;
	}
	VerboseStringPrint("Ethernet Port '%s' opened successfully",portName);
	EthPortCount++;

	return (EthPortCount-1);
}

/***********************
 *   EthPortCloseAll()
 *
 *   Closs all open ports.
 *
 *   Arguments : None
 *
 *   Return:	- EDPAT_SUCESS or EDPAT_FAULED
 *
 ********/
EDPAT_RETVAL EthPortCloseAll(void)
{
	int i;
	if (EDPAT_FALSE == ArrayInitFlag)
	{
		// Nothing to cleanup.
		return EDPAT_SUCCESS;
	}
	for(i=0; i < EthPortCount; i++)
	{
		ethPortClose(&EthPortInfoTable[i]);
	}
	return EDPAT_SUCCESS;
}


/***********************
 *   ethPortRead()
 *
 *   Read a packet from the MQ associated to the Ethport that the receiver thread had filled
 *
 *   Arguments : 
 *	SocketFd	- INPUT. Index to the portInfo table from where
 *			  Pkt to be read.
 *      data		- INPUT/OUTPU. pointer to buffer where read data
 *			  need to be stored. The receved packet is returned
 *			  to the coller in this buffer.
 *	dataLen		- INPUT/OUTPUT. caller specify the size of'data'
 *			  the function will retirn the size of packet returned
 *	waitTime	- INPUT. How long to wait for packet befor timeout.
 *			  specified in seconds.
 *
 *   Return:	- EDPAT_SUCESS or EDPAT_FAULED
 *
 ***********************/
static EDPAT_RETVAL ethPortRead( const int SocketFd,
			unsigned char *data, int *dataLen, const int waitTime)
{
	struct timespec readTime;
	int 	rcvDataLen;

	if ((*dataLen) < MqMsgSize)
	{
		ExecErrorMsgPrint("MQ Receive buffer size too small(%d). "
			"Should be greater than %d which is configured in"
			" /proc/sys/fs/mqueue/msgsize_max",
			(*dataLen),MqMsgSize);
		return EDPAT_FAILED;
	}
	clock_gettime(CLOCK_REALTIME, &readTime);
	readTime.tv_sec += waitTime;

	// Wait till waittime to receive packet
	rcvDataLen = mq_timedreceive(SocketFd,
			data, *dataLen, NULL,
			&readTime);
	if (0 > rcvDataLen)
	{
		if (ETIMEDOUT == errno)
		{
			return EDPAT_NOTFOUND;
		}
		ExecErrorMsgPrint("pthread() failed");
		return EDPAT_FAILED;
	}
	*dataLen = rcvDataLen;

	return EDPAT_SUCCESS;
}

/*****************************
 *
 * 	EthPortReceive
 *
 * 	Read the packets in the MQ and load it into data
 *
 * 	Arguments	:	portName	-	Name of the port to receive packets form 
 * 				data		- 	the data that is read from the mq is filled here
 * 				datalen		- 	Length of data filled
 * 	Return 		: 	EDPAT_RETVAL
 *
 * ***************************/

EDPAT_RETVAL EthPortReceive(const char *portName, unsigned char *data, int *dataLen)
{
	int portIdx;
	EDPAT_RETVAL retVal;

	portIdx = ethPortIdxFindByName(portName);

	if (0 > portIdx)
	{
		ScriptErrorMsgPrint(
			"Trying to read from '%s' which is not yet opened",
			portName);
		return EDPAT_NOTFOUND;
	}

	retVal = ethPortRead(EthPortInfoTable[portIdx].mqRcvFd,
			data,dataLen,PacketReceiveTimeout);
	if (EDPAT_SUCCESS == retVal)
	{
		VerboseStringPrint("Packet of length %d "
					"received at port '%s'",
					*dataLen, portName);
		VerbosePacketHeaderPrint(data);
		VerbosePacketPrint(data,*dataLen);
	}
	return retVal;

}

/******************************
 *
 *	EthPortReceiveAny
 *
 *	Read from all the ports we have stored in EthPortInfoTable, this is used to find if even one extra unwanted packet is in the queue 
 *	
 *	Arguments	: 	portname	-	it is used to writeback the portname that received data
 *				data		- 	it is used to vriteback the data received
 *				datalen		-	it is used to writeback the datalen of dat received
 *
 *	Return 		: 	EDPAT_RETVAL
 *
 ******************************/

EDPAT_RETVAL EthPortReceiveAny(char *portName, unsigned char *data, int *dataLen)
{
	int portIdx;
	EDPAT_RETVAL retVal;

	for (portIdx=0; portIdx < EthPortCount; portIdx++)
	{
		retVal = ethPortRead(EthPortInfoTable[portIdx].mqRcvFd,
					data,dataLen,0);
		switch(retVal)
		{
			case EDPAT_SUCCESS:
				strcpy(portName,
					EthPortInfoTable[portIdx].portName);
				VerboseStringPrint(
					"Packet of length %d "
					"received at port '%s'",
					*dataLen, portName);
				VerbosePacketHeaderPrint(data);
				VerbosePacketPrint(data,*dataLen);
				return EDPAT_SUCCESS;
			case EDPAT_NOTFOUND:
				break;
			default:
				return EDPAT_FAILED;
		}
	}
	return EDPAT_NOTFOUND;
}

/*************************
 *
 *	EthPortSend
 *
 *	Used to send data to the specified port
 *
 *	Arguments	:	portName	-	the name of the port for packets to be send to 
 *				data		-	the data to be sent to the port
 *				dataLen		-	the length of data to be sent
 *
 *	return 		: 	EDPAT_RETVAL
 *
 *
 *************************/

EDPAT_RETVAL EthPortSend(const char *portName, unsigned char *data, const int dataLen)
{
	struct sockaddr_ll addr={0};
	EDPAT_RETVAL retVal;
	int portIdx;

	portIdx = ethPortIdxFindByName(portName);

	if (0 > portIdx)
	{
		ScriptErrorMsgPrint(
			"Trying to send to '%s' which is not yet opened",
			portName);
		return EDPAT_NOTFOUND;
	}

	// A mimimum of 1 bytes is needed
	if (14 > dataLen)
	{
		ScriptErrorMsgPrint(
			"Insufficent Bytes in send packet. "
			"Minimum 14 is needed");
		return EDPAT_NOTFOUND;
	}

	addr.sll_family=AF_PACKET;
	addr.sll_ifindex=EthPortInfoTable[portIdx].ifIndex;
	addr.sll_halen=ETHER_ADDR_LEN;
	addr.sll_protocol=htons(ETH_P_ALL);
	memcpy(addr.sll_addr,data,ETHER_ADDR_LEN);

	retVal = sendto(EthPortInfoTable[portIdx].ethPortSocketFd,
				data,dataLen,
				0,(struct sockaddr*)&addr,sizeof(addr));
	if ( 0 > retVal)
	{
		ExecErrorMsgPrint("sendto(%s) failed",portName);
		return EDPAT_FAILED;
	}

	VerboseStringPrint("Packet of length %d send successfuly at port '%s'",
			dataLen,portName);
	VerbosePacketHeaderPrint(data);
	VerbosePacketPrint(data,dataLen);
	return EDPAT_SUCCESS;
}

/****************************
 * 	EthPortClearBuf
 *
 * 	This is used to cleanup the memory queue after the testcases have been run
 * 	
 * 	Arguments	:	void
 * 	Return		: 	void
 *
 ****************************/
EDPAT_RETVAL EthPortClearBuf(void)
{
	unsigned char pkt[MAX_PKT_SIZE];
	char portName[MAX_ETH_PORT_NAME_LEN];
	int pktLen;
	EDPAT_RETVAL retVal;
	// Keep doing receive any as long as there are packets in the queue
	do {
		pktLen = MAX_PKT_SIZE;
		retVal = EthPortReceiveAny(portName, pkt, &pktLen);
	} while(EDPAT_SUCCESS == retVal);
	VerboseStringPrint("All receive buffers cleared");
	return retVal;
}
