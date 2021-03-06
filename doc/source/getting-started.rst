Getting Started
---------------

Prerequisite
************

DCE has been tested only on few systems see:  `Ns-3-dce portability <http://www.nsnam.org/wiki/index.php/Ns-3-dce_portability>`_.
Now I am using it well with a Fedora 14 x86-64 and glibc 2.13-1.

Building options
****************

DCE offers two majors mode of operation:
 1. The basic mode, where DCE use the NS3 TCP stacks,
 2. The advanced mode, where DCE use a linux network stack instead.

Building ns-3 and DCE
*********************

First you need to download NS-3 DCE using mercurial:

::

  $ mkdir test_build_ns3_dce
  $ cd test_build_ns3_dce
  $ hg clone http://code.nsnam.org/furbani/ns-3-dce 


DCE brings a script usable to retrieve the others sources and build the whole things:

:: 

  $ ls ns-3-dce/utils/clone_and_compile_ns3_dce.sh

You should edit this file if you want to try the advanced mode and change the following line:

::

  USE_KERNEL=NO

by

::

  USE_KERNEL=YES

then you can launch the build:

::

  $ ns-3-dce/utils/clone_and_compile_ns3_dce.sh
  clone readversiondef
  ...
  2 files updated, 0 files merged, 0 files removed, 0 files unresolved
  clone ns-3-dce
  ...
  2105 files updated, 0 files merged, 0 files removed, 0 files unresolved
  ...
  Launch NS3TEST-DCE
  PASS process-manager 16.030ms
    PASS Check that process "test-empty" completes correctly. 1.220ms
    PASS Check that process "test-sleep" completes correctly. 0.030ms
    PASS Check that process "test-pthread" completes correctly. 0.020ms
    PASS Check that process "test-mutex" completes correctly. 0.110ms
    PASS Check that process "test-once" completes correctly. 0.030ms
    PASS Check that process "test-pthread-key" completes correctly. 0.020ms
    PASS Check that process "test-sem" completes correctly. 0.040ms
    PASS Check that process "test-malloc" completes correctly. 0.030ms
    PASS Check that process "test-malloc-2" completes correctly. 0.020ms
    PASS Check that process "test-fd-simple" completes correctly. 0.030ms
    PASS Check that process "test-strerror" completes correctly. 0.030ms
    PASS Check that process "test-stdio" completes correctly. 0.030ms
    PASS Check that process "test-string" completes correctly. 0.030ms
    PASS Check that process "test-netdb" completes correctly. 0.220ms
    PASS Check that process "test-env" completes correctly. 0.030ms
    PASS Check that process "test-cond" completes correctly. 0.100ms
    PASS Check that process "test-timer-fd" completes correctly. 0.030ms
    PASS Check that process "test-stdlib" completes correctly. 0.030ms
    PASS Check that process "test-select" completes correctly. 0.080ms
    PASS Check that process "test-nanosleep" completes correctly. 0.030ms
    PASS Check that process "test-random" completes correctly. 0.030ms
    PASS Check that process "test-fork" completes correctly. 0.030ms
    PASS Check that process "test-local-socket" completes correctly. 12.840ms
    PASS Check that process "test-poll" completes correctly. 0.090ms
    PASS Check that process "test-tcp-socket" completes correctly. 0.880ms
    
After some minutes all is compiled and DCE test should complete successfully as above.
Now you should see some directories:
::

  $ ls
  build  ns-3-dev  ns-3-dce  readversiondef ns-3-linux iproute2-2.6.33

Where:
 1. *build* contains the result of compilation: some binaries some libs and some include files usable to do your simulations scripts.

 2. *ns-3-dev* contains the current sources of NS3, 

 3. *ns-3-dce* contains the DCE sources,

 4. *readversiondef* contains source of a tool used by DCE build system. 

 5. *ns-3-linux* (only in advanced mode) contains source of a linux kernel + some glue code for DCE / Kernel communication.

 6. *iproute2-2.6.33* (only in advanced mode) contains source of *ip* tool needed to be compiled for DCE in order to configure ip routes of the slave kernel used by DCE.

Setting Environnement
*********************

In order to have the good paths you should call the setenv.sh script in order to setup env variables (PATH, LD_LIBRARY_PATH and PKG_CONFIG_PATH):

::

  $ source ns-3-dce/utils/setenv.sh

Examples
********

Under example directory there is the sources files of DCE samples simulations.

Sample: DCE UPD SAMPLE
######################

This sample execute the binaries named udp-client and udp-server under NS3 using DCE.

This 2 binaries are writen using libc api in order to send and received udp packets.

Please take time to look at the source dce-udp-simple.cc which is our NS3 simulation "script":

