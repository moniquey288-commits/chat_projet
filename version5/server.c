/**
 * @file    server.c
 * @author  Maïmounata (Université Joseph Ki-Zerbo)
 * @date    2024
 * @brief   Serveur de chat multi-clients TCP avec select()
 *
 * Fonctionnalités :
 *  - Connexions simultanées jusqu'à MAX_CLIENTS clients
 *  - Gestion des pseudos, salons (avec mot de passe optionnel)
 *  - Broadcast, unicast, multicast
 *  - Transfert de fichiers avec approbation
 *  - Commandes avancées : /stats, /kick, /away, /list, /help ...
 *  - Logging horodaté dans chat.log
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <time.h>

#define PORT            8080
#define BUFFER_SIZE     1024
#define MAX_CLIENTS     20
#define MAX_SALONS      10
#define MAX_PSEUDO_LEN  50
#define MAX_SALON_LEN   50
#define MAX_FILENAME    255
#define MAX_FILE_DATA   65536
#define LOG_FILE        "chat.log"

typedef struct {
    int  socket_fd;
    char pseudo[MAX_PSEUDO_LEN];
    char salon[MAX_SALON_LEN];
    char away_msg[BUFFER_SIZE];
    int  nb_messages;
} Client;

typedef struct {
    char nom[MAX_SALON_LEN];
    char password[MAX_PSEUDO_LEN];
    int  actif;
} Salon;

typedef struct {
    int  sender_idx;
    int  receiver_idx;
    char filename[MAX_FILENAME];
    int  pending;
} FileTransfer;

Client      clients[MAX_CLIENTS];
Salon       salons[MAX_SALONS];
FileTransfer transfers[MAX_CLIENTS];

int  nb_clients       = 0;
int  nb_salons        = 0;
int  total_connexions = 0;
int  total_messages   = 0;
time_t server_start;

void get_heure(char *heure) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(heure, 10, "%H:%M:%S", t);
}

void log_message(const char *message) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    char heure[10];
    get_heure(heure);
    fprintf(f, "[%s] %s\n", heure, message);
    fclose(f);
}

int find_client_by_pseudo(const char *pseudo) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd != -1 &&
            strcmp(clients[i].pseudo, pseudo) == 0)
            return i;
    }
    return -1;
}

int find_salon(const char *nom) {
    for (int i = 0; i < MAX_SALONS; i++) {
        if (salons[i].actif && strcmp(salons[i].nom, nom) == 0)
            return i;
    }
    return -1;
}

void send_to(int idx, const char *msg) {
    send(clients[idx].socket_fd, msg, strlen(msg), 0);
}

void disconnect_client(int idx) {
    close(clients[idx].socket_fd);
    clients[idx].socket_fd = -1;
    memset(clients[idx].pseudo,    0, MAX_PSEUDO_LEN);
    memset(clients[idx].salon,     0, MAX_SALON_LEN);
    memset(clients[idx].away_msg,  0, BUFFER_SIZE);
    clients[idx].nb_messages = 0;
    transfers[idx].pending   = 0;
    nb_clients--;
}

void detruire_salon_si_vide(const char *nom_salon) {
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (clients[j].socket_fd != -1 &&
            strcmp(clients[j].salon, nom_salon) == 0)
            return;
    }
    int idx = find_salon(nom_salon);
    if (idx != -1) {
        salons[idx].actif = 0;
        memset(salons[idx].nom,      0, MAX_SALON_LEN);
        memset(salons[idx].password, 0, MAX_PSEUDO_LEN);
        nb_salons--;
        char log_buf[BUFFER_SIZE];
        snprintf(log_buf, BUFFER_SIZE, "Salon '%s' détruit (vide)", nom_salon);
        log_message(log_buf);
        printf("Salon '%s' détruit\n", nom_salon);
    }
}

void broadcast(int sender_idx, const char *message) {
    char msg[BUFFER_SIZE], heure[10];
    get_heure(heure);
    snprintf(msg, BUFFER_SIZE, "[%s][%s] : %s\n",
             heure, clients[sender_idx].pseudo, message);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd != -1 && i != sender_idx) {
            send_to(i, msg);
            if (strlen(clients[i].away_msg) > 0) {
                char away[2 * BUFFER_SIZE];
                snprintf(away, sizeof(away),
                         "[AUTO] %s est absent(e) : %s\n",
                         clients[i].pseudo, clients[i].away_msg);
                send_to(sender_idx, away);
            }
        }
    }
    total_messages++;
    clients[sender_idx].nb_messages++;
}

void unicast(int sender_idx, const char *target_pseudo, const char *message) {
    int idx = find_client_by_pseudo(target_pseudo);
    char msg[BUFFER_SIZE], heure[10];
    get_heure(heure);
    if (idx == -1) {
        snprintf(msg, BUFFER_SIZE, "Utilisateur '%s' introuvable.\n", target_pseudo);
        send_to(sender_idx, msg);
    } else {
        if (strlen(clients[idx].away_msg) > 0) {
            char away[2 * BUFFER_SIZE];
            snprintf(away, sizeof(away),
                     "[AUTO] %s est absent(e) : %s\n",
                     clients[idx].pseudo, clients[idx].away_msg);
            send_to(sender_idx, away);
        }
        snprintf(msg, BUFFER_SIZE, "[%s][Privé de %s] : %s\n",
                 heure, clients[sender_idx].pseudo, message);
        send_to(idx, msg);
        snprintf(msg, BUFFER_SIZE, "[%s][Privé à %s] : %s\n",
                 heure, target_pseudo, message);
        send_to(sender_idx, msg);
    }
}

void multicast(int sender_idx, const char *message) {
    char msg[BUFFER_SIZE], heure[10];
    get_heure(heure);
    snprintf(msg, BUFFER_SIZE, "[%s][Salon %s][%s] : %s\n",
             heure, clients[sender_idx].salon,
             clients[sender_idx].pseudo, message);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd != -1 && i != sender_idx &&
            strcmp(clients[i].salon, clients[sender_idx].salon) == 0)
            send_to(i, msg);
    }
}

void cmd_nick(int i, const char *buffer) {
    char new_pseudo[MAX_PSEUDO_LEN];
    strncpy(new_pseudo, buffer + 6, MAX_PSEUDO_LEN - 1);
    new_pseudo[MAX_PSEUDO_LEN - 1] = '\0';
    char msg[BUFFER_SIZE];
    if (strlen(new_pseudo) == 0) {
        send_to(i, "Usage : /nick <pseudo>\n");
        return;
    }
    int existing = find_client_by_pseudo(new_pseudo);
    if (existing != -1 && existing != i) {
        snprintf(msg, BUFFER_SIZE, "Pseudo '%s' déjà utilisé.\n", new_pseudo);
        send_to(i, msg);
    } else {
        char old_pseudo[MAX_PSEUDO_LEN];
        strncpy(old_pseudo, clients[i].pseudo, MAX_PSEUDO_LEN);
        strncpy(clients[i].pseudo, new_pseudo, MAX_PSEUDO_LEN);
        snprintf(msg, BUFFER_SIZE, "Pseudo changé en '%s'\n", new_pseudo);
        send_to(i, msg);
        char notif[BUFFER_SIZE];
        snprintf(notif, BUFFER_SIZE, "*** %s est maintenant connu sous '%s' ***\n",
                 old_pseudo, new_pseudo);
        for (int j = 0; j < MAX_CLIENTS; j++) {
            if (clients[j].socket_fd != -1 && j != i)
                send_to(j, notif);
        }
    }
}

void cmd_who(int i) {
    char msg[BUFFER_SIZE];
    strcpy(msg, "=== Utilisateurs connectés ===\n");
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (clients[j].socket_fd != -1) {
            char line[BUFFER_SIZE];
            if (strlen(clients[j].away_msg) > 0)
                snprintf(line, BUFFER_SIZE, "  - %s [absent]\n", clients[j].pseudo);
            else if (strlen(clients[j].salon) > 0)
                snprintf(line, BUFFER_SIZE, "  - %s [salon: %s]\n",
                         clients[j].pseudo, clients[j].salon);
            else
                snprintf(line, BUFFER_SIZE, "  - %s\n", clients[j].pseudo);
            strncat(msg, line, BUFFER_SIZE - strlen(msg) - 1);
        }
    }
    strncat(msg, "==============================\n", BUFFER_SIZE - strlen(msg) - 1);
    send_to(i, msg);
}

void cmd_whois(int i, const char *buffer) {
    char target[MAX_PSEUDO_LEN];
    strncpy(target, buffer + 7, MAX_PSEUDO_LEN - 1);
    int idx = find_client_by_pseudo(target);
    char msg[BUFFER_SIZE];
    if (idx == -1) {
        snprintf(msg, BUFFER_SIZE, "Utilisateur '%s' introuvable.\n", target);
    } else {
        snprintf(msg, BUFFER_SIZE,
                 "Utilisateur : %s | Socket : %d | Salon : %s | Messages : %d\n",
                 clients[idx].pseudo,
                 clients[idx].socket_fd,
                 strlen(clients[idx].salon) > 0 ? clients[idx].salon : "(aucun)",
                 clients[idx].nb_messages);
    }
    send_to(i, msg);
}

void cmd_create(int i, const char *buffer) {
    char nom_salon[MAX_SALON_LEN]  = {0};
    char password[MAX_PSEUDO_LEN]  = {0};
    char msg[BUFFER_SIZE];
    char args[BUFFER_SIZE];
    strncpy(args, buffer + 8, BUFFER_SIZE - 1);
    char *space = strchr(args, ' ');
    if (space) {
        strncpy(nom_salon, args, space - args);
        strncpy(password, space + 1, MAX_PSEUDO_LEN - 1);
    } else {
        strncpy(nom_salon, args, MAX_SALON_LEN - 1);
    }
    if (strlen(nom_salon) == 0) {
        send_to(i, "Usage : /create <nom> [motdepasse]\n");
        return;
    }
    if (find_salon(nom_salon) != -1) {
        snprintf(msg, BUFFER_SIZE, "Salon '%s' existe déjà.\n", nom_salon);
        send_to(i, msg);
        return;
    }
    if (nb_salons >= MAX_SALONS) {
        send_to(i, "Nombre maximum de salons atteint.\n");
        return;
    }
    for (int j = 0; j < MAX_SALONS; j++) {
        if (!salons[j].actif) {
            strncpy(salons[j].nom,      nom_salon, MAX_SALON_LEN - 1);
            strncpy(salons[j].password, password,  MAX_PSEUDO_LEN - 1);
            salons[j].actif = 1;
            nb_salons++;
            if (strlen(password) > 0)
                snprintf(msg, BUFFER_SIZE,
                         "Salon '%s' créé avec mot de passe.\n", nom_salon);
            else
                snprintf(msg, BUFFER_SIZE, "Salon '%s' créé.\n", nom_salon);
            send_to(i, msg);
            char log_buf[BUFFER_SIZE];
            snprintf(log_buf, BUFFER_SIZE, "Salon '%s' créé par %s",
                     nom_salon, clients[i].pseudo);
            log_message(log_buf);
            break;
        }
    }
}

void cmd_join(int i, const char *buffer) {
    char nom_salon[MAX_SALON_LEN] = {0};
    char password[MAX_PSEUDO_LEN] = {0};
    char msg[BUFFER_SIZE];
    char args[BUFFER_SIZE];
    strncpy(args, buffer + 6, BUFFER_SIZE - 1);
    char *space = strchr(args, ' ');
    if (space) {
        strncpy(nom_salon, args, space - args);
        strncpy(password, space + 1, MAX_PSEUDO_LEN - 1);
    } else {
        strncpy(nom_salon, args, MAX_SALON_LEN - 1);
    }
    int idx = find_salon(nom_salon);
    if (idx == -1) {
        snprintf(msg, BUFFER_SIZE, "Salon '%s' introuvable.\n", nom_salon);
        send_to(i, msg);
        return;
    }
    if (strlen(salons[idx].password) > 0 &&
        strcmp(salons[idx].password, password) != 0) {
        send_to(i, "Mot de passe incorrect.\n");
        return;
    }
    if (strlen(clients[i].salon) > 0) {
        char ancien[MAX_SALON_LEN];
        strncpy(ancien, clients[i].salon, MAX_SALON_LEN);
        memset(clients[i].salon, 0, MAX_SALON_LEN);
        detruire_salon_si_vide(ancien);
    }
    strncpy(clients[i].salon, nom_salon, MAX_SALON_LEN - 1);
    snprintf(msg, BUFFER_SIZE, "Vous avez rejoint le salon '%s'.\n", nom_salon);
    send_to(i, msg);
    char notif[BUFFER_SIZE];
    snprintf(notif, BUFFER_SIZE, "*** %s a rejoint le salon ***\n",
             clients[i].pseudo);
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (clients[j].socket_fd != -1 && j != i &&
            strcmp(clients[j].salon, nom_salon) == 0)
            send_to(j, notif);
    }
}

void cmd_leave(int i) {
    char msg[BUFFER_SIZE];
    if (strlen(clients[i].salon) == 0) {
        send_to(i, "Vous n'êtes dans aucun salon.\n");
        return;
    }
    char nom_salon[MAX_SALON_LEN];
    strncpy(nom_salon, clients[i].salon, MAX_SALON_LEN);
    char notif[BUFFER_SIZE];
    snprintf(notif, BUFFER_SIZE, "*** %s a quitté le salon ***\n",
             clients[i].pseudo);
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (clients[j].socket_fd != -1 && j != i &&
            strcmp(clients[j].salon, nom_salon) == 0)
            send_to(j, notif);
    }
    memset(clients[i].salon, 0, MAX_SALON_LEN);
    detruire_salon_si_vide(nom_salon);
    snprintf(msg, BUFFER_SIZE, "Vous avez quitté le salon '%s'.\n", nom_salon);
    send_to(i, msg);
}

void cmd_list(int i) {
    char msg[BUFFER_SIZE];
    if (nb_salons == 0) {
        send_to(i, "Aucun salon disponible.\n");
        return;
    }
    strcpy(msg, "=== Salons disponibles ===\n");
    for (int j = 0; j < MAX_SALONS; j++) {
        if (salons[j].actif) {
            char line[BUFFER_SIZE];
            if (strlen(salons[j].password) > 0)
                snprintf(line, BUFFER_SIZE, "  - %s [protégé]\n", salons[j].nom);
            else
                snprintf(line, BUFFER_SIZE, "  - %s\n", salons[j].nom);
            strncat(msg, line, BUFFER_SIZE - strlen(msg) - 1);
        }
    }
    strncat(msg, "==========================\n", BUFFER_SIZE - strlen(msg) - 1);
    send_to(i, msg);
}

void cmd_kick(int i, const char *buffer) {
    char target[MAX_PSEUDO_LEN];
    strncpy(target, buffer + 6, MAX_PSEUDO_LEN - 1);
    char msg[BUFFER_SIZE];
    if (strlen(target) == 0) {
        send_to(i, "Usage : /kick <pseudo>\n");
        return;
    }
    int idx = find_client_by_pseudo(target);
    if (idx == -1) {
        snprintf(msg, BUFFER_SIZE, "Utilisateur '%s' introuvable.\n", target);
        send_to(i, msg);
        return;
    }
    if (idx == i) {
        send_to(i, "Vous ne pouvez pas vous expulser vous-même.\n");
        return;
    }
    send_to(idx, "Vous avez été expulsé du serveur par un modérateur.\n");
    char notif[BUFFER_SIZE];
    snprintf(notif, BUFFER_SIZE, "*** %s a été expulsé du serveur ***\n", target);
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (clients[j].socket_fd != -1 && j != idx)
            send_to(j, notif);
    }
    char log_buf[BUFFER_SIZE];
    snprintf(log_buf, BUFFER_SIZE, "%s a expulsé %s", clients[i].pseudo, target);
    log_message(log_buf);
    disconnect_client(idx);
    snprintf(msg, BUFFER_SIZE, "'%s' a été expulsé avec succès.\n", target);
    send_to(i, msg);
}

void cmd_away(int i, const char *buffer) {
    if (strlen(buffer) <= 6) {
        memset(clients[i].away_msg, 0, BUFFER_SIZE);
        send_to(i, "Vous êtes maintenant disponible.\n");
    } else {
        strncpy(clients[i].away_msg, buffer + 6, BUFFER_SIZE - 1);
        char away_confirm[2 * BUFFER_SIZE];
        snprintf(away_confirm, sizeof(away_confirm),
                 "Message d'absence défini : \"%s\"\n", clients[i].away_msg);
        send_to(i, away_confirm);
    }
}

void cmd_stats(int i) {
    char msg[BUFFER_SIZE];
    time_t now     = time(NULL);
    long   uptime  = (long)(now - server_start);
    long   heures  = uptime / 3600;
    long   minutes = (uptime % 3600) / 60;
    long   secondes = uptime % 60;
    snprintf(msg, BUFFER_SIZE,
             "=== Statistiques du serveur ===\n"
             "  Uptime          : %ldh %ldm %lds\n"
             "  Clients actifs  : %d / %d\n"
             "  Total connexions: %d\n"
             "  Messages total  : %d\n"
             "  Salons actifs   : %d / %d\n"
             "===============================\n",
             heures, minutes, secondes,
             nb_clients, MAX_CLIENTS,
             total_connexions,
             total_messages,
             nb_salons, MAX_SALONS);
    send_to(i, msg);
}

void cmd_help(int i) {
    const char *msg =
        "========= Commandes disponibles =========\n"
        " /nick <pseudo>              Changer de pseudo\n"
        " /who                        Liste des connectés\n"
        " /whois <pseudo>             Infos sur un utilisateur\n"
        " /msg <pseudo> <msg>         Message privé\n"
        " /create <salon> [mdp]       Créer un salon\n"
        " /join <salon> [mdp]         Rejoindre un salon\n"
        " /leave                      Quitter le salon actuel\n"
        " /list                       Liste des salons\n"
        " /msgsalon <msg>             Message dans le salon\n"
        " /kick <pseudo>              Expulser un utilisateur\n"
        " /away [message]             Activer/désactiver absence\n"
        " /stats                      Statistiques du serveur\n"
        " /sendfile <pseudo> <fich>   Envoyer un fichier\n"
        " /accept                     Accepter un fichier\n"
        " /reject                     Refuser un fichier\n"
        " /quit                       Se déconnecter\n"
        "=========================================\n";
    send_to(i, msg);
}

void cmd_sendfile(int i, const char *buffer) {
    char target[MAX_PSEUDO_LEN] = {0}, filename[MAX_FILENAME] = {0};
    char msg[BUFFER_SIZE];
    const char *args  = buffer + 10;
    const char *space = strchr(args, ' ');
    if (!space) {
        send_to(i, "Usage : /sendfile <pseudo> <fichier>\n");
        return;
    }
    strncpy(target,   args, space - args);
    strncpy(filename, space + 1, MAX_FILENAME - 1);
    int idx = find_client_by_pseudo(target);
    if (idx == -1) {
        snprintf(msg, BUFFER_SIZE, "Utilisateur '%s' introuvable.\n", target);
        send_to(i, msg);
    } else if (transfers[idx].pending) {
        send_to(i, "Cet utilisateur a déjà un transfert en attente.\n");
    } else {
        transfers[idx].sender_idx   = i;
        transfers[idx].receiver_idx = idx;
        strncpy(transfers[idx].filename, filename, MAX_FILENAME - 1);
        transfers[idx].pending = 1;
        snprintf(msg, BUFFER_SIZE,
                 "[%s] veut vous envoyer '%s'. Tapez /accept ou /reject.\n",
                 clients[i].pseudo, filename);
        send_to(idx, msg);
        snprintf(msg, BUFFER_SIZE,
                 "Demande envoyée à '%s'. En attente...\n", target);
        send_to(i, msg);
    }
}

void cmd_accept(int i) {
    char msg[BUFFER_SIZE];
    if (!transfers[i].pending) {
        send_to(i, "Aucun transfert en attente.\n");
        return;
    }
    int sender = transfers[i].sender_idx;
    snprintf(msg, BUFFER_SIZE, "FILE_SEND %s\n", transfers[i].filename);
    send_to(sender, msg);
    snprintf(msg, BUFFER_SIZE,
             "'%s' a accepté. Envoyez le contenu avec /file <contenu>\n",
             clients[i].pseudo);
    send_to(sender, msg);
    snprintf(msg, BUFFER_SIZE, "Vous avez accepté '%s'.\n",
             transfers[i].filename);
    send_to(i, msg);
}

void cmd_reject(int i) {
    char msg[BUFFER_SIZE];
    if (!transfers[i].pending) {
        send_to(i, "Aucun transfert en attente.\n");
        return;
    }
    int sender = transfers[i].sender_idx;
    snprintf(msg, BUFFER_SIZE, "'%s' a refusé '%s'.\n",
             clients[i].pseudo, transfers[i].filename);
    send_to(sender, msg);
    snprintf(msg, BUFFER_SIZE, "Vous avez refusé '%s'.\n",
             transfers[i].filename);
    send_to(i, msg);
    transfers[i].pending = 0;
}

void cmd_file(int i, const char *buffer) {
    char msg[BUFFER_SIZE];
    int found = 0;
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (transfers[j].pending && transfers[j].sender_idx == i) {
            snprintf(msg, BUFFER_SIZE, "FICHIER REÇU de '%s' : %s\n",
                     clients[i].pseudo, buffer + 6);
            send_to(j, msg);
            snprintf(msg, BUFFER_SIZE,
                     "Fichier '%s' envoyé avec succès à '%s'.\n",
                     transfers[j].filename, clients[j].pseudo);
            send_to(i, msg);
            char log_buf[BUFFER_SIZE];
            snprintf(log_buf, BUFFER_SIZE, "Fichier '%s' : %s -> %s",
                     transfers[j].filename,
                     clients[i].pseudo, clients[j].pseudo);
            log_message(log_buf);
            transfers[j].pending = 0;
            found = 1;
            break;
        }
    }
    if (!found)
        send_to(i, "Aucun transfert en cours.\n");
}

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int max_fd, activity;

    server_start = time(NULL);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket_fd   = -1;
        clients[i].nb_messages = 0;
        memset(clients[i].pseudo,   0, MAX_PSEUDO_LEN);
        memset(clients[i].salon,    0, MAX_SALON_LEN);
        memset(clients[i].away_msg, 0, BUFFER_SIZE);
        transfers[i].pending = 0;
    }
    for (int i = 0; i < MAX_SALONS; i++) {
        salons[i].actif = 0;
        memset(salons[i].nom,      0, MAX_SALON_LEN);
        memset(salons[i].password, 0, MAX_PSEUDO_LEN);
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("Erreur socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    printf("✔ Socket créée\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur bind"); close(server_fd); exit(EXIT_FAILURE);
    }
    printf("✔ Bind effectué sur le port %d\n", PORT);

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Erreur listen"); close(server_fd); exit(EXIT_FAILURE);
    }
    printf("✔ Serveur en écoute (max %d clients)...\n", MAX_CLIENTS);
    log_message("Serveur démarré");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket_fd != -1) {
                FD_SET(clients[i].socket_fd, &readfds);
                if (clients[i].socket_fd > max_fd)
                    max_fd = clients[i].socket_fd;
            }
        }

        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) { perror("Erreur select"); break; }

        if (FD_ISSET(server_fd, &readfds)) {
            client_fd = accept(server_fd,
                               (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) { perror("Erreur accept"); continue; }

            if (nb_clients >= MAX_CLIENTS) {
                const char *msg =
                    "Server cannot accept incoming connections anymore. "
                    "Try again later.\n";
                send(client_fd, msg, strlen(msg), 0);
                close(client_fd);
            } else {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket_fd == -1) {
                        clients[i].socket_fd   = client_fd;
                        clients[i].nb_messages = 0;
                        strncpy(clients[i].pseudo, "anonyme", MAX_PSEUDO_LEN);
                        memset(clients[i].salon,    0, MAX_SALON_LEN);
                        memset(clients[i].away_msg, 0, BUFFER_SIZE);
                        transfers[i].pending = 0;
                        nb_clients++;
                        total_connexions++;
                        printf("✔ Nouveau client (socket %d) — %d/%d\n",
                               client_fd, nb_clients, MAX_CLIENTS);
                        log_message("Nouveau client connecté");
                        char notif[BUFFER_SIZE];
                        snprintf(notif, BUFFER_SIZE,
                                 "*** Un nouveau client a rejoint le serveur (%d/%d) ***\n",
                                 nb_clients, MAX_CLIENTS);
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].socket_fd != -1 && j != i)
                                send_to(j, notif);
                        }
                        send_to(i, "Bienvenue ! Identifiez-vous avec /nick <pseudo>\n");
                        send_to(i, "Tapez /help pour voir les commandes.\n");
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket_fd == -1 ||
                !FD_ISSET(clients[i].socket_fd, &readfds))
                continue;

            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(clients[i].socket_fd, buffer, BUFFER_SIZE - 1, 0);

            if (bytes <= 0) {
                printf("Client %s déconnecté\n", clients[i].pseudo);
                char log_buf[BUFFER_SIZE];
                snprintf(log_buf, BUFFER_SIZE, "%s déconnecté", clients[i].pseudo);
                log_message(log_buf);
                char notif[BUFFER_SIZE];
                snprintf(notif, BUFFER_SIZE, "*** %s a quitté le chat ***\n",
                         clients[i].pseudo);
                if (strlen(clients[i].salon) > 0) {
                    char nom_salon[MAX_SALON_LEN];
                    strncpy(nom_salon, clients[i].salon, MAX_SALON_LEN);
                    memset(clients[i].salon, 0, MAX_SALON_LEN);
                    detruire_salon_si_vide(nom_salon);
                }
                disconnect_client(i);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j].socket_fd != -1)
                        send_to(j, notif);
                }
                continue;
            }

            buffer[strcspn(buffer, "\n")] = '\0';
            printf("[%s] : %s\n", clients[i].pseudo, buffer);

            char log_buf[2 * BUFFER_SIZE];
            snprintf(log_buf, sizeof(log_buf), "[%s] : %s", clients[i].pseudo, buffer);
            log_message(log_buf);

            if      (strncmp(buffer, "/nick ",    6)  == 0) cmd_nick(i, buffer);
            else if (strcmp (buffer, "/who")          == 0) cmd_who(i);
            else if (strncmp(buffer, "/whois ",   7)  == 0) cmd_whois(i, buffer);
            else if (strncmp(buffer, "/msg ",     5)  == 0) {
                char target[MAX_PSEUDO_LEN], message[BUFFER_SIZE];
                sscanf(buffer + 5, "%49s %[^\n]", target, message);
                unicast(i, target, message);
            }
            else if (strncmp(buffer, "/create ",  8)  == 0) cmd_create(i, buffer);
            else if (strncmp(buffer, "/join ",    6)  == 0) cmd_join(i, buffer);
            else if (strcmp (buffer, "/leave")        == 0) cmd_leave(i);
            else if (strcmp (buffer, "/list")         == 0) cmd_list(i);
            else if (strncmp(buffer, "/msgsalon ", 10) == 0) {
                if (strlen(clients[i].salon) == 0)
                    send_to(i, "Vous n'êtes dans aucun salon.\n");
                else
                    multicast(i, buffer + 10);
            }
            else if (strncmp(buffer, "/kick ",    6)  == 0) cmd_kick(i, buffer);
            else if (strncmp(buffer, "/away",     5)  == 0) cmd_away(i, buffer);
            else if (strcmp (buffer, "/stats")        == 0) cmd_stats(i);
            else if (strcmp (buffer, "/help")         == 0) cmd_help(i);
            else if (strncmp(buffer, "/sendfile ", 10) == 0) cmd_sendfile(i, buffer);
            else if (strcmp (buffer, "/accept")       == 0) cmd_accept(i);
            else if (strcmp (buffer, "/reject")       == 0) cmd_reject(i);
            else if (strncmp(buffer, "/file ",    6)  == 0) cmd_file(i, buffer);
            else if (strcmp (buffer, "/quit")         == 0) {
                send_to(i, "You will be terminated\n");
                char notif[BUFFER_SIZE];
                snprintf(notif, BUFFER_SIZE, "*** %s a quitté le chat ***\n",
                         clients[i].pseudo);
                if (strlen(clients[i].salon) > 0) {
                    char nom_salon[MAX_SALON_LEN];
                    strncpy(nom_salon, clients[i].salon, MAX_SALON_LEN);
                    memset(clients[i].salon, 0, MAX_SALON_LEN);
                    detruire_salon_si_vide(nom_salon);
                }
                disconnect_client(i);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j].socket_fd != -1)
                        send_to(j, notif);
                }
            }
            else {
                broadcast(i, buffer);
            }
        }
    }

    close(server_fd);
    return 0;
}