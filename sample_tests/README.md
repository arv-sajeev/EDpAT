# Sample testcase scripts for EDpAT

I have included a set of simple tesscripts to show how the testscripts work for the different protocols that are supported by EDpAT along with walkthroughs for how to set them up. Before running them you need to set some variables depending on the network configuration of your system .
#Setup

## 1. Find the details of your interfaces
1. run the `ip addr show` command, you will get a list of the different interfaces and the details linked to each one
	1. note down the IP addresses and MAC with each interface you will need them to fill packets
1. make sure you are connected to an external network via wifi or ethernet
1. run the `arp -v` command to get entries of the arp table, with this you can find the default gateway and the interface that is connected to it
