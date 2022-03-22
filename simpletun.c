/**************************************************************************
 * simpletun.c                                                            *
 *                                                                        *
 * A simplistic, simple-minded, naive tunnelling program using tun/tap    *
 * interfaces and TCP. DO NOT USE THIS PROGRAM FOR SERIOUS PURPOSES.      *
 *                                                                        *
 * You have been warned.                                                  *
 *                                                                        *
 * (C) 2010 Davide Brini.                                                 *
 *                                                                        *
 * DISCLAIMER AND WARNING: this is all work in progress. The code is      *
 * ugly, the algorithms are naive, error checking and input validation    *
 * are very basic, and of course there can be bugs. If that's not enough, *
 * the program has not been thoroughly tested, so it might even fail at   *
 * the few simple things it should be supposed to do right.               *
 * Needless to say, I take no responsibility whatsoever for what the      *
 * program might do. The program has been written mostly for learning     *
 * purposes, and can be used in the hope that is useful, but everything   *
 * is to be taken "as is" and without any kind of warranty, implicit or   *
 * explicit. See the file LICENSE for further details.                    *
 *************************************************************************/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>           //to use the structure ifreq in the tun_alloc function. We don't use linux/if.h because we're not using the linux kernel nor linux userspace.
#include <linux/if_tun.h>     //to create the virtual interface
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>

// buffer for reading from the tun/tap interface, must be >= 1500 
#define BUFSIZE 2000   
#define CLIENT 0
#define SERVER 1
#define PORT 55555

//In case we don't need to use libbpcap but only need the IFF_TUN, IFF_SET, TUNSETIFF:
#define IFF_TUN		0x0001
#define IFF_TAP		0x0002
#define TUNSETIFF     _IOW('T', 202, int) 
#define IFF_NO_PI	0x1000

int debug;
char *progname;

/**************************************************************************
 * tun_alloc: allocates or reconnects to a tun/tap device. The caller     *
 *            must reserve enough space in *name.                          *
 **************************************************************************/
int tun_alloc(char *name, int flags) {
  
 /* Arguments used: 
  * char *name is - the name of an interface:if the field is not empty, the call either create a new virtual network interface 
  *		    with the chosen name or an existing one (depending on whether a network interface with this name already exists)
  *               - or '\0' if the user is requesting the allocation of a new interface
  * int flags is IFF_TUN (to indicate a TUN device) 
  *              or IFF_TAP (to indicate a TAP device) 
  *              or IFF_NO_PI (to tell the kernel that packets will be pure IP packets with no bytes added).
  * If we have set IFF_NO_PI in the ifr_flags field, the version of the IP protocol (IPv4 or IPv6) is deduced from the IP version number in the packet. 
  * If IFF_NO_PI is not set, 4 bytes (struct tun_pi) are prepended to each packet.
  *
  * Return value: the file_descriptor = which the caller will use to talk with the virtual interface
  */

  struct ifreq ifr;                     //the structure ifreq describes the virtual interface, it is used to set the IP address and informations. It doesn't work with AF_INET6 (IPv6 addresses) -> use in6_ifreq
  int file_descriptor;
  int error;
  char *clonedevice = "/dev/net/tun";   //the clone device

  //We try to open the clone device to read/write:
  if( (file_descriptor = open(clonedevice , O_RDWR)) < 0 ) {
    perror("Opening /dev/net/tun");
    return file_descriptor;
  }

  //We initialise the struct ifr:
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = flags;

  //If the name of the device was specified, we write it inside the struct ifr:
  if (*name) {
    strncpy(ifr.ifr_name, name, IFNAMSIZ); //maximum IFNAMSIZ octets are copied
  }

  //We try to create the device (NB: it must be done by root)
  //file_descriptor (first arg) must be open
  //TUNSETIFF is the request code
  //ioctl encodes the struct ifr inside the file_descriptor; itreturns 0 if succeeded, -1 if failed.
  if( (error = ioctl(file_descriptor, TUNSETIFF, (void *)&ifr)) < 0 ) {  
    perror("ioctl(TUNSETIFF)");
    close(file_descriptor);
    return error;
  }

 /*
  * If we succeeded, the virtual interface is created and the file_descriptor is associated to it :
  * The program can communicate with the virtual network interface using the file descriptor.
  *
  * We now write the name of the interface to the veriable "name" so that the caller can know it: 
  */
	
  strcpy(name, ifr.ifr_name);

  return file_descriptor;

 /*
  * The code to attach to an existing tun/tap interface is the same as the code to create one.
  *	
  * When the the program closes the last file descriptor associated with the TUN/TAP interface, the systems destroys the interface. 
  * We can prevent the system from destroying the interface by making it made persistent with the TUNSETPERSIST ioctl() or tunctl (only for tap interfaces) or openvpn --mktun 
  */
	
}


