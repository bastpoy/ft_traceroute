# Ft_traceroute

## Fonctionnement principale

- Traceroute utilise le protocole IP. Plus particulièrement le paramètre time to live (TTL).
  - Il envoie un paquet avec des TTL faibles afin de recevoir des retours d'erreur: ICMP TIME_EXCEEDED
- A chaque nouveau paquet envoyé le TTL est incrémenté jusqu'à recevoir un ICMP "port unreachable", ce qui signifie que le paquet a atteint l'host.
- Il faut que j'envoie 3 paquets par 3 paquets avec le même ttl. Et que j'incrémente de 1 le TTL
- Je surveille chaque paquet pour vérifier qu'il n'y a pas un host unreachable.
- Attendre 5 sec avant de timeout un ttl

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
- icmp time excedeed : type 11
- icmp host unreach : type 3 code 3

## UDP header

- 8 octets
  - Port source => (2 octets)
  - Port dest => (2 octets)
  - length => (2 octets)
  - checksum => (2 octets)
