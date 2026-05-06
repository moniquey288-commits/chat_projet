# Version 4 — Application de Chat

## Description
Quatrième version de l'application de chat. Implémentation complète
du système de messagerie avec broadcast, unicast, multicast et salons.

## Fonctionnalités
- Broadcast : message à tous les utilisateurs
- Unicast : message privé à un utilisateur
- Multicast : message dans un salon
- Création et gestion de salons
- Destruction automatique du salon quand vide
- Liste des salons disponibles

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
| `/msg <pseudo> <message>` | Message privé |
| `/create <salon>` | Créer un salon |
| `/join <salon>` | Rejoindre un salon |
| `/leave` | Quitter le salon |
| `/list` | Liste des salons |
| `/msgsalon <message>` | Message dans le salon |
| `/quit` | Se déconnecter |

## Exemple d'utilisation
```
/nick aicha
[Server] : Pseudo changé en 'aicha'
Bonjour tout le monde !
[Server] : [aicha] : Bonjour tout le monde !
/msg karim Salut en privé !
[Server] : [Privé à karim] : Salut en privé !
/create general
[Server] : Salon 'general' créé.
/join general
[Server] : Vous avez rejoint le salon 'general'.
/msgsalon Bonjour dans le salon !
```

## Auteur
- **Nom et prenom** : yameogo Monique dit Maimounata 
- **Université** : Université Joseph Ki-Zerbo
- **Cours** : Programmation Système 
- **Enseignant** : Assane Ilboudo