::

  int main (int argc, char *argv[])
  {
    CommandLine cmd;
    cmd.Parse (argc, argv);
  
    NodeContainer nodes;
    nodes.Create (1);
  
    InternetStackHelper stack;
    stack.Install (nodes);
  
    DceManagerHelper dceManager;
    dceManager.Install (nodes);
  
    DceApplicationHelper dce;
    ApplicationContainer apps;
  
    dce.SetStackSize (1<<20);
  
    dce.SetBinary ("udp-server");
    dce.ResetArguments();
    apps = dce.Install (nodes.Get (0));
    apps.Start (Seconds (4.0));
  
    dce.SetBinary ("udp-client");
    dce.ResetArguments();
    dce.AddArgument ("127.0.0.1");
    apps = dce.Install (nodes.Get (0));
    apps.Start (Seconds (4.5));
  
    Simulator::Stop (Seconds(1000100.0));
    Simulator::Run ();
    Simulator::Destroy ();
  
    return 0;
  }
  
You can notice that we create a NS-3 Node with an Internet Stack (please refer to `NS-3 <http://www.nsnam.org/documentation/>`_ doc. for more infos),
and we can also see 2 new Helpers:

 1. DceManagerHelper which is used to Manage DCE loading system in each nodes where it DCE will be used.
 2. DceApplicationHelper which is used to describe real application to be lauched by DCE within NS-3 simulation environnement.
 
As you have already set the environnement you can launch this simulation from anywhere:

::

  $ cd /tmp
  $ mkdir my_test
  $ cd my_test
  $ dce-udp-simple
  $ ls 
    elf-cache  files-0
  $ ls -lR files-0
    files-0:
    total 4
    drwxr-x--- 3 furbani planete 4096 Sep  2 17:02 var

    files-0/var:
    total 4
    drwxr-x--- 4 furbani planete 4096 Sep  2 17:02 log

    files-0/var/log:
    total 8
    drwxr-x--- 2 furbani planete 4096 Sep  2 17:02 53512
    drwxr-x--- 2 furbani planete 4096 Sep  2 17:02 53513

    files-0/var/log/53512:
    total 12
    -rw------- 1 furbani planete  12 Sep  2 17:02 cmdline
    -rw------- 1 furbani planete 185 Sep  2 17:02 status
    -rw------- 1 furbani planete   0 Sep  2 17:02 stderr
    -rw------- 1 furbani planete  21 Sep  2 17:02 stdout

    files-0/var/log/53513:
    total 12
    -rw------- 1 furbani planete  22 Sep  2 17:02 cmdline
    -rw------- 1 furbani planete 185 Sep  2 17:02 status
    -rw------- 1 furbani planete   0 Sep  2 17:02 stderr
    -rw------- 1 furbani planete  22 Sep  2 17:02 stdout

This simulation produce two directories, the content of elf-cache is not important now for us, but files-0 is.
files-0 contains the files tree of the first node, it also contains the result files of the dce applications launched on this node. So in the directory /var/log there is some directories named with the virtual pid of corresponding DCE applications. Under these directories there is always 4 files:

  1. cmdline : which contains the command line of the corresponding DCE application, in order to help you to retrieve what is it,
  2. stdout: contains the stdout produced by the execution of the corresponding application,
  3. stderr: contains the stderr produced by the execution of the corresponding application.
  4. status: contains a status of the corresponding process with the start time of it and if exists the end time with the exit code.
              
You may also create files-xx directories before launching your simulation, and you may also provides some files needed by your applications under these directories.

Sample DCE LINUX
################

This sample show how to use DCE in advanced mode, with a linux kernel IP stack.
It uses also the binaries *udp-server* and *udp-client* like the above sample, there is also *tcp-server* and *tcp-client* if you choose the reliable transport option.
Two other binaries are required the linux kernel stack named *libnet-next-2.6.so* and the tool needed to configure this kernel stack named *ip*.
This example simulates an exchange of data between too nodes, using TCP or UDP, and the nodes are linked by one of three possible links , Wifi, Point 2 point or CSMA.
The main executable is named *dce-linux*, it cames with too options:

  1. linkType allow to choose the link type between c, w or p for Csma, Wifi or Point 2 point,
  2. reliable allow to choose transport between TCP (1) or UDP (0).

The following code snippet show how to enable DCE advanced mode (you can see it in the source file dce-linux.cc under example directory):

::

  DceManagerHelper processManager;
 // processManager.SetLoader ("ns3::DlmLoaderFactory");
  processManager.SetNetworkStack("ns3::LinuxSocketFdFactory", "Library", StringValue ("libnet-next-2.6.so"));
  processManager.Install (nodes);

  for (int n=0; n < 2; n++)
    {
      AddAddress (nodes.Get (n), Seconds (0.1), "sim0", "10.0.0.", 2 + n, "/8" );
      RunIp (nodes.Get (n), Seconds (0.11), ( 'p' == linkType )? "link set sim0 up arp off":"link set sim0 up arp on");
      RunIp (nodes.Get (n), Seconds (0.2), "link show");
      RunIp (nodes.Get (n), Seconds (0.3), "route show table all");
      RunIp (nodes.Get (n), Seconds (0.4), "addr list");
    }

The first important call is *SetNetworkStack* used to indicate which file contains the linux kernel stack.
Then in the for loop we setup on each nodes the networks interfaces using the ip executable to configure the kernel stack.
Because this source code factorizes some call, it is not very readeable so below there is the corresponding calls to ip executable with the arguments:

