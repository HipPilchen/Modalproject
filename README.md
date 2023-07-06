# Modalproject
Tun/Tap interface with encrypted messages for MacOS

-------

To compile the program, just do

$ gcc simpletun.c -o simpletun

$ make simpletun

-------

Usage:
simpletun -i <ifacename> [-s|-c <serverIP>] [-p <port>] [-u|-a] [-d]
simpletun -h

-i <ifacename>: Name of interface to use (mandatory)
-s|-c <serverIP>: run in server mode (-s), or specify server address (-c <serverIP>) (mandatory)
-p <port>: port to listen on (if run in server mode) or to connect to (in client mode), default 55555
-u|-a: use TUN (-u, default) or TAP (-a)
-d: outputs debug information while running
-h: prints this help text

-------

Refer to http://backreference.org/2010/03/26/tuntap-interface-tutorial/ for 
more information on tun/tap interfaces in Linux in general.

Install tuntap:

$ /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

$ echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> /Users/maiapecastaings/.zprofile

$ eval "$(/opt/homebrew/bin/brew shellenv)"

$ brew install git

$ brew install libpcap

$ brew install Caskroom/cask/tuntap

