/**
 * @file    server.c
 * @brief   Version 4 - Application de Chat (broadcast, unicast, multicast, salons)
 * @author  [Votre Nom]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 20
#define MAX_SALONS 10

typedef struct {
    int socket_fd;
    char pseudo[50];
    char salon[50];
} Client;

typedef struct {
    char nom[50];
    int actif;
} Salon;

Client clients[MAX_CLIENTS];
Salon salons[MAX_SALONS];
int nb_clients = 0;
int nb_salons = 0;

int find_client_by_pseudo(char *pseudo) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd != -1 &&
            strcmp(clients[i].pseudo, pseudo) == 0)
            return i;
    }
    return -1;
}

int find_salon(char *nom) {
    for (int i = 0; i < MAX_SALONS; i++) {
        if (salons[i].actif && strcmp(salons[i].nom, nom) == 0)
            return i;
    }
    return -1;
}

void broadcast(int sender_idx, char *message) {
    char msg[BUFFER_SIZE];
    snprintf(msg, BUFFER_SIZE, "[%s] : %s\n", clients[sender_idx].pseudo, message);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd != -1 && i != sender_idx)
            send(clients[i].socket_fd, msg, strlen(msg), 0);
    }
}

void unicast(int sender_idx, char *target_pseudo, char *message) {
    int idx = find_client_by_pseudo(target_pseudo);
    char msg[BUFFER_SIZE];
    if (idx == -1) {
        snprintf(msg, BUFFER_SIZE, "Utilisateur '%s' introuvable.\n", target_pseudo);
        send(clients[sender_idx].socket_fd, msg, strlen(msg), 0);
    } else {
        snprintf(msg, BUFFER_SIZE, "[Privé de %s] : %s\n", clients[sender_idx].pseudo, message);
        send(clients[idx].socket_fd, msg, strlen(msg), 0);
        snprintf(msg, BUFFER_SIZE, "[Privé à %s] : %s\n", target_pseudo, message);
        send(clients[sender_idx].socket_fd, msg, strlen(msg), 0);
    }
}

void multicast(int sender_idx, char *message) {
    char msg[BUFFER_SIZE];
    snprintf(msg, BUFFER_SIZE, "[Salon %s][%s] : %s\n",
             clients[sender_idx].salon,
             clients[sender_idx].pseudo, message);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd != -1 && i != sender_idx &&
            strcmp(clients[i].salon, clients[sender_idx].salon) == 0)
            send(clients[i].socket_fd, msg, strlen(msg), 0);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int max_fd, activity, i;

    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket_fd = -1;
        memset(clients[i].pseudo, 0, 50);
        memset(clients[i].salon, 0, 50);
    }
    for (i = 0; i < MAX_SALONS; i++) {
        salons[i].actif = 0;
        memset(salons[i].nom, 0, 50);
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("Erreur socket"); exit(EXIT_FAILURE); }
    printf("✔ Socket créée\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur bind"); close(server_fd); exit(EXIT_FAILURE);
    }
    printf("✔ Bind effectué sur le port %d\n", PORT);

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Erreur listen"); close(server_fd); exit(EXIT_FAILURE);
    }
    printf("✔ Serveur en écoute (max %d clients)...\n", MAX_CLIENTS);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket_fd != -1) {
                FD_SET(clients[i].socket_fd, &readfds);
                if (clients[i].socket_fd > max_fd)
                    max_fd = clients[i].socket_fd;
            }
        }

        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) { perror("Erreur select"); break; }

        if (FD_ISSET(server_fd, &readfds)) {
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) { perror("Erreur accept"); continue; }

            if (nb_clients >= MAX_CLIENTS) {
                char *msg = "Server cannot accept incoming connections anymore. Try again later.\n";
                send(client_fd, msg, strlen(msg), 0);
                close(client_fd);
            } else {
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket_fd == -1) {
                        clients[i].socket_fd = client_fd;
                        strcpy(clients[i].pseudo, "anonyme");
                        memset(clients[i].salon, 0, 50);
                        nb_clients++;
                        printf("✔ Client connecté (socket %d) %d/%d\n",
                               client_fd, nb_clients, MAX_CLIENTS);
                        char *msg = "Bienvenue ! Identifiez-vous avec /nick <pseudo>\n";
                        send(client_fd, msg, strlen(msg), 0);
                        break;
                    }
                }
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket_fd != -1 &&
                FD_ISSET(clients[i].socket_fd, &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes = recv(clients[i].socket_fd, buffer, BUFFER_SIZE - 1, 0);

                if (bytes <= 0) {
                    printf("Client %s déconnecté\n", clients[i].pseudo);
                    close(clients[i].socket_fd);
                    clients[i].socket_fd = -1;
                    memset(clients[i].pseudo, 0, 50);
                    memset(clients[i].salon, 0, 50);
                    nb_clients--;
                } else {
                    buffer[strcspn(buffer, "\n")] = 0;
                    printf("[%s] : %s\n", clients[i].pseudo, buffer);

                    // /nick
                    if (strncmp(buffer, "/nick ", 6) == 0) {
                        char new_pseudo[50];
                        strcpy(new_pseudo, buffer + 6);
                        if (find_client_by_pseudo(new_pseudo) != -1) {
                            char msg[BUFFER_SIZE];
                            snprintf(msg, BUFFER_SIZE, "Pseudo '%s' déjà utilisé.\n", new_pseudo);
                            send(clients[i].socket_fd, msg, strlen(msg), 0);
                        } else {
                            strcpy(clients[i].pseudo, new_pseudo);
                            char msg[BUFFER_SIZE];
                            snprintf(msg, BUFFER_SIZE, "Pseudo changé en '%s'\n", new_pseudo);
                            send(clients[i].socket_fd, msg, strlen(msg), 0);
                        }
                    }
                    // /who
                    else if (strcmp(buffer, "/who") == 0) {
                        char msg[BUFFER_SIZE];
                        strcpy(msg, "Utilisateurs connectés :\n");
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].socket_fd != -1) {
                                strcat(msg, "- ");
                                strcat(msg, clients[j].pseudo);
                                strcat(msg, "\n");
                            }
                        }
                        send(clients[i].socket_fd, msg, strlen(msg), 0);
                    }
                    // /whois
                    else if (strncmp(buffer, "/whois ", 7) == 0) {
                        char target[50];
                        strcpy(target, buffer + 7);
                        int idx = find_client_by_pseudo(target);
                        char msg[BUFFER_SIZE];
                        if (idx == -1) {
                            snprintf(msg, BUFFER_SIZE, "Utilisateur '%s' introuvable.\n", target);
                        } else {
                            snprintf(msg, BUFFER_SIZE, "Utilisateur '%s' connecté sur socket %d\n",
                                     clients[idx].pseudo, clients[idx].socket_fd);
                        }
                        send(clients[i].socket_fd, msg, strlen(msg), 0);
                    }
                    // /msg
                    else if (strncmp(buffer, "/msg ", 5) == 0) {
                        char target[50], message[BUFFER_SIZE];
                        sscanf(buffer + 5, "%s %[^\n]", target, message);
                        unicast(i, target, message);
                    }
                    // /create
                    else if (strncmp(buffer, "/create ", 8) == 0) {
                        char nom_salon[50];
                        strcpy(nom_salon, buffer + 8);
                        char msg[BUFFER_SIZE];
                        if (find_salon(nom_salon) != -1) {
                            snprintf(msg, BUFFER_SIZE, "Salon '%s' existe déjà.\n", nom_salon);
                            send(clients[i].socket_fd, msg, strlen(msg), 0);
                        } else {
                            for (int j = 0; j < MAX_SALONS; j++) {
                                if (!salons[j].actif) {
                                    strcpy(salons[j].nom, nom_salon);
                                    salons[j].actif = 1;
                                    nb_salons++;
                                    snprintf(msg, BUFFER_SIZE, "Salon '%s' créé.\n", nom_salon);
                                    send(clients[i].socket_fd, msg, strlen(msg), 0);
                                    break;
                                }
                            }
                        }
                    }
                    // /join
                    else if (strncmp(buffer, "/join ", 6) == 0) {
                        char nom_salon[50];
                        strcpy(nom_salon, buffer + 6);
                        char msg[BUFFER_SIZE];
                        if (find_salon(nom_salon) == -1) {
                            snprintf(msg, BUFFER_SIZE, "Salon '%s' introuvable.\n", nom_salon);
                            send(clients[i].socket_fd, msg, strlen(msg), 0);
                        } else {
                            strcpy(clients[i].salon, nom_salon);
                            snprintf(msg, BUFFER_SIZE, "Vous avez rejoint le salon '%s'.\n", nom_salon);
                            send(clients[i].socket_fd, msg, strlen(msg), 0);
                        }
                    }
                    // /list
                    else if (strcmp(buffer, "/list") == 0) {
                        char msg[BUFFER_SIZE];
                        if (nb_salons == 0) {
                            strcpy(msg, "Aucun salon disponible.\n");
                        } else {
                            strcpy(msg, "Salons disponibles :\n");
                            for (int j = 0; j < MAX_SALONS; j++) {
                                if (salons[j].actif) {
                                    strcat(msg, "- ");
                                    strcat(msg, salons[j].nom);
                                    strcat(msg, "\n");
                                }
                            }
                        }
                        send(clients[i].socket_fd, msg, strlen(msg), 0);
                    }
                    // /leave
                    else if (strcmp(buffer, "/leave") == 0) {
                        char msg[BUFFER_SIZE];
                        if (strlen(clients[i].salon) == 0) {
                            snprintf(msg, BUFFER_SIZE, "Vous n'êtes dans aucun salon.\n");
                            send(clients[i].socket_fd, msg, strlen(msg), 0);
                        } else {
                            char nom_salon[50];
                            strcpy(nom_salon, clients[i].salon);
                            memset(clients[i].salon, 0, 50);
                            int salon_vide = 1;
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (clients[j].socket_fd != -1 &&
                                    strcmp(clients[j].salon, nom_salon) == 0) {
                                    salon_vide = 0;
                                    break;
                                }
                            }
                            if (salon_vide) {
                                int idx = find_salon(nom_salon);
                                if (idx != -1) {
                                    salons[idx].actif = 0;
                                    nb_salons--;
                                    printf("Salon '%s' détruit\n", nom_salon);
                                }
                            }
                            snprintf(msg, BUFFER_SIZE, "Vous avez quitté le salon '%s'.\n", nom_salon);
                            send(clients[i].socket_fd, msg, strlen(msg), 0);
                        }
                    }
                    // /msgsalon
                    else if (strncmp(buffer, "/msgsalon ", 10) == 0) {
                        char msg[BUFFER_SIZE];
                        if (strlen(clients[i].salon) == 0) {
                            snprintf(msg, BUFFER_SIZE, "Vous n'êtes dans aucun salon.\n");
                            send(clients[i].socket_fd, msg, strlen(msg), 0);
                        } else {
                            multicast(i, buffer + 10);
                        }
                    }
                    // /quit
                    else if (strcmp(buffer, "/quit") == 0) {
                        char *msg = "You will be terminated\n";
                        send(clients[i].socket_fd, msg, strlen(msg), 0);
                        close(clients[i].socket_fd);
                        clients[i].socket_fd = -1;
                        memset(clients[i].pseudo, 0, 50);
                        memset(clients[i].salon, 0, 50);
                        nb_clients--;
                    }
                    // Broadcast
                    else {
                        broadcast(i, buffer);
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}