::

   ip -f inet addr add 10.0.0.2 dev sim0        // set the ip adresse of the first (sim0) net device of the corresponding node
   ip link set sim0 up arp on                   // enable the use of the device use arp off instead for P2P link
   ip link show
   ip route show table all
   ip addr list



CCNx samples
############

Under example/ccnx there is more realistics samples using the implementation of an experimental protocol named CCN. In this examples we use the `PARC  <http://www.parc.com>`_ implementation named `CCNx <http://www.ccnx.org>`_ (c) in its early version 0.4.0.
  
CCNx setup
==========

In order to run ccnx binaries you must compile them with some required compilator and linker parameters.
The principe here is to obtain Position Independent Executable. To obtain this type of exe you should use the gcc -fPIC when compiling sources, and the option -pie when linking your exe.
For CNNx we notice that (under linux) its configure script sets by default the -fPIC option, you can check it in the generated file named conf.mk under directory ccnx.0.4.0/csrc:
::

  $ cat cscr/conf.mk
  ...
  PLATCFLAGS=-fPIC
  ...

Then you should start the make like this:

::

  $ make MORE_LDLIBS=-pie

Then you must verify that your ccnx run well, to do this read the README file, then try to launch a ccnd daemon and retrieve a file using the commands ccnget and ccnput. This verification is MANDATORY in order to create the key files used by ccnx to sign and verify exchanged data, these keys files can not by produced by NS3/DCE so we should copy them in simulation environnement before doing the simulations as we explain it in the following chapter.

Simulation script setup
=======================

To launch the ccnx simulation you must change some path in a script shell used to setup the virtual tree content of the nodes of the simulation.
This script shell is under example/ccnx directory, it is named run-ccnx-common.sh:

+-----------------------+---------------------------------------------------+--------------------------+
| Variable name         | Description                                       | Example                  |  
+-----------------------+---------------------------------------------------+--------------------------+
| CCNX_PATH             | Where to find the ccnx sources and executables    | $HOME/dev/ccnx-0.4.0/    |
+-----------------------+---------------------------------------------------+--------------------------+
| CCNX_KEY_PATH         | Path to keystore used by ccn commands like ccnget | $HOME/.ccnx              |
+-----------------------+---------------------------------------------------+--------------------------+
| CCND_REAL_KEYSTORE    | Path to keystore used by ccnd daemon              | /var/tmp/.ccnx-user15019 |
+-----------------------+---------------------------------------------------+--------------------------+
| VIRTUAL_USER_KEYSTORE | Path to NS3 keystore used by ccn commands         | /home/furbani            |
+-----------------------+---------------------------------------------------+--------------------------+

Sample CCNX-SIMPLE
##################

This simulation launch a ccnd daemon, publish a file using ccnput and retrieve this data using ccnget command, all command are on a single node:

::

  $ . ....../ns-3-dce/utils/setenv.sh
  $ cd ...../ns-3-dce/example/ccnx
  $ ./run-ccnx-simple.sh 

This script end with opening an emacs displaying the output of the simulation command and the output of the simulated process ie: ccnd, ccnget and ccnput.
The stdout of ccnget should be named : files-0/var/log/53514/stdout and it must contains the 8 first Ko of the CCNX README file, this is the file published by ccnget.

Sample CCND LINEAR MULTIPLE
###########################

This simulation use multiple nodes placed in a line, each node are linked 2 by 2 by a point to point link, each node holds a ccnd daemon, the first node put a file (with ccnput), and the last node fetch this file (with ccnget). Also each node minus the first one forward interrests starting with /NODE0 to its predecessor.

  .. image:: images/ccnd-linear-multiple-1.png

The launch script run-ccnd-linear-multiple.sh offer 3 options:

 1. NNODES allows to choose the Number of Nodes,
 2. USE-TCP allows to use TCP or if not UDP to connect the ccnd deamons (via forwarding interrest).
 3. KERN allows to use Linux IP Stack (only working in advanced mode) instead of NS3 one.

for example with 200 nodes and TCP transport you should see this in the first ccnget output command:

::

  $ cat files-199/var/log/30918/stdout
  Si tu peux lire ca ca marche !
  ...
  Si tu peux lire ca ca marche !

  $ cat files-199/var/log/30918/status
  Start Time: NS3 Time:          2s (          +2700000000.0ns) , REAL Time: 1314977630
        Time: NS3 Time:         11s (         +11890668601.0ns) , REAL Time: 1314977632 --> Exit (0)
 
You can see that the first get take about 9 seconds,

now if we use UDP :

::

  $ cat files-199/var/log/30918/status


In this case the first get take about 1 second. The difference between UDP and TCP is due to fact that in TCP mode it occurs 199 TCP connections. Notice also that in this configuration there is no UDP packet lost, but it is possible to ask NS3 to simulate some sort of packet lost behavior.











 


 
  







  
  
  
  
  
  
  
    
    
  
