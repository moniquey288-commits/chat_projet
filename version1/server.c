/**
 * @file    server.c
 * @brief   Version 1 - Serveur de base (echo)
 * @author  [Votre Nom]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

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
    if (listen(server_fd, 1) < 0) {
        perror("Erreur listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("✔ Serveur en écoute...\n");

    // 5. Accepter un client
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Erreur accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("✔ Client connecté !\n");

    // 6. Boucle d'échange
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            printf("Client déconnecté.\n");
            break;
        }
        printf("[Client] : %s\n", buffer);

        if (strncmp(buffer, "/quit", 5) == 0) {
            char *msg = "You will be terminated\n";
            send(client_fd, msg, strlen(msg), 0);
            printf("Connexion fermée.\n");
            break;
        }
        send(client_fd, buffer, strlen(buffer), 0);
    }

    // 7. Fermer
    close(client_fd);
    close(server_fd);
    printf("✔ Sockets fermées.\n");
    return 0;
}