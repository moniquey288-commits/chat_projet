============================================
VERSION 4 - Application de Chat
============================================

DESCRIPTION :
Quatrieme version. Application de chat
complete avec broadcast, messages prives
et salons de discussion.

FONCTIONNALITES :
- Broadcast : message a tous
- Unicast : message prive
- Multicast : message dans un salon
- Creation et gestion de salons
- Destruction automatique du salon vide
- Liste des salons avec /list

COMPILATION :
    make

UTILISATION :
    Terminal 1 : ./server
    Terminal 2 : ./client 8080 127.0.0.1

COMMANDES :
    /nick <pseudo>          : Choisir son pseudo
    /who                    : Liste des connectes
    /whois <pseudo>         : Infos utilisateur
    /msg <pseudo> <message> : Message prive
    /create <salon>         : Creer un salon
    /join <salon>           : Rejoindre un salon
    /leave                  : Quitter le salon
    /list                   : Liste des salons
    /msgsalon <message>     : Message dans salon
    /quit                   : Se deconnecter

AUTEUR :
    Nom        : Yameogo Monique dit Maimounata
    Universite : Universite Joseph Ki-Zerbo
    Cours      : Programmation Systeme 
    Enseignant : Assane Ilboudo
============================================