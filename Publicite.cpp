#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ, idShm;
int fd;
void handlerSIGUSR1(int sig);


int main()
{
  printf("\n\n entre dans la publicite\n\n");
  // Armement des signaux
  struct  sigaction sig;
  sigfillset(&sig.sa_mask);
  sigdelset(&sig.sa_mask, SIGUSR1);
  sig.sa_handler = handlerSIGUSR1;
  sig.sa_flags = 0;
  if (sigaction(SIGUSR1, &sig, NULL) == -1)
    {
      fprintf(stderr, "(CLIENT %d) (ERROR) Erreur d'armement du signal SIGUSR1.", getpid());
      exit(1);
    }
    printf("(CLIENT %d) (SUCCESS) Signal SIGUSR1 arme\n", getpid());
    
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
  if((idQ = msgget(CLE, 0))==-1)
  {
    perror("Publicite erreur de msgget");
    exit(1);
  }

  // Recuperation de l'identifiant de la mémoire partagée
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la mémoire partagée\n",getpid());
  if((idShm = shmget(CLE,0,0))==-1)
  {
    perror("Publicite erreur de shmget");
    exit(1);
  }
  // Attachement à la mémoire partagée
  char *pSh;
  if((pSh= (char*)shmat(idShm,NULL, 0))==(char*)-1)
  {
    perror("Publicite erreur de shmat");
    exit(1);
  }
  // Ouverture du fichier de publicité
  if((fd = open("publicites.dat", O_RDWR))==-1)
  {
   pause();
  }
  while(1)
  {
  	PUBLICITE pub;
    // Lecture d'une publicité dans le fichier
    if(read(fd, &pub, sizeof(PUBLICITE))==0)
    {
      lseek(fd, 0, SEEK_SET);    
    }
    // Ecriture en mémoire partagée 
    printf("\n\n %s\n\n", pub.texte);
    strcpy(pSh, pub.texte);
    // Envoi d'une requete UPDATE_PUB au serveur
    MESSAGE m;
    m.type = 1;
    m.requete = UPDATE_PUB;
    m.expediteur=getpid();
    if (msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", getpid());
  }
    sleep(pub.nbSecondes);
  }
}

void handlerSIGUSR1(int sig)
{
  printf("sigusr11 est appelé???\n\n");
}