/*
 * We will use the functions read() and write() and not fread() or fwrite()
 * NB: read and write use file descriptors, but fread and fwrite use the FILE pointers,
 * which are actually pointers which contain file descriptors and other info about the file opened.
 */


/**************************************************************************
 * cread: read routine that checks for errors and exits if an error is    *
 *        returned.                                                       *
 **************************************************************************/
int cread(int fd, char *buf, int n){
  
  int nread;

  if((nread=read(fd, buf, n)) < 0){
    perror("Reading data");
    exit(1);
  }
  return nread;
}

/**************************************************************************
 * cwrite: write routine that checks for errors and exits if an error is  *
 *         returned.                                                      *
 **************************************************************************/
int cwrite(int fd, char *buf, int n){
  
  int nwrite;

  if((nwrite=write(fd, buf, n)) < 0){
    perror("Writing data");
    exit(1);
  }
  return nwrite;
}

/**************************************************************************
 * read_n: ensures we read exactly n bytes, and puts them into "buf".     *
 *         (unless EOF, of course)                                        *
 **************************************************************************/
int read_n(int fd, char *buf, int n) {

  int nread, left = n;

  while(left > 0) {
    if ((nread = cread(fd, buf, left)) == 0){
      return 0 ;      
    }else {
      left -= nread;
      buf += nread;
    }
  }
  return n;  
}

/**************************************************************************
 * do_debug: prints debugging stuff (doh!)                                *
 **************************************************************************/
void do_debug(char *msg, ...){
  
  va_list argp;
  
  if(debug) {
	va_start(argp, msg);
	vfprintf(stderr, msg, argp);
	va_end(argp);
  }
}

/**************************************************************************
 * my_err: prints custom error messages on stderr.                        *
 **************************************************************************/
void my_err(char *msg, ...) {

  va_list argp;
  
  va_start(argp, msg);
  vfprintf(stderr, msg, argp);
  va_end(argp);
}

/**************************************************************************
 * usage: prints usage and exits.                                         *
 **************************************************************************/
void usage(void) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s -i <ifacename> [-s|-c <serverIP>] [-p <port>] [-u|-a] [-d]\n", progname);
  fprintf(stderr, "%s -h\n", progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "-i <ifacename>: Name of interface to use (mandatory)\n");
  fprintf(stderr, "-s|-c <serverIP>: run in server mode (-s), or specify server address (-c <serverIP>) (mandatory)\n");
  fprintf(stderr, "-p <port>: port to listen on (if run in server mode) or to connect to (in client mode), default 55555\n");
  fprintf(stderr, "-u|-a: use TUN (-u, default) or TAP (-a)\n");
  fprintf(stderr, "-d: outputs debug information while running\n");
  fprintf(stderr, "-h: prints this help text\n");
  exit(1);
}

//We do tunneling:

int main(int argc, char *argv[]) {
  
  int tap_fd;   //our file_descriptor
  int flags = IFF_TUN;
  char if_name[IFNAMSIZ] = "";
  int maxfd;
  uint16_t nread, nwrite, plength;
  char buffer[BUFSIZE];

  struct sockaddr_in local, remote;

  char remote_ip[16] = "";            /* dotted quad IP string */
  unsigned short int port = PORT;

  int sock_fd, net_fd, optval = 1;
  socklen_t remotelen;
  int cliserv = -1;    /* must be specified on cmd line */
  unsigned long int tap2net = 0, net2tap = 0;

  progname = argv[0];
  
  //Step 1: We check command line options (to know who is the server and who is the client):
	
  int option;
  while((option = getopt(argc, argv, "i:sc:p:uahd")) > 0) {
    switch(option) {
      case 'd':
        debug = 1;
        break;
      case 'h':
        usage();
        break;
      case 'i':
        strncpy(if_name,optarg, IFNAMSIZ-1);
        break;
      case 's':
        cliserv = SERVER;
        break;
      case 'c':
        cliserv = CLIENT;
        strncpy(remote_ip,optarg,15);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'u':
        flags = IFF_TUN;
        break;
      case 'a':
        flags = IFF_TAP;
        break;
      default:
        my_err("Unknown option %c\n", option);
        usage();
    }
  }
	
  
  argv += optind;
  argc -= optind; 
	
  //Since one option is linked to one argument maximum, we check that we don't have more arguments than options:
  if(argc > 0) {
    my_err("Too many options!\n");
    usage();
  }
	
  //We also check that the command gives proper information about the interface and the client/server:
  if(*if_name == '\0') {
    my_err("Must specify interface name!\n");
    usage();
  } else if(cliserv < 0) {
    my_err("Must specify client or server mode!\n");
    usage();
  } else if((cliserv == CLIENT)&&(*remote_ip == '\0')) {
    my_err("Must specify server address!\n");
    usage();
  }

  //Step 2: We initialize tun/tap interface:
  if ((tap_fd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 ) {
    my_err("Error connecting to tun/tap interface %s!\n", if_name);
    exit(1);
  }

  //And we debug:
  do_debug("Successfully connected to interface %s\n", if_name);

  //We create a socket to exchange data with the remote host (destination):
  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    exit(1);
  }

  //Step 3: We do the TCP connection. Two situations are possible : either we're the client or the server.
	
  //First situation, we are the client and try to connect to the server (the remote host) with TCP:
  if(cliserv == CLIENT) {

    //We assign the fields for the destination address:
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(remote_ip);
    remote.sin_port = htons(port);

    //Connection request:
    if (connect(sock_fd, (struct sockaddr*) &remote, sizeof(remote)) < 0) {      //sockaddr and sockaddr_in structures have the same size (16 octets)
      perror("connect()");
      exit(1);
    }

    net_fd = sock_fd;
    do_debug("CLIENT: Connected to server %s\n", inet_ntoa(remote.sin_addr));
    
  }
	
  //Second situation: we are the server and wait for the remote host to connect via TCP:
  else {

    /* avoid EADDRINUSE error on bind() */
    if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
      perror("setsockopt()");
      exit(1);
    }
    
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(port);
    if (bind(sock_fd, (struct sockaddr*) &local, sizeof(local)) < 0) {
      perror("bind()");
      exit(1);
    }
    
    if (listen(sock_fd, 5) < 0) {
      perror("listen()");
      exit(1);
    }
    
    //We wait for connection request:
    remotelen = sizeof(remote);
    memset(&remote, 0, remotelen);
    if ((net_fd = accept(sock_fd, (struct sockaddr*)&remote, &remotelen)) < 0) {
      perror("accept()");
      exit(1);
    }

    do_debug("SERVER: Client connected from %s\n", inet_ntoa(remote.sin_addr));
  }

  //At this stage, the client and the server are connected and can communicate.
  //net_fd is the network file descriptor (for the remote), 
  //tap_fd is the file descriptor connected to the tun/tap interface (so our file descriptor)

  //Step 4: sending data:
	
	
  //We use select() to handle two descriptors at once:
  maxfd = (tap_fd > net_fd)?tap_fd:net_fd;

  while(1) {
    int ret;
    fd_set rd_set;

    FD_ZERO(&rd_set);//initialize
    FD_SET(tap_fd, &rd_set); FD_SET(net_fd, &rd_set);//This macro adds the file descriptor fd to set.  Adding a file descriptor that is already present in the set is a no-op, and does not produce an error.

    ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);
	  /* select() allows a program to monitor multiple file descriptors,
       waiting until one or more of the file descriptors become "ready"
       for some class of I/O operation (e.g., input possible).  A file
       descriptor is considered ready if it is possible to perform a
       corresponding I/O operation (e.g., read(2), or a sufficiently
       small write(2)) without blocking. 1st : This argument should be set to the highest-numbered file
              descriptor in any of the three sets, plus 1. Then : 3 different set */

    if (ret < 0 && errno == EINTR){
      continue;
    }

    if (ret < 0) {
      perror("select()");
      exit(1);
    }

    if(FD_ISSET(tap_fd, &rd_set)) {
      /* data from tun/tap: just read it and write it to the network */
      
      nread = cread(tap_fd, buffer, BUFSIZE);

      tap2net++;
      do_debug("TAP2NET %lu: Read %d bytes from the tap interface\n", tap2net, nread);

      /* write length + packet */
      plength = htons(nread);
      nwrite = cwrite(net_fd, (char *)&plength, sizeof(plength));
      nwrite = cwrite(net_fd, buffer, nread);
      
      do_debug("TAP2NET %lu: Written %d bytes to the network\n", tap2net, nwrite);
    }

    if(FD_ISSET(net_fd, &rd_set)) {
      /* data from the network: read it, and write it to the tun/tap interface. 
       * We need to read the length first, and then the packet */

      /* Read length */      
      nread = read_n(net_fd, (char *)&plength, sizeof(plength));
      if(nread == 0) {
        /* ctrl-c at the other end */
        break;
      }

      net2tap++;

      /* read packet */
      nread = read_n(net_fd, buffer, ntohs(plength));
      do_debug("NET2TAP %lu: Read %d bytes from the network\n", net2tap, nread);

      /* now buffer[] contains a full packet or frame, write it into the tun/tap interface */ 
      nwrite = cwrite(tap_fd, buffer, nread);
      do_debug("NET2TAP %lu: Written %d bytes to the tap interface\n", net2tap, nwrite);
    }
  }
  
  return(0);
}
