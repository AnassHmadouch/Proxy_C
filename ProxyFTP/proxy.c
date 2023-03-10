#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include "./simpleSocketAPI.h"


#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define FTPPORT21 "21"
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port


int main(){
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    int descSockCOM;                 // Descripteur de socket de communication
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les
                                     // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par
                                      // la fonction getaddrinfo

     // Récupération des informations du serveur
     ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
     if (ecode) {
         fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
         exit(1);
     }
     // Publication de la socket
     ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
     if (ecode == -1) {
         perror("Erreur liaison de la socket de RDV");
         exit(3);
     }
     // Nous n'avons plus besoin de cette liste chainée addrinfo
     freeaddrinfo(res);

     // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
     len=sizeof(struct sockaddr_storage);
     ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
     if (ecode == -1)
     {
         perror("SERVEUR: getsockname");
         exit(4);
     }
     ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN,
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
     if (ecode != 0) {
             fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
             exit(4);
     }
     printf("L'adresse d'ecoute est: %s\n", serverAddr);
     printf("Le port d'ecoute est: %s\n", serverPort);

     // Definition de la taille du tampon contenant les demandes de connexion
     ecode = listen(descSockRDV, LISTENLEN);
     if (ecode == -1) {
         perror("Erreur initialisation buffer d'écoute");
         exit(5);
     }

    len = sizeof(struct sockaddr_storage);
     // Attente connexion du client
     // Lorsque demande de connexion, creation d'une socket de communication avec le client
     descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
     if (descSockCOM == -1){
         perror("Erreur accept\n");
         exit(6);
     }
    // Echange de données avec le client connecté

    /*****
     * Testez de mettre 220 devant BLABLABLA ...
     * **/
    strcpy(buffer, "220 BLABLABLA\n");
    write(descSockCOM, buffer, strlen(buffer));

    //Lecture des indentifiants
    ecode = read(descSockCOM, buffer, MAXBUFFERLEN - 1);
    if (ecode == -1) {
                perror("probleme de lecture\n");
                exit(3);
    }
    buffer[ecode] = '\0';
    printf("Recu du client ftp: %s\n", buffer);

    //Chaine entrée "anonymous@ftp.fau.de"
    char login[50], hostFTP[50];
    // decoupage du message
    sscanf(buffer, "%50[^@]@%50s", login, hostFTP);
    strncat(login, "\n", 49);
    
    int descSockServ;
    ecode = connect2Server(hostFTP, FTPPORT21, &descSockServ);

    ecode=read(descSockServ, buffer, MAXBUFFERLEN-1);
    if (ecode == -1){
        perror("probleme de lecture\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    // Envoie les informations de connexion au serveur
    sprintf(buffer,"%s\r\n",login);
    ecode = write(descSockServ, buffer, strlen(buffer));
    if (ecode == -1){
        perror("Erreur écriture dans socket\n");
        exit(10);
    }
    printf("Message envoyé au serveur : %s\n", buffer);

    // Lit le message envoyé par le serveur
    ecode=read(descSockServ, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){
        perror("probleme de lecture\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    // Envoie le message du serveur au client
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if(ecode == -1){
        perror("Erreur écriture\n");
        exit(10);
    }
    printf("Message envoyé au client : %s\n", buffer);

    // Lit le mot de passe envoyé par le client
    ecode=read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1){
        perror("probleme de lecture\n");
        exit(7);
    }
    buffer[ecode]='\0';
    printf("Message recu : %s\n", buffer);

    // Envoie le mot de passe qui correspond à l'email au serveur
    write(descSockServ, buffer, strlen(buffer));
    printf("Message envoyé au serveur : %s\n", buffer);

    // lit le message envoyé par le serveur
    ecode=read(descSockServ, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){
        perror("probleme de lecture\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    // Envoie le message du serveur "230 Anonymous access granted, restrictions apply" au client
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if(ecode == -1){
        perror("probleme de lecture\n");
        exit(10);
    }
    printf("Message envoyé au client : %s\n", buffer);


    // Lit le message envoyé par le client auto SYST
    ecode=read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1){
        perror("probleme de lecture dans socket\n");
        exit(7);
    }
    buffer[ecode]='\0';
    printf("Message recu : %s\n", buffer);

    // Envoie la requête SYST au serveur
    write(descSockServ, buffer, strlen(buffer));
    printf("Message envoyé au serveur : %s\n", buffer);

    // Lit le message envoyé par le serveur
    ecode=read(descSockServ, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){
        perror("probleme de lecture dans socket\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    // Envoie le message du serveur au client
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if(ecode == -1){
        perror("probleme de lecture dans socket\n");
        exit(10);
    }
    printf("Message envoyé au client : %s\n", buffer);

    // Lit le message envoyé par le client (PORT)
    ecode=read(descSockCOM, buffer, MAXBUFFERLEN-1);
    if (ecode == -1){
        perror("probleme de lecture dans socket\n");
        exit(7);
    }
    buffer[ecode]='\0';
    printf("Message recu : %s\n", buffer);

    // Découpe le message pour récupérer les informations de connexion
    char ip[20];
    int port;
    int port1, port2;

    sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &port1, &port2);
    sprintf(ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    port = (port1 << 8) + port2;
    char portStr[10];
    sprintf(portStr, "%d", port);

    // Connecte le client au serveur de données
    int dataSock;
    ecode = connect2Server(ip, portStr, &dataSock);
    if (ecode == -1){
        perror("probleme de connexion au serveur\n");
        exit(8);
    }
    printf("Connexion au serveur réussie\n");

    //Envoi la requête PASV au serveur
    sprintf(buffer, "PASV\r\n", strlen("PASV\r\n"));
    ecode = write(descSockServ, buffer, strlen(buffer));
    if(ecode == -1){
        perror("probleme de écriture dans socket\n");
        exit(10);
    }
    printf("Message envoyé au serveur : %s\n", buffer);

    // Lit le message envoyé par le serveur
    ecode=read(descSockServ, buffer, MAXBUFFERLEN-1);
    if(ecode == -1){
        perror("probleme de lecture dans socket\n");
        exit(9);
    }
    buffer[ecode]='\0';
    printf("Message recu du serveur : %s\n", buffer);

    // Permet de Récupérer les informations de connexion du serveur de données
    int ip1, ip2, ip3, ip4;
    int portSrv;
    int portS1, portS2;
    sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip1, &ip2, &ip3, &ip4, &portS1, &portS2);
    portSrv = (portS1 << 8) + portS2;
    char portStrSrv[10];
    char ipStr[20];
    sprintf(portStrSrv, "%d", portSrv);
    sprintf(ipStr, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    printf("ipStr : %s\n", ipStr);
    printf("portSrtSrv : %s\n", portStrSrv);

    // Création d'un socket pour le transfert des données (liste) entre le proxy et le serveur
    int serverDataSock;
    ecode = connect2Server(ipStr, portStrSrv, &serverDataSock); //? Attribution du socket au port donné par le serveur
    if (ecode == -1){
        perror("Erreur connexion au serveur\n");
        exit(8);
    }
    printf("Connexion au serveur réussie\n\n");

    // Informer le client que la connexion est établie
    strcpy(buffer, "220 connexion établie.\r\n"); //  Copier la réponse dans le buffer de communication
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if (ecode == -1){
        perror("probleme d'écriture dans socket\n");
        exit(10);
    }

    // Récupérer la requête LIST envoyée par le client
    ecode = read(descSockCOM, buffer, MAXBUFFERLEN - 1);
    if (ecode == -1){
        perror("probleme de lecture dans socket\n");
        exit(7);
    }
    buffer[ecode] = '\0';
    printf("Message recu du client : %s\n", buffer);

    // Envoi de la requête au serveur
    ecode = write(descSockServ, buffer, strlen(buffer));
    if (ecode == -1){
        perror("probleme d'écriture dans socket\n");
        exit(10);
    }
    printf("Message envoyé au serveur : %s\n", buffer);

    // Récupère la première partie de la liste retournée par le serveur et la stocke dans le socket serveur
    ecode = read(serverDataSock, buffer, MAXBUFFERLEN - 1);
    if (ecode == -1){
        perror("probleme de lecture\n");
        exit(9);
    }
    buffer[ecode] = '\0';
    printf("Message recu serveur : %s\n", buffer);

    // Récupère le reste de la liste
    while (ecode != 0){
      ecode = write(dataSock, buffer, strlen(buffer));
      if (ecode == -1){
          perror("Erreur écriture dans socket\n");
          exit(10);
      }
      bzero(buffer, MAXBUFFERLEN - 1);
      ecode = read(serverDataSock, buffer, MAXBUFFERLEN - 1);
      if (ecode == -1){
          perror("probleme de lecture\n");
          exit(9);
      }
    }
    ecode = read(descSockServ, buffer, MAXBUFFERLEN - 1);
    // Envoi du message au client
    ecode = write(descSockCOM, buffer, strlen(buffer));
    if (ecode == -1){
        perror("Erreur écriture\n");
        exit(10);
    }
    bzero(buffer, MAXBUFFERLEN - 1);

    //Fermeture de la connexion
    close(descSockCOM);
    close(descSockRDV);
    close(descSockServ);
}
