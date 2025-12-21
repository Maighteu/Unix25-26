#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <mysql.h>
#include <setjmp.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include "Login.h"
#define FICHIER_CLIENTS "user.dat"

int idQ,idShm,idSem, pidServeur = getpid();
int fdPipe[2];
TAB_CONNEXIONS *tab;
sigjmp_buf contexte;

void afficheTab();
void connectClient(int pidClient);
void disconnectClient(int pidClient);
int clientConnecte(int pidClient);
void envoyerMessage(MESSAGE& m);
void handlerSIGINT(int signal);
void handlerSIGCHLD(int signal);
void closing(int codeSortie);
void logClient(int pidClient, char* nouveauClient, char* identifiant, char* password);
void sendToSelectedUser(int pidClient, char* nom, char* message);
void unlogClient(int pidClient);
void acceptUser(char *nom, int pid);
void refuseUser(char *nom, int pid);
void sendPub();
int rechercheClient(char *nom);
void handlerSIGALRM(int signal);
MYSQL* connexion;


int main()
{
  int pidPublicite, pidAccesBD;
  // Connection à la BD
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(SERVEUR) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Armement des signaux
 // Armement de SIGINT
      struct sigaction sig, sigC;
    sigfillset(&sig.sa_mask);
    sigdelset(&sig.sa_mask, SIGINT);
    sig.sa_handler = handlerSIGINT;
    sig.sa_flags = 0;

  if (sigaction(SIGINT, &sig, NULL) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de sigaction\n", pidServeur);
    exit(1);
  }
  printf("(SERVEUR %d) (SUCCESS) Signal SIGINT arme\n", pidServeur);

  // Armement de SIGCHLD
  sigfillset(&sigC.sa_mask);
  sigdelset(&sigC.sa_mask, SIGCHLD);
  sigC.sa_handler = handlerSIGCHLD;
  sigC.sa_flags = 0;

  if (sigaction(SIGCHLD, &sigC, NULL) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de sigaction\n", pidServeur);
    exit(1);
  }
  printf("(SERVEUR %d) (SUCCESS) Signal SIGCHLD arme\n", pidServeur);

 

  // Creation des ressources
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  // Creation de la memoire partagee
  if ((idShm = shmget(CLE, 51 * sizeof(char), IPC_CREAT | IPC_EXCL | 0666)) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de shmget()\n", pidServeur);
    closing(1);
  }
  printf("(SERVEUR %d) (SUCCESS) Memoire partagee cree\n", pidServeur);



  // Initialisation du tableau de connexions
  fprintf(stderr,"(SERVEUR %d) Initialisation de la table des connexions\n",getpid());
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
    tab->connexions[i].pidModification = 0;
  }
  tab->pidServeur1 = getpid();
  tab->pidServeur2 = 0;
  tab->pidAdmin = 0;
  tab->pidPublicite = 0;

  afficheTab();

  // Creation du processus Publicite
   int idFils, idPub;
 idPub = fork();
 if(idPub==0)
 {
   if(execl("./Publicite", "Publicite", NULL)==-1)
   {
     perror("Serveur erreur de execl");
     exit(1);
   }
 }

  int i,k,j;
  MESSAGE m;
  MESSAGE reponse;
  char requete[200];
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  PUBLICITE pub;

  while(1)
  {
  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }

    switch(m.requete)
    {
      case CONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                      connectClient(m.expediteur);
                      break; 

      case DECONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);
                      disconnectClient(m.expediteur);

                      break; 

      case LOGIN :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.texte);
                      logClient(m.expediteur, m.data1, m.data2, m.texte);
                      break; 

      case LOGOUT :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      unlogClient(m.expediteur);
                      break;

      case ACCEPT_USER :
                      acceptUser(m.data1, m.expediteur);
                      fprintf(stderr,"(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n",getpid(),m.expediteur);
                      break;

      case REFUSE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete REFUSE_USER reçue de %d\n",getpid(),m.expediteur);
                      refuseUser(m.data1, m.expediteur);
                      break;

      case SEND :  
                      fprintf(stderr,"(SERVEUR %d) Requete SEND reçue de %d\n",getpid(),m.expediteur);
                      sendToSelectedUser(m.expediteur, m.data1, m.texte);
                      break; 

      case UPDATE_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
                        sendPub();
                      break;

      case CONSULT :
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case MODIF1 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF1 reçue de %d\n",getpid(),m.expediteur);
                      break;

      case MODIF2 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF2 reçue de %d\n",getpid(),m.expediteur);
                      break;

      case LOGIN_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n",getpid(),m.expediteur);
                      break;

      case LOGOUT_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n",getpid(),m.expediteur);
                      break;

      case NEW_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                      break;

      case DELETE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n",getpid(),m.expediteur,m.data1);
                      break;

      case NEW_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;
    }
    afficheTab();
  }
}

