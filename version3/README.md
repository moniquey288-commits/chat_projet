# Version 3 — Gestion des utilisateurs

## Description
Troisième version de l'application de chat. Ajout du système
d'identification des utilisateurs avec pseudonymes.

## Fonctionnalités
- Identification avec un pseudo via `/nick`
- Liste des utilisateurs connectés via `/who`
- Informations sur un utilisateur via `/whois`
- Pseudo par défaut "anonyme" à la connexion
- Vérification des pseudos en double
- Déconnexion propre avec `/quit`

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
| `/nick <pseudo>` | Choisir ou changer son pseudo |
| `/who` | Lister les utilisateurs connectés |
| `/whois <pseudo>` | Infos sur un utilisateur |
| `/quit` | Se déconnecter |

## Exemple d'utilisation
```
✔ Connecté au serveur 127.0.0.1:8080
[Server] : Bienvenue ! Identifiez-vous avec /nick <pseudo>
/nick aicha
[Server] : Pseudo changé en 'aicha'
/who
[Server] : Utilisateurs connectés :
- aicha
- karim
/whois karim
[Server] : Utilisateur 'karim' connecté sur socket 5
```

## Auteur
- **Nom et prenom** : yameogo Monique dit Maimounata 
- **Université** : Université Joseph Ki-Zerbo
- **Cours** : Programmation Système 
- **Enseignant** : Assane Ilboudo