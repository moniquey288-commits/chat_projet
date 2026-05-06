# Version 5 — Version Finale Complète

## Description
Version finale de l'application de chat client/serveur en C.
Intègre toutes les fonctionnalités des versions précédentes plus
des améliorations avancées : transfert de fichiers, modération,
statistiques, logs et gestion d'absence.

## Fonctionnalités
### Fonctionnalités de base (versions 1-4)
- Connexion TCP multi-clients (max 20)
- Identification avec pseudo
- Broadcast, unicast, multicast
- Gestion de salons avec mot de passe optionnel

### Améliorations supplémentaires
- Transfert de fichiers avec approbation
- Expulsion d'utilisateurs (`/kick`)
- Message d'absence automatique (`/away`)
- Statistiques du serveur (`/stats`)
- Horodatage des messages `[HH:MM:SS]`
- Logs dans `chat.log`
- Notifications en temps réel
- Thread POSIX pour réception asynchrone
- Gestion propre de `Ctrl+C` (SIGINT)
- `SO_REUSEADDR` pour redémarrage immédiat

## Compilation
```bash
make
```

## Utilisation

**Terminal 1 — Lancer le serveur :**
```bash
./server
```

**Terminal 2, 3 — Lancer les clients :**
```bash
./client 8080 127.0.0.1
```

## Commandes disponibles

| Commande | Description |
|----------|-------------|
| `/nick <pseudo>` | Choisir son pseudo |
| `/who` | Liste des connectés |
| `/whois <pseudo>` | Infos sur un utilisateur |
| `/msg <pseudo> <msg>` | Message privé |
| `/create <salon> [mdp]` | Créer un salon |
| `/join <salon> [mdp]` | Rejoindre un salon |
| `/leave` | Quitter le salon |
| `/list` | Liste des salons |
| `/msgsalon <msg>` | Message dans le salon |
| `/kick <pseudo>` | Expulser un utilisateur |
| `/away [message]` | Activer/désactiver absence |
| `/stats` | Statistiques du serveur |
| `/sendfile <pseudo> <fich>` | Envoyer un fichier |
| `/accept` | Accepter un fichier |
| `/reject` | Refuser un fichier |
| `/help` | Afficher l'aide |
| `/quit` | Se déconnecter |

## Exemple d'utilisation
```
✔ Connecté au serveur 127.0.0.1:8080
/nick aicha
[Server] : Pseudo changé en 'aicha'
/stats
[Server] : === Statistiques du serveur ===
  Uptime          : 0h 5m 23s
  Clients actifs  : 2 / 20
/away Je suis en réunion
[Server] : Message d'absence défini : "Je suis en réunion"
/sendfile karim rapport.txt
[Server] : Demande envoyée à 'karim'. En attente...
```

## Fichiers du projet
| Fichier | Description |
|---------|-------------|
| `server.c` | Code source du serveur |
| `client.c` | Code source du client |
| `Makefile` | Script de compilation |
| `chat.log` | Journal des activités |

## Auteur
- **Nom et prenom** : yameogo Monique dit Maimounata 
- **Université** : Université Joseph Ki-Zerbo
- **Cours** : Programmation Système 
- **Enseignant** : Assane Ilboudo