void afficheTab()
{
  fprintf(stderr,"Pid Serveur 1 : %d\n",tab->pidServeur1);
  fprintf(stderr,"Pid Serveur 2 : %d\n",tab->pidServeur2);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid Admin     : %d\n",tab->pidAdmin);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d %6d %6d %6d %6d - %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].autres[0],
                                                      tab->connexions[i].autres[1],
                                                      tab->connexions[i].autres[2],
                                                      tab->connexions[i].autres[3],
                                                      tab->connexions[i].autres[4],
                                                      tab->connexions[i].pidModification);
  fprintf(stderr,"\n");
}

void envoyerMessage(MESSAGE& m)
{
  printf("Mon message s'envoie \n");
  printf("mon message est du type: %d\n", m.requete);
   if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    printf("erreur d'envoi \n");
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de msgsnd()\n", pidServeur);
    return;
  }
}
void connectClient(int pidClient)
{
  int i = 0;
  MESSAGE m;
  while (i < 6 && tab->connexions[i].pidFenetre !=0)
  {
    i ++;
  }
  if (i>=6)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Le serveur n'a plus assez de place pour accepter le client\n", pidServeur);


    envoyerMessage(m);

    if (kill(pidClient, SIGUSR1) == -1)
    {
      fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
    }
    return;
    int i = 0;
  }
  else  tab->connexions[i].pidFenetre = pidClient;
}

void disconnectClient(int pidClient)
{
  int posClient;

  if((posClient = clientConnecte(pidClient)) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Le client a deconnecter n'a pas pu etre trouve dans la table des connexions\n", pidServeur);
    return;
  }
  else
  {
  MESSAGE reponse;
  for(int i; i<6; i++)
  {
    if( posClient != i  && strcmp(tab->connexions[i].nom,""))
    {
      for(int j=0; j<6;j++)
      {
        tab->connexions[i].autres[j] = 0;
      }
      reponse.type = tab->connexions[i].pidFenetre;
        reponse.expediteur = pidServeur;
        reponse.requete = REMOVE_USER;
        printf("vide liste de contact");
        printf("J'envoie un nom\n");
        printf("le nom est %s\n", tab->connexions[posClient].nom);
        printf("je l'envoie à %ld\n", tab->connexions[i].pidFenetre);
        
        strcpy(reponse.data1, "OK");
        strcpy(reponse.data2, tab->connexions[posClient].nom);
        envoyerMessage(reponse);
        sleep(1);
        if (kill(tab->connexions[i].pidFenetre, SIGUSR1) == -1)
        {
          printf("Reponse ne s'envoie pas \n");
          fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
        }
    }
  }

   tab->connexions[posClient].pidFenetre = 0;
   strcpy(tab->connexions[posClient].nom, "");
  }

  
}

int clientConnecte(int pidClient)
{
  int i = 0;
  while(i<6 && tab->connexions[i].pidFenetre != pidClient)
  {
    i++;
  }
  if (i>=6) return -1;
    else return i;
}

void closing(int codeSortie)
{


  if (msgctl(idQ, IPC_RMID, NULL) == -1) // Si la file de message n'a pas ete supprime
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de msgctl()\n", pidServeur);
    codeSortie = 1;
  }
  else
  {
    printf("(SERVEUR %d) (SUCCESS) File de message supprimee\n", pidServeur);
  }


  if (shmctl(idShm, IPC_RMID, NULL) ==-1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de shmctl\n", pidServeur);
    codeSortie = 1;
  }
  else
  {
    printf("(SERVEUR %d) (SUCCESS) Memoire partagee supprimee\n", pidServeur);
  }
  if (kill(tab->pidPublicite, SIGKILL) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
    codeSortie = 1;
  }
  fprintf(stderr, "(SERVEUR %d) (SUCCESS) Processus Publicite tue.\n", pidServeur);

  if (close(fdPipe[0]) == -1)
  {
      fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de fermeture sortie pipe\n", pidServeur);
    codeSortie = 1;
  }
  else
  {
      fprintf(stderr, "(SERVEUR %d) (SUCCESS) sortie pipe ferme\n", pidServeur);
  }

  if (close(fdPipe[1]) == -1)
  {
      fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de fermeture entree pipe\n", pidServeur);
    codeSortie = 1;
  }
  else
  {
      fprintf(stderr, "(SERVEUR %d) (SUCCESS) entree pipe ferme\n", pidServeur);
  }
  exit(codeSortie);
}

