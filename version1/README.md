# Version 1 — Client/Serveur de base

## Description
Première version de l'application de chat client/serveur en C.
Implémente la communication TCP de base entre un client et un serveur.

## Fonctionnalités
- Connexion TCP client/serveur sur le port 8080
- Écho de messages (le serveur renvoie le message reçu au client)
- Déconnexion propre avec la commande `/quit`

## Compilation
```bash
make
```

## Utilisation

**Terminal 1 — Lancer le serveur :**
```bash
./server
```

**Terminal 2 — Lancer le client :**
```bash
./client 8080 127.0.0.1
```

## Commandes disponibles

| Commande | Description |
|----------|-------------|
| `/quit`  | Se déconnecter du serveur |

## Exemple d'utilisation
```
✔ Socket créée
✔ Bind effectué sur le port 8080
✔ Serveur en écoute...
✔ Client connecté !
[Client] : Bonjour le serveur !
[Client] : /quit
Connexion fermée.
```

## Auteur
- **Nom et prenom** : yameogo Monique dit Maimounata 
- **Université** : Université Joseph Ki-Zerbo
- **Cours** : Programmation Système 
- **Enseignant** : Assane Ilboudo