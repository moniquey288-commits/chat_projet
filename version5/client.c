/**
 * @file    client.c
 * @author  Maïmounata (Université Joseph Ki-Zerbo)
 * @date    2024
 * @brief   Client TCP pour l'application de chat multi-utilisateurs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE     1024
#define MAX_PSEUDO_LEN  50
#define PROMPT          "> "

int client_fd  = -1;
int running    = 1;
char my_pseudo[MAX_PSEUDO_LEN] = "anonyme";

void print_prompt(void) {
    printf("%s", PROMPT);
    fflush(stdout);
}

void update_local_pseudo(const char *buffer) {
    if (strncmp(buffer, "/nick ", 6) == 0) {
        strncpy(my_pseudo, buffer + 6, MAX_PSEUDO_LEN - 1);
        my_pseudo[MAX_PSEUDO_LEN - 1] = '\0';
    }
}

void handle_sigint(int sig) {
    (void)sig;
    printf("\n[Client] Déconnexion en cours...\n");
    if (client_fd != -1) {
        const char *quit_msg = "/quit";
        send(client_fd, quit_msg, strlen(quit_msg), 0);
    }
    running = 0;
}

void *receive_messages(void *arg) {
    (void)arg;
    char buffer[BUFFER_SIZE];

    while (running) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0) {
            if (running) {
                printf("\n[Serveur] Connexion fermée par le serveur.\n");
            }
            running = 0;
            break;
        }

        printf("\r%s\n", buffer);
        fflush(stdout);
        print_prompt();

        if (strncmp(buffer, "You will be terminated", 22) == 0) {
            printf("[Client] Connection terminated\n");
            running = 0;
            break;
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    pthread_t recv_thread;

    if (argc != 3) {
        fprintf(stderr, "Usage : %s <port> <adresse_IP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int   port = atoi(argv[1]);
    char *ip   = argv[2];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Erreur : port invalide\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_sigint);

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }
    printf("✔ Socket créée\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Erreur : adresse IP invalide\n");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connexion à %s:%d ...\n", ip, port);
    if (connect(client_fd,
                (struct sockaddr*)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("Erreur connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("✔ Connecté au serveur %s:%d\n", ip, port);
    printf("─────────────────────────────────────\n");
    printf("  Tapez /nick <pseudo> pour vous identifier\n");
    printf("  Tapez /help pour voir les commandes\n");
    printf("  Tapez /quit pour quitter\n");
    printf("─────────────────────────────────────\n");

    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        perror("Erreur pthread_create");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    print_prompt();
    while (running) {
        memset(buffer, 0, BUFFER_SIZE);

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) break;

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0) {
            print_prompt();
            continue;
        }

        update_local_pseudo(buffer);

        if (send(client_fd, buffer, strlen(buffer), 0) < 0) {
            perror("Erreur send");
            break;
        }

        if (strcmp(buffer, "/quit") == 0) {
            running = 0;
            break;
        }

        print_prompt();
    }

    running = 0;
    pthread_join(recv_thread, NULL);
    close(client_fd);
    client_fd = -1;
    printf("\n✔ Déconnecté. À bientôt, %s !\n", my_pseudo);

    return EXIT_SUCCESS;
}