void logClient(int pidClient, char* nouveauClient, char *identifiant, char *password)
{
  int fd, retour, posClient;
  bool connexionReussie = false;
  MESSAGE m;
  m.type = pidClient;
  m.expediteur = pidServeur;
  m.requete = LOGIN;

  printf("%s\n", identifiant);
  printf("%s\n", password);

  if ((posClient = clientConnecte(pidClient)) == -1)
  {
    strcpy(m.texte, "Vous n'êtes pas connecté au serveur. Veuillez redémarrer votre application.");

    envoyerMessage(m);

   {
      fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
    }

    return;
  }

  // Le client veut créer un compte
  if (strcmp(nouveauClient,"1") == 0)
  {
    printf("recherche nouveaux user\n");

    if (rechercheUser(identifiant) > 0)
    {
      strcpy(m.texte, "Account already exist\n");
    }
    else
    {
      addUser(identifiant, password);
      connexionReussie = true;
      strcpy(m.texte, "account created and connected\n");
    }
  }
  
  // Le client veut se connecter
  else
  {
    printf("recherche old user\n");
    if (authenticate(identifiant,password) ==true) // Le client existe
    {
      printf("authenticate passé\n");
        connexionReussie = true;
        strcpy(m.texte, "Connected\n");
    }
    else if (authenticate(identifiant,password) == false)
    {
      printf("authenticate failed\n");
      strcpy(m.texte, "Echec d'authentification.\n");
    }
    else // Erreur du serveur
    {
      fprintf(stderr, "Erreur d'ouverture du fichier %s\n", FICHIER_CLIENTS);
      strcpy(m.texte, "Server error");
    }
  
  }


  if (connexionReussie == true)
  {
    printf("connexion réussie \n\n");
    strcpy(tab->connexions[posClient].nom, identifiant);
    strcpy(m.data1,"OK");
  }
  else // Sinon, on ne le le fait pas.
  {
    strcpy(m.data1,"KO");
  }

  // On envoie le message de reponse au client
  envoyerMessage(m);

  // On previent le client qu'il a un message
  if (kill(pidClient, SIGUSR1) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
  }

  // on remplis la table de contact
  if (connexionReussie == true)
  {
    MESSAGE reponse;

  // notre nouveau client reçois les noms des autres clients

    reponse.type = pidClient;
    reponse.expediteur = pidServeur;
    reponse.requete = ADD_USER;

    for(int i = 0; i<6; i++)
    {
      if((strcmp(tab->connexions[i].nom,identifiant) != 0)&& strcmp(tab->connexions[i].nom,""))
      {
        printf("J'envoie un nom\n");
        printf("le nom est %s\n", tab->connexions[i].nom);
        printf("je l'envoie à %ld\n", pidClient);
        strcpy(reponse.data1, "OK");
        strcpy(reponse.data2, tab->connexions[i].nom);
        envoyerMessage(reponse);
        sleep(1);
         if (kill(pidClient, SIGUSR1) == -1)
        {
          printf("Reponse ne s'envoie pas \n");
          fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
        }
        else printf("le signal s'envoie\n");
      }
    }

    for(int i = 0; i<6; i++)
    {
      if((strcmp(tab->connexions[i].nom,identifiant) != 0)&& strcmp(tab->connexions[i].nom,""))
      {
        reponse.type = tab->connexions[i].pidFenetre;
        reponse.expediteur = pidServeur;
        reponse.requete = ADD_USER;
        printf("J'envoie un nom\n");
        printf("le nom est %s\n", tab->connexions[i].nom);
        printf("je l'envoie à %ld\n", identifiant);
        strcpy(reponse.data1, "OK");
        strcpy(reponse.data2, identifiant);
        envoyerMessage(reponse);
        sleep(1);
        if (kill(tab->connexions[i].pidFenetre, SIGUSR1) == -1)
        {
          printf("Reponse ne s'envoie pas \n");
          fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
        }
        else printf("le signal s'envoie\n");
        
      }
    }

  }

}

