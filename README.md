# 1. EDpAT
EDpAT stands for Ethernet Dataplane Application Tester. It is a test tool to test ethernet based data plane 
applications. It can send and receive raw Ethernet packets to one or more ethernet ports. 
The packets to be sent/received are specified in an input script file in the desired order.
The tool will read this file as input and send/receive the packets as specified in the file. 
The received packets are compared against what is specified in the script file to determine the the result of each testcase

# 2. Installation 
The Makefile has been included in this repository. Just run `make` in the working directory

# 3. Usage
 
         edpat.exe [-f] [-h] [-s] [-t] [-v] [-w <waittimeout>] <script> [<logfile> [<reportfile>]]

Parameter | Description
----------|------------
`<script>` | The input file which contains the test case specifcation. This is a mandatory argument, the syntax of the file is discussed in Section 3
`<logfile>` | If specified all the test execution logs will be written to this file, else will be written to stdout.
`<reportfile>` | if specified all the test results of each testcase with testcase ID and result will be written here, else will be written to stdout.
`-f`  | Don't filter broadcast packets, All ARP, LLDP, IGMP, ICMP, DHCP, SSDP and MDNS packets are discarded by default, this flag disables filtering.
`-h` | Help. Display usage information
`-s` | Perform only syntax checking of the Input script file without executing the testcases.
`-t` | Enable timestamping of entries in the logfile 
`-v` | Enable verbose mode in the logfile
`-w` | Timeout period while waiting for receiving a packet specified in the test script, `<waittimeout>` is specified in seconds and default value is 3 seconds.

# 4. Input file syntax
  * An input file can contain multiple test cases
  * A test case can consist of multiple test statements
    * Each test statement always starts from a new line
    * Each test statement terminates with a semi-colon `;`
    * Test statement can span across multiple lines
  * The `!` is used to make comments, if found on a line the rest of the line is taken as a comment
  * The first character of a statement indicates the action performed by that statement
  * Refer [Sample scripts](https://github.com/arv-sajeev/EDpAT/tree/master/sample_tests) for simple, easy to follow testscripts.
  
  ## List of commands 
  
  Command | Description
  --------|------------
  `@`| It creates a new test case, test case ID can be specified by `@ <testcase-ID>;` where the testcase ID follows the format `[a-zA-Z0-9]+`
  `#` | Used to include another test-script `# <test-script>` 
  `<` | Used to specify a test case that receives a specified packet sequence from a specified interface `< <interface-id> <packet-specification>;`
  `>` | Used to send a specified packet sequence to a specified interface `> <interface-id> <packet-specification>;`
  `$` | Used to declare a variable and assign a value to it `$<var-name>=<value>;`
  ## Packet specification 
  * The packets are specified byte by byte in hexadecimal format, they can be assigned to variables as shown above and then used in packet specifications
  * The `*` character can be used as as a wildcard in the receive specification, if * is is specified that byte will not be compared
  * `? <n>` is to be used while specifying send packet specification to copy a specified byte from the packet just previously received
  * `&<n1>-<n2>` is to be used to specify that that word is to be filled with the checksum calculated for the bytes from position n1 to n2 of the packet
  
# 5. Limitations
   * The application only supports IPv4 network now
   
   
