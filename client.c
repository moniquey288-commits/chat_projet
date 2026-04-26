/**
 * @file    client.c
 * @author  Maïmounata (Université Joseph Ki-Zerbo)
 * @date    2024
 * @brief   Client TCP pour l'application de chat multi-utilisateurs.
 *
 * Utilise un thread POSIX pour la réception des messages du serveur
 * en parallèle de la saisie utilisateur (stdin).
 *
 * Compilation :
 *   gcc -o client client.c -lpthread
 *
 * Utilisation :
 *   ./client <port> <adresse_IP>
 *   ./client 8080 127.0.0.1
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

/* =========================================================
 *  CONSTANTES
 * ========================================================= */
#define BUFFER_SIZE     1024   /**< Taille maximale d'un message             */
#define MAX_PSEUDO_LEN  50     /**< Longueur maximale d'un pseudo            */
#define PROMPT          "> "   /**< Invite de saisie affichée à l'utilisateur */

/* =========================================================
 *  VARIABLES GLOBALES
 * ========================================================= */
int client_fd  = -1;  /**< Descripteur de socket du client                  */
int running    = 1;   /**< Indicateur de boucle active (0 = arrêt demandé)  */
char my_pseudo[MAX_PSEUDO_LEN] = "anonyme"; /**< Pseudo local du client     */

/* =========================================================
 *  UTILITAIRES
 * ========================================================= */

/**
 * @brief Affiche l'invite de saisie (prompt) sans retour à la ligne.
 *        Utilisé pour réafficher le prompt après un message reçu.
 */
void print_prompt(void) {
    printf("%s", PROMPT);
    fflush(stdout);
}

/**
 * @brief Extrait le pseudo depuis une commande /nick.
 *        Met à jour my_pseudo si la commande est valide.
 * @param buffer Ligne de commande saisie par l'utilisateur.
 */
void update_local_pseudo(const char *buffer) {
    if (strncmp(buffer, "/nick ", 6) == 0) {
        strncpy(my_pseudo, buffer + 6, MAX_PSEUDO_LEN - 1);
        my_pseudo[MAX_PSEUDO_LEN - 1] = '\0';
    }
}

/**
 * @brief Gestionnaire de signal SIGINT (Ctrl+C).
 *        Envoie /quit au serveur avant de fermer proprement.
 * @param sig Numéro du signal reçu (non utilisé).
 */
void handle_sigint(int sig) {
    (void)sig; /* Éviter le warning "unused parameter" */
    printf("\n[Client] Déconnexion en cours...\n");
    if (client_fd != -1) {
        const char *quit_msg = "/quit";
        send(client_fd, quit_msg, strlen(quit_msg), 0);
    }
    running = 0;
}

/* =========================================================
 *  THREAD DE RÉCEPTION
 * ========================================================= */

/**
 * @brief Fonction exécutée par le thread de réception.
 *
 * Écoute en permanence les messages entrants depuis le serveur
 * et les affiche sur stdout. Gère la déconnexion propre en cas
 * de fermeture de connexion ou de message de terminaison.
 *
 * @param arg Non utilisé (requis par l'API pthread).
 * @return NULL dans tous les cas.
 */
void *receive_messages(void *arg) {
    (void)arg; /* Éviter le warning "unused parameter" */
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

        /* Effacer le prompt actuel, afficher le message, réafficher le prompt */
        printf("\r%s\n", buffer);
        fflush(stdout);
        print_prompt();

        /* Détecter la terminaison demandée par le serveur */
        if (strncmp(buffer, "You will be terminated", 22) == 0) {
            printf("[Client] Connection terminated\n");
            running = 0;
            break;
        }
    }

    return NULL;
}

/* =========================================================
 *  MAIN
 * ========================================================= */

/**
 * @brief Point d'entrée du client de chat.
 *
 * Crée la socket, se connecte au serveur, lance le thread de
 * réception, puis entre dans la boucle de saisie utilisateur.
 *
 * @param argc Nombre d'arguments (attendu : 3).
 * @param argv Tableau d'arguments : argv[1] = port, argv[2] = IP.
 * @return EXIT_SUCCESS en cas de succès, EXIT_FAILURE sinon.
 */
int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    pthread_t recv_thread;

    /* --- Vérification des arguments --- */
    if (argc != 3) {
        fprintf(stderr, "Usage : %s <port> <adresse_IP>\n", argv[0]);
        fprintf(stderr, "Exemple : %s 8080 127.0.0.1\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int   port = atoi(argv[1]);
    char *ip   = argv[2];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Erreur : port invalide (%s). Plage : 1–65535\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* --- Gestionnaire Ctrl+C --- */
    signal(SIGINT, handle_sigint);

    /* --- 1. Création de la socket --- */
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }
    printf("✔ Socket créée\n");

    /* --- 2. Configuration de l'adresse serveur --- */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Erreur : adresse IP invalide (%s)\n", ip);
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    /* --- 3. Connexion au serveur --- */
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

    /* --- 4. Lancement du thread de réception --- */
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        perror("Erreur pthread_create");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    /* --- 5. Boucle de saisie utilisateur --- */
    print_prompt();
    while (running) {
        memset(buffer, 0, BUFFER_SIZE);

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            /* EOF (Ctrl+D) — déconnexion propre */
            break;
        }

        /* Supprimer le \n final */
        buffer[strcspn(buffer, "\n")] = '\0';

        /* Ignorer les lignes vides */
        if (strlen(buffer) == 0) {
            print_prompt();
            continue;
        }

        /* Mettre à jour le pseudo local si /nick */
        update_local_pseudo(buffer);

        /* Envoyer la commande ou le message au serveur */
        if (send(client_fd, buffer, strlen(buffer), 0) < 0) {
            perror("Erreur send");
            break;
        }

        /* /quit : on arrête la boucle localement aussi */
        if (strcmp(buffer, "/quit") == 0) {
            running = 0;
            break;
        }

        print_prompt();
    }

    /* --- 6. Fermeture propre --- */
    running = 0;
    pthread_join(recv_thread, NULL);
    close(client_fd);
    client_fd = -1;
    printf("\n✔ Déconnecté. À bientôt, %s !\n", my_pseudo);

    return EXIT_SUCCESS;
}