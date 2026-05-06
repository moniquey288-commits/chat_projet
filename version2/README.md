# Version 2 — Serveur Multi-clients

## Description
Deuxième version de l'application de chat. Le serveur peut maintenant
gérer plusieurs clients simultanément grâce à la fonction `select()`.

## Fonctionnalités
- Gestion simultanée de 20 clients maximum
- Utilisation de `select()` pour le multiplexage
- Refus du 21ème client avec message d'erreur
- Écho de messages à chaque client
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

**Terminal 2, 3, ... — Lancer les clients :**
```bash
./client 8080 127.0.0.1
```

**Tester les 21 clients :**
```bash
for i in $(seq 1 21); do ./client 8080 127.0.0.1 & done
```

## Commandes disponibles

| Commande | Description |
|----------|-------------|
| `/quit`  | Se déconnecter du serveur |

## Exemple d'utilisation
```
✔ Serveur en écoute (max 20 clients)...
✔ Client connecté (socket 4) 1/20 clients
✔ Client connecté (socket 5) 2/20 clients
...
✔ Client connecté (socket 23) 20/20 clients
⚠ Connexion refusée (serveur plein)
```

## Auteur
- **Nom et prenom** : yameogo Monique dit Maimounata 
- **Université** : Université Joseph Ki-Zerbo
- **Cours** : Programmation Système 
- **Enseignant** : Assane Ilboudo