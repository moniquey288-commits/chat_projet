/**
 * @file    server.c
 * @brief   Version 2 - Serveur Multi-clients (max 20)
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

int main() {
    int server_fd, client_fd;
    int clients[MAX_CLIENTS];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int max_fd, activity, i, nb_clients = 0;

    // Initialiser le tableau des clients
    for (i = 0; i < MAX_CLIENTS; i++)
        clients[i] = -1;

    // 1. Créer la socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }
    printf("✔ Socket créée\n");

    // 2. Configurer l'adresse
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 3. Bind
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("✔ Bind effectué sur le port %d\n", PORT);

    // 4. Listen
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Erreur listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("✔ Serveur en écoute (max %d clients)...\n", MAX_CLIENTS);

    // 5. Boucle principale avec select()
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1) {
                FD_SET(clients[i], &readfds);
                if (clients[i] > max_fd)
                    max_fd = clients[i];
            }
        }

        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Erreur select");
            break;
        }

        // Nouvelle connexion
        if (FD_ISSET(server_fd, &readfds)) {
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                perror("Erreur accept");
                continue;
            }

            if (nb_clients >= MAX_CLIENTS) {
                char *msg = "Server cannot accept incoming connections anymore. Try again later.\n";
                send(client_fd, msg, strlen(msg), 0);
                close(client_fd);
                printf("⚠ Connexion refusée (serveur plein)\n");
            } else {
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] == -1) {
                        clients[i] = client_fd;
                        nb_clients++;
                        printf("✔ Client connecté (socket %d) %d/%d clients\n",
                               client_fd, nb_clients, MAX_CLIENTS);
                        break;
                    }
                }
            }
        }

        // Messages des clients
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != -1 && FD_ISSET(clients[i], &readfds)) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytes = recv(clients[i], buffer, BUFFER_SIZE - 1, 0);

                if (bytes <= 0) {
                    printf("Client (socket %d) déconnecté\n", clients[i]);
                    close(clients[i]);
                    clients[i] = -1;
                    nb_clients--;
                } else {
                    printf("[Client %d] : %s\n", clients[i], buffer);

                    if (strncmp(buffer, "/quit", 5) == 0) {
                        char *msg = "You will be terminated\n";
                        send(clients[i], msg, strlen(msg), 0);
                        close(clients[i]);
                        clients[i] = -1;
                        nb_clients--;
                    } else {
                        // Écho au même client
                        send(clients[i], buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}