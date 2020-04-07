# Sample testcase scripts for EDpAT

I have included a set of simple tesscripts to show how the testscripts work for the different protocols that are supported by EDpAT along with walkthroughs for how to set them up. Before running them you need to set some variables depending on the network configuration of your system .
#Setup

## 1. Find the details of your interfaces
1. run the `ip addr show` command, you will get a list of the different interfaces and the details linked to each one
	1. note down the interface names, IP addresses and MAC addresses with each interface you will need them to create the sample packets
1. make sure you are connected to an external network via wifi or ethernet
1. run the `arp -v` command to get entries of the arp table, with this you can find the default gateway and the interface that is connected to it

## 2. The ethernet frame format
1. The following is the frame structure of the typical ethernet II frame in the data-link layer

MAC Destination address | MAC Source address | Ethertype (length in IEEE 802.3) | Payload 
------|----------|----------|-------------	
6 bytes | 6 bytes | 2 bytes | 46 - 1500 bytes 
1. The MAC dest and MAC src addresses should be filled in depending on the values that you get from step 1.
1. Ethertype has to be filled based on what protocol you plan to use, few important values are
	1. IPv4	- 0x0800
	2. ARP  - 0x0806
	3. RARP - 0x8035
 
