# Modalproject
simpletun, a (too) simple tunnelling program.

-------

To compile the program, just do

$ gcc simpletun.c -o simpletun

If you have GNU make, you can also exploit implicit targets and do

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
more information on tun/tap interfaces in Linux in general, and on this 
program in particular.
The program must be run at one end as a server, and as client at the other 
end. The tun/tap interface must already exist, be up and configured with an IP 
address, and owned by the user who runs simpletun. That user must also have
read/write permission on /dev/net/tun. (Alternatively, you can run the
program as root, and configure the transient interfaces manually before
starting to exchange packets. This is not recommended)

Use is straightforward. On one end just run

[server]$ ./simpletun -i tun13 -s

at the other end run

[client]$ ./simpletun -i tun0 -c 10.2.3.4

where 10.2.3.4 is the remote server's IP address, and tun13 and tun0 must be 
replaced with the names of the actual tun interfaces used on the computers.
By default it assumes a tun device is being used (use -u to be explicit), and
-a can be used to tell the program that the interface is tap. 
By default it uses TCP port 55555, but you can change that by using -p (the 
value you use must match on the client and the server, of course). Use -d to 
add some debug information. Press ctrl-c on either side to exit (the other end
will exit too).

The program is very limited, so expect to be disappointed.
  
  Qu’est-ce que TUN / TAP?

Contrairement aux périphériques réseau ordinaires dans un système (périphériques physiques acheminant les paquets via des câbles Ethernet), TUN / TAP est une interface complètement virtuelle qui simule ces connexions physiques au sein du noyau du système d’exploitation (la partie du système d’exploitation qui est toujours active dans votre périphérique. mémoire et a un contrôle complet sur tout dans le système).
  L’avantage de TUN / TAP est que les applications de l’espace utilisateur, telles que les clients VPN, peuvent interagir avec ces appareils comme s’ils étaient réels. Cela permet au système d’exploitation d’injecter des paquets dans la pile réseau régulière selon les besoins, ce qui entraîne des transferts de données comme si des périphériques réseau physiques étaient en cours d’utilisation.

  
  Problèmes du programme sur le tutoriel (à bosser pour nous):
  - L'interface virtuelle n'est pas persistente
  - Ne prend pas en compte que read_n() et cwrite() peuvent bloquer
  - Implémenter TCP et aussi UDP
  - Regarder le TCP-over-TCP pourquoi c'est pas bien

 Questions :
  - linux include <linux/if_tun.h> --> bibliothèque libpcap sur macOS. 1. installer homebrew
  - Comment le local host récupère l'info de son interface ? (décodage) Et comment le local host écrit à son interface virtuelle ?
  - Pour rendre persistent ? tunctl ou ioctl(,TUNSETPERSIST) ou openvpn --mktun

  qd on envoie données sur réseau, on envoie table de routage (qui dit: ce packet est envoyé à telle itneface). Par ex afficher le packet sous forme hexadécimale sur l'écran - boucle sur le buffer et printf. On peut le garder, le chiffrer, l'envoyer via le réseau. tap interface = comme un robinet, on arrive à extraire un paquet. --> intercepter le paquet sortant, le prendre en copie et fait qqch. ON fait ce qu'on veut du buffer, on peut l'afficher, l'encoder, écrire une copie sur le disque etc.
