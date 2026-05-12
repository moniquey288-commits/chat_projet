============================================
VERSION 2 - Serveur Multi-clients
============================================

DESCRIPTION :
Deuxieme version. Le serveur gere jusqu'a
20 clients simultanement avec select().

FONCTIONNALITES :
- Gestion de 20 clients simultanes
- Utilisation de select()
- Refus du 21eme client
- Echo de messages
- Deconnexion avec /quit

COMPILATION :
    make

UTILISATION :
    Terminal 1 : ./server
    Terminal 2 : ./client 8080 127.0.0.1
    Test 21 clients : for i in $(seq 1 21); do ./client 8080 127.0.0.1 & done

COMMANDES :
    /quit  : Se deconnecter

AUTEUR :
    Nom        : Yameogo Monique dit Maimounata
    Universite : Universite Joseph Ki-Zerbo
    Cours      : Programmation Systeme 
    Enseignant : Assane Ilboudo
============================================