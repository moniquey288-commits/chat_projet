/**
 * @file    client.c
 * @brief   Version 2 - Client simple
 * @author  [Votre Nom]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if (argc != 3) {
        printf("Usage: ./client <port> <adresse_IP>\n");
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    char *ip = argv[2];

    // 1. Créer la socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }
    printf("✔ Socket créée\n");

    // 2. Configurer l'adresse
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Adresse IP invalide");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Se connecter
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("✔ Connecté au serveur %s:%d\n", ip, port);

    // 4. Boucle d'échange
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        send(client_fd, buffer, strlen(buffer), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            printf("Connexion fermée.\n");
            break;
        }
        printf("[Server] : %s\n", buffer);

        if (strncmp(buffer, "You will be terminated", 22) == 0) {
            printf("Connection terminated\n");
            break;
        }
    }

    close(client_fd);
    printf("✔ Socket fermée.\n");
    return 0;
}