void unlogClient(int pidClient)
{
  int posClient;

  // Recherche la position du client.
  if ((posClient = clientConnecte(pidClient)) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Le client %d n'a pas ete trouve dans la table de connexion. Impossible de delogger le client.\n", pidServeur, pidClient);
    return;
  }

  MESSAGE reponse;
  for(int i; i<6; i++)
  {
    if( posClient != i  && strcmp(tab->connexions[i].nom,""))
    {
       for(int j=0; j<6;j++)
      {
        tab->connexions[i].autres[j] = 0;
      }
      
      reponse.type = tab->connexions[i].pidFenetre;
        reponse.expediteur = pidServeur;
        reponse.requete = REMOVE_USER;
        printf("vide liste de contact");
        printf("J'envoie un nom\n");
        printf("le nom est %s\n", tab->connexions[posClient].nom);
        printf("je l'envoie à %ld\n", tab->connexions[i].pidFenetre);
        
        strcpy(reponse.data1, "OK");
        strcpy(reponse.data2, tab->connexions[posClient].nom);
        envoyerMessage(reponse);
        sleep(1);
        if (kill(tab->connexions[i].pidFenetre, SIGUSR1) == -1)
        {
          printf("Reponse ne s'envoie pas \n");
          fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
        }
    }
  }
  // Supprime le nom du client de la table
  strcpy(tab->connexions[posClient].nom, "");
  // Envoie un message LOGOUT au processus Caddie.
  MESSAGE m;
  m.type = tab->connexions[posClient].pidFenetre;
  m.requete = LOGOUT;
  m.expediteur = pidServeur;

  envoyerMessage(m);
    if (kill(pidClient, SIGUSR1) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
  }
}
int rechercheUser(char* nom)
{
  for(int i=0; i<6; i++)
  {
    if(strcmp(tab->connexions[i].nom, nom)==0)
    {
      printf("\n\nClient trouve pour la recherche en position %d\n\n", i);
      return(i);
    }
  }
  return(-1);

}
void acceptUser(char*nom, int pid)
{
  int i,j ;
  i= rechercheUser(nom);
  for(j = 0; j<6;j++)
  {
    if (tab->connexions[i].autres[j] == 0) 
      {
            printf("on insere en position : %d\n\n", j);

        break;
      }
  }

  tab->connexions[i].autres[j] = pid;
}
void refuseUser(char*nom, int pid)
{
  int i,j;
  i = rechercheUser(nom);
  for( j = 0; j<6;j++)
  {
    if (tab->connexions[i].autres[j] == pid) break;
  }
  tab->connexions[i].autres[j] = 0;

}
void sendToSelectedUser(int pidClient, char* nom, char* message)
{
  printf("debut de l'action envoi\n\n");
  int j;
  int i = rechercheUser(nom);
  MESSAGE m;
  m.requete = SEND;
  m.expediteur = pidServeur;
  strcpy(m.data1, nom);
  strcpy(m.texte, message);
  for(j = 0; j<6;j++)
  {
    if (tab->connexions[i].autres[j] != 0) 
      {
        printf("personne trouvée pour envoi message\n\n");
        m.type = tab->connexions[i].autres[j];
        envoyerMessage(m);
        if (kill(tab->connexions[i].autres[j], SIGUSR1) == -1)
        {
          printf("Reponse ne s'envoie pas \n");
          fprintf(stderr, "(SERVEUR %d) (ERROR) Erreur de kill()\n", pidServeur);
        }
        break;
      }
  }

}

void sendPub()
{
  for(int i = 0; i<6; i++)
  {
    if(tab->connexions[i].pidFenetre!=0)
    {
      kill(tab->connexions[i].pidFenetre,SIGUSR2);
    }
  }
}

void handlerSIGINT(int signal)
{
  fprintf(stderr, "\n(SERVEUR %d) (SUCCESS) Signal %d recu\n", pidServeur, signal);

  closing(0);
}

void handlerSIGCHLD(int signal)
{
  fprintf(stderr, "\n(SERVEUR %d) (SUCCESS) Signal %d recu\n", pidServeur, signal);

  closing(0);
}
