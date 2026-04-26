# 💬 Application de Chat Client/Serveur en C

> Projet de Programmation Système — L3 Informatique  
> Université Joseph Ki-Zerbo (UJKZ)  
> Enseignant : Dr. Moustapha BIKIENGA

---

## 📋 Description

Application de chat en temps réel fonctionnant selon une architecture **client/serveur TCP** développée en langage C sous Linux.

- Jusqu'à **20 clients simultanés** grâce à `select()`
- **Broadcast**, **unicast** et **multicast** (salons)
- **Transfert de fichiers** avec approbation
- **Salons protégés** par mot de passe
- **Journalisation** horodatée dans `chat.log`

---

## 🗂️ Structure du projet

```
chat_project/
├── server.c          # Serveur multi-clients (select)
├── client.c          # Client TCP (pthread)
├── Makefile          # Compilation automatique
├── chat.log          # Journal d'activité (généré à l'exécution)
└── README.md         # Ce fichier
```

---

## ⚙️ Compilation

```bash
make
```

Ou manuellement :

```bash
gcc server.c -o server
gcc client.c -o client -lpthread
```

---

## 🚀 Utilisation

### 1. Lancer le serveur

```bash
./server
```

### 2. Connecter un client

```bash
./client <port> <adresse_IP>
```

Exemple en local :

```bash
./client 8080 127.0.0.1
```

### 3. S'identifier

```
/nick <pseudo>
```

---

## 📖 Commandes disponibles

### Utilisateurs
| Commande | Description |
|---|---|
| `/nick <pseudo>` | Choisir ou changer son pseudo |
| `/who` | Lister les utilisateurs connectés |
| `/whois <pseudo>` | Infos sur un utilisateur |
| `/away [message]` | Activer un message d'absence |
| `/away` | Revenir disponible |
| `/quit` | Se déconnecter |

### Messages
| Commande | Description |
|---|---|
| `<message>` | Envoyer à tous (broadcast) |
| `/msg <pseudo> <message>` | Message privé (unicast) |
| `/msgsalon <message>` | Message dans le salon (multicast) |

### Salons
| Commande | Description |
|---|---|
| `/create <nom>` | Créer un salon |
| `/create <nom> <mdp>` | Créer un salon protégé |
| `/join <nom>` | Rejoindre un salon |
| `/join <nom> <mdp>` | Rejoindre un salon protégé |
| `/leave` | Quitter le salon actuel |
| `/list` | Lister les salons disponibles |

### Transfert de fichiers
| Commande | Description |
|---|---|
| `/sendfile <pseudo> <fichier>` | Demander à envoyer un fichier |
| `/accept` | Accepter un fichier entrant |
| `/reject` | Refuser un fichier entrant |
| `/file <contenu>` | Envoyer le contenu du fichier |

### Administration
| Commande | Description |
|---|---|
| `/kick <pseudo>` | Expulser un utilisateur |
| `/stats` | Statistiques du serveur |
| `/help` | Afficher l'aide |

---

## 🧪 Exemple de session

**Terminal 1 — Serveur :**
```bash
./server
```

**Terminal 2 — mimi :**
```
./client 8080 127.0.0.1
/nick mimi
Bonjour tout le monde !
/create general motdepasse
/join general motdepasse
/msgsalon Bienvenue dans le salon !
```

**Terminal 3 — karim :**
```
./client 8080 127.0.0.1
/nick karim
/msg mimi Salut mimi !
/join general motdepasse
/msgsalon Bonjour depuis le salon !
```

---

## 📌 Limites

- Maximum **20 clients** simultanés
- Maximum **10 salons** simultanés
- Le transfert de fichier passe par le buffer texte (1024 octets)
- Un seul transfert en attente par utilisateur à la fois

---

## 👩‍💻 Auteur

**Maïmounata**  
L3 Informatique — Université Joseph Ki-Zerbo  
Burkina Faso, 2024–2025
