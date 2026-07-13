# Ft_traceroute

## Fonctionnement principale

- Traceroute utilise le protocole IP. Plus particulièrement le paramètre time to live (TTL).
  - Il envoie un paquet avec des TTL faibles afin de recevoir des retours d'erreur: ICMP TIME_EXCEEDED
- A chaque nouveau paquet envoyé le TTL est incrémenté jusqu'à recevoir un ICMP "port unreachable", ce qui signifie que le paquet a atteint l'host.
- Il faut que j'envoie 3 paquets par 3 paquets avec le même ttl. Et que j'incrémente de 1 le TTL
- Je surveille chaque paquet pour vérifier qu'il n'y a pas un host unreachable.
- Le soucis réside dans la réception du paquet, car quand je regarde sur wireshark je n'ai pas de 

## Protocole et paquet

- Il utilise l'UDP (couche 4 du modèle OSI => Transport)

Paquet
- AF_INET => IPV4 packet 
- SOCK_DGRAM => UDP -- pas de SOCK_RAW car je n'ai pas l'accès root

## Envoie

- Pour l'envoie du packet rien de bien compliqué:
  - ```C socket(AF_INET, SOCK_DGRAM, 0);```
  - on crée une sockaddr_in qui correspond à l'addresse de destination

## Réception

- La réception du paquet finale se fait en ICMP. Il faut donc créer un autre paquet qui va écouter les retours ICMP.

## UDP header

- 8 octets
  - Port source => (2 octets)
  - Port dest => (2 octets)
  - length => (2 octets)
  - checksum => (2 octets)


## FONCTIONS

- **getpid / getuid** => pour envoyer l'id dans le paquet
- **getaddrinfo / gettimeofday** => Récupère les addresses du domaine spécifiés dans node
- **getnameinfo** => inverse de getaddrinfo, convertis une addresse IP en nom de domaine.
- **gethostbyaddr** => obsolète, vaut mieux utiliser getaddrinfo car elle fait la même chose
- **inet_ntoa / inet_pton** => convertie une string de network byte order vers vers une string corespondant à l'écriture décimal séparé par des points / Convertis une addresse texte IPV4 / IPV6 en binaire.
- **ntohs / htons** => unsigned short de network byte order vers host byte order / l'inverse
- **freeaddrinfo** => free la structure addrinfo
- **exit** => quitte le process
- **select** => Surveille des file descriptor (fd)
- **setsockopt** => Configurer des options sur un socket
- **recvfrom / sendto** => fonction d'envoies et de réception de paquets
- **bind / socket / close** => Lier un socket avec une addresse / Créer un socket / fermer un file descriptor
- **strerror / gai_strerror** => renvoe la string correspondant au code d'erreur / retourne la string correspondant au code d'erreur de getaddrinfo et getnameinfo