# Sample testcase scripts for EDpAT

included below are a set of simple testscripts to show how the testscripts work for the different protocols 
that are supported by EDpAT along with walkthroughs for how to set them up. Before running them you need to 
set some variables depending on the network configuration of your system .
# Setup

## 1. Find the details of your interfaces
1. run the `ip addr show` command, you will get a list of the different interfaces and the details linked to each one
	1. note down the interface names, IP addresses and MAC addresses with each interface you will need 
them to create the sample packets
1. make sure you are connected to an external network via wifi or ethernet
1. run the `arp -v` command to get entries of the arp table, with this you can find the default gateway and the interface that is connected to it
1. Use these details to update system specific information in the include file `addr.edpat`

## 2. The ethernet frame format
1. The following is the frame structure of the typical ethernet II frame in the data-link layer

MAC Destination address | MAC Source address | Ethertype (length in IEEE 802.3) | Payload 
------|----------|----------|-------------	
6 bytes | 6 bytes | 2 bytes | 46 - 1500 bytes 
1. The MAC dest and MAC src addresses should be filled in depending on the values that you get from step 1.
1. Ethertype has to be filled based on what protocol you plan to use, few important values are
	1. IPv4	- 0x0800
	1. ARP  - 0x0806
	1. RARP - 0x8035
	1. The entire list can be found at [Ethertype numbers](https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml)

## 3. include address file
1.  To use the sample programs provided, change the values of addresses in this file to suit your system 
configuration, it is included in the sample files where all the fields other than the address are already 
defined and ready
## 1. ARP
1. The file `arp.edpat` in `sample_tests` can be run using `sudo ./edpat.exe ./sample_tests/arp.txt`. 
1. Set the required values in header.txt to send an ARP request packet to your default gateway and 
check whether you are receiving the required response which is an ARP Reply with the MAC address of the requested IP
1.  Refer [ARP payload](https://tools.ietf.org/html/rfc6747) to make custom packets.

## 2. ICMP
1. The file `echo.edpat` in `sample_tests` contains a sample ICMP echo request testscript
1. Notice that the checksum calculation functionality has been used here for the IP and ICMP header
1. Refer [ICMP payload](https://tools.ietf.org/html/rfc6747) to make custom packets

 
