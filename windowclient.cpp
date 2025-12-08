#include "windowclient.h"
#include <QCloseEvent>
#include "ui_windowclient.h"
#include <QMessageBox>
#include "dialogmodification.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>

extern WindowClient *w;

#include "protocole.h"

int idQ, idShm, pidClient = getpid();
char* pShm;
#define TIME_OUT 120
bool logged;
void handlerSIGUSR1(int sig);
void handlerSIGUSR2(int sig);
int timeOut = TIME_OUT;

void handlerSIGUSR1(int sig);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
    ui->setupUi(this);
    ::close(2);
    logoutOK();

///////////////////////////////////////////////////////
    // Recuperation de l'identifiant de la file de messages
    if ((idQ = msgget(CLE, 0)) == -1)
    {
      fprintf(stderr, "(CLIENT %d) (ERROR) Impossible de recuperer l'identifiant de la file de message.", pidClient);
      exit(1);
    }
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());

//////////////////////////////////////////////////////////////////
    // Recuperation de l'identifiant de la mémoire partagée

    if ((idShm = shmget(CLE, 0, 0)) == -1)
    {
      fprintf(stderr, "(CLIENT %d) (ERROR) Impossible de recuperer l'identifiant de la memoire partagee.\n", pidClient);
      exit(1);
    }
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());


///////////////////////////////////////////////////////
    // Attachement à la mémoire partagée
    if ((pShm = (char*)shmat(idShm, NULL, 0)) == (char*)-1)
    {
      fprintf(stderr, "(CLIENT %d) (ERROR) Impossible de s'attacher à la mémoire partagée.", pidClient);
      exit(1);
    }
    fprintf(stderr, "(CLIENT %d) (SUCCESS) Attachement a la memoire partagee reussi", pidClient);


///////////////////////////////////////////////////////
    // Armement des signaux
    struct sigaction sig,sig2;
    sigfillset(&sig.sa_mask);
    sigdelset(&sig.sa_mask, SIGUSR1);
    sig.sa_handler = handlerSIGUSR1;
    sig.sa_flags = 0;

    if (sigaction(SIGUSR1, &sig, NULL) == -1)
    {
      fprintf(stderr, "(CLIENT %d) (ERROR) Erreur d'armement du signal SIGUSR1.", pidClient);
      exit(1);
    }
    printf("(CLIENT %d) (SUCCESS) Signal SIGUSR1 arme\n", pidClient);

    sigfillset(&sig2.sa_mask);
      sigdelset(&sig2.sa_mask, SIGUSR2);
      sig2.sa_handler = handlerSIGUSR2;
      sig2.sa_flags = 0;

    if (sigaction(SIGUSR2, &sig2, NULL) == -1)
    {
      fprintf(stderr, "(CLIENT %d) (ERROR) Erreur d'armement du signal SIGUSR2.", pidClient);
      exit(1);
    }
    printf("(CLIENT %d) (SUCCESS) Signal SIGUSR2 arme\n", pidClient);


//////////////////////////////////////////////
    // Envoi d'une requete de connexion au serveur
    MESSAGE requete;  // Structure pour la requête

    requete.type = 1;               // Type pour le serveur
    requete.expediteur = pidClient;  // PID du client
    requete.requete = CONNECT;      // Macro définie dans "protocole.h"

    if (msgsnd(idQ, &requete, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      fprintf(stderr, "(CLIENT %d) (ERROR) Impossible d'envoyer la requête CONNECT au serveur.", pidClient);
      exit(1);
    }
    printf("(CLIENT %d) (SUCCESS) Envoi d'une requete CONNECT au serveur reussi\n", pidClient);

  }

WindowClient::~WindowClient()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom()
{
  strcpy(connectes[0],ui->lineEditNom->text().toStdString().c_str());
  return connectes[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::isNouveauChecked()
{
  if (ui->checkBoxNouveau->isChecked()) return 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTimeOut(int nb)
{
  ui->lcdNumberTimeOut->display(nb);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setAEnvoyer(const char* Text)
{
  //fprintf(stderr,"---%s---\n",Text);
  if (strlen(Text) == 0 )
  {
    ui->lineEditAEnvoyer->clear();
    return;
  }
  ui->lineEditAEnvoyer->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getAEnvoyer()
{
  strcpy(aEnvoyer,ui->lineEditAEnvoyer->text().toStdString().c_str());
  return aEnvoyer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPersonneConnectee(int i,const char* Text)
{
  if (strlen(Text) == 0 )
  {
    switch(i)
    {
        case 1 : ui->lineEditConnecte1->clear(); break;
        case 2 : ui->lineEditConnecte2->clear(); break;
        case 3 : ui->lineEditConnecte3->clear(); break;
        case 4 : ui->lineEditConnecte4->clear(); break;
        case 5 : ui->lineEditConnecte5->clear(); break;
    }
    return;
  }
  switch(i)
  {
      case 1 : ui->lineEditConnecte1->setText(Text); break;
      case 2 : ui->lineEditConnecte2->setText(Text); break;
      case 3 : ui->lineEditConnecte3->setText(Text); break;
      case 4 : ui->lineEditConnecte4->setText(Text); break;
      case 5 : ui->lineEditConnecte5->setText(Text); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getPersonneConnectee(int i)
{
  QLineEdit *tmp;
  switch(i)
  {
    case 1 : tmp = ui->lineEditConnecte1; break;
    case 2 : tmp = ui->lineEditConnecte2; break;
    case 3 : tmp = ui->lineEditConnecte3; break;
    case 4 : tmp = ui->lineEditConnecte4; break;
    case 5 : tmp = ui->lineEditConnecte5; break;
    default : return NULL;
  }

  strcpy(connectes[i],tmp->text().toStdString().c_str());
  return connectes[i];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteMessage(const char* personne,const char* message)
{
  // Choix de la couleur en fonction de la position
  int i=1;
  bool trouve=false;
  while (i<=5 && !trouve)
  {
      if (getPersonneConnectee(i) != NULL && strcmp(getPersonneConnectee(i),personne) == 0) trouve = true;
      else i++;
  }
  char couleur[40];
  if (trouve)
  {
      switch(i)
      {
        case 1 : strcpy(couleur,"<font color=\"red\">"); break;
        case 2 : strcpy(couleur,"<font color=\"blue\">"); break;
        case 3 : strcpy(couleur,"<font color=\"green\">"); break;
        case 4 : strcpy(couleur,"<font color=\"darkcyan\">"); break;
        case 5 : strcpy(couleur,"<font color=\"orange\">"); break;
      }
  }
  else strcpy(couleur,"<font color=\"black\">");
  if (strcmp(getNom(),personne) == 0) strcpy(couleur,"<font color=\"purple\">");

  // ajout du message dans la conversation
  char buffer[300];
  sprintf(buffer,"%s(%s)</font> %s",couleur,personne,message);
  ui->textEditConversations->append(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNomRenseignements(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNomRenseignements->clear();
    return;
  }
  ui->lineEditNomRenseignements->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNomRenseignements()
{
  strcpy(nomR,ui->lineEditNomRenseignements->text().toStdString().c_str());
  return nomR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setGsm(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditGsm->clear();
    return;
  }
  ui->lineEditGsm->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setEmail(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditEmail->clear();
    return;
  }
  ui->lineEditEmail->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setCheckbox(int i,bool b)
{
  QCheckBox *tmp;
  switch(i)
  {
    case 1 : tmp = ui->checkBox1; break;
    case 2 : tmp = ui->checkBox2; break;
    case 3 : tmp = ui->checkBox3; break;
    case 4 : tmp = ui->checkBox4; break;
    case 5 : tmp = ui->checkBox5; break;
    default : return;
  }
  tmp->setChecked(b);
  if (b) tmp->setText("Accepté");
  else tmp->setText("Refusé");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK()
{
  ui->pushButtonLogin->setEnabled(false);
  ui->pushButtonLogout->setEnabled(true);
  ui->lineEditNom->setReadOnly(true);
  ui->lineEditMotDePasse->setReadOnly(true);
  ui->checkBoxNouveau->setEnabled(false);
  ui->pushButtonEnvoyer->setEnabled(true);
  ui->pushButtonConsulter->setEnabled(true);
  ui->pushButtonModifier->setEnabled(true);
  ui->checkBox1->setEnabled(true);
  ui->checkBox2->setEnabled(true);
  ui->checkBox3->setEnabled(true);
  ui->checkBox4->setEnabled(true);
  ui->checkBox5->setEnabled(true);
  ui->lineEditAEnvoyer->setEnabled(true);
  ui->lineEditNomRenseignements->setEnabled(true);
  setTimeOut(TIME_OUT);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK()
{
  ui->pushButtonLogin->setEnabled(true);
  ui->pushButtonLogout->setEnabled(false);
  ui->lineEditNom->setReadOnly(false);
  ui->lineEditNom->setText("");
  ui->lineEditMotDePasse->setReadOnly(false);
  ui->lineEditMotDePasse->setText("");
  ui->checkBoxNouveau->setEnabled(true);
  ui->pushButtonEnvoyer->setEnabled(false);
  ui->pushButtonConsulter->setEnabled(false);
  ui->pushButtonModifier->setEnabled(false);
  for (int i=1 ; i<=5 ; i++)
  {
      setCheckbox(i,false);
      setPersonneConnectee(i,"");
  }
  ui->checkBox1->setEnabled(false);
  ui->checkBox2->setEnabled(false);
  ui->checkBox3->setEnabled(false);
  ui->checkBox4->setEnabled(false);
  ui->checkBox5->setEnabled(false);
  setNomRenseignements("");
  setGsm("");
  setEmail("");
  ui->textEditConversations->clear();
  setAEnvoyer("");
  ui->lineEditAEnvoyer->setEnabled(false);
  ui->lineEditNomRenseignements->setEnabled(false);
  setTimeOut(TIME_OUT);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions permettant d'afficher des boites de dialogue /////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueMessage(const char* titre,const char* message)
{
   QMessageBox::information(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueErreur(const char* titre,const char* message)
{
   QMessageBox::critical(this,titre,message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Clic sur la croix de la fenêtre ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::closeEvent(QCloseEvent *event)
{
  MESSAGE message;
  message.type = 1; //1 car contact serv
  message.expediteur = pidClient;

  // Envoi d'une requete LOGOUT si logged
  if (logged)
  {
    message.requete = LOGOUT;

    if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      dialogueErreur("Erreur", "Impossible d'envoyer la requête de déconnexion du compte."); // Note : Est-ce qu'il faut avertir le client ?
      fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
      return;
    }
    printf("(CLIENT %d) (SUCCESS) Logout reussi\n", pidClient);
  }

  // Envoi d'une requete DECONNECT au serveur
  message.requete = DECONNECT;

  if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    dialogueErreur("Erreur", "Impossible d'envoyer la requête de déconnexion du serveur."); // Note : Est-ce qu'il faut avertir le client ?
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
    return;
  }
  printf("(CLIENT %d) (SUCCESS) Deconnexion reussie\n", pidClient);

  // Note : Si le serveur ne sait pas qu'on se déconnecte, le client se déconnecte quand même ou pas ?
  event->accept();
  exit(0);

    QApplication::exit();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
    MESSAGE message;
    message.type = 1;
    message.requete = LOGIN;
    message.expediteur = pidClient;
    sprintf(message.data1, "%d", isNouveauChecked());
     if (strcmp(getNom(), "") == 0 || strcmp(getMotDePasse(), "") == 0) // Si au moins un des champs est vide
  {
    dialogueErreur("Attention", "Le nom d'utilisateur et le mot de passe doivent être remplis !");
    return;
  }
  strcpy(message.data2, getNom());          // Nom du client
  strcpy(message.texte, getMotDePasse());   // Mot de passe

  if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    dialogueErreur("Erreur", "Impossible d'envoyer la requête pour se connecter.");
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
  }
}

void WindowClient::on_pushButtonLogout_clicked()
{
  MESSAGE message;
  message.type = 1;
  message.requete = LOGOUT;
  message.expediteur = pidClient;

  if (msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
  {
    dialogueErreur("Erreur", "Impossible d'envoyer la requête LOGOUT");
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgsnd()\n", pidClient);
  }
  printf("(CLIENT %d) (SUCCESS) Envoi de la requete LOGOUT reussi\n", pidClient);

  logged = false;
  logoutOK();
  dialogueMessage("Déconnexion", "Vous êtes maitenant déconnecté.");
}

void WindowClient::on_pushButtonEnvoyer_clicked()
{
    // TO DO
}

void WindowClient::on_pushButtonConsulter_clicked()
{
    // TO DO

}

void WindowClient::on_pushButtonModifier_clicked()
{
  // TO DO
  // Envoi d'une requete MODIF1 au serveur
  MESSAGE m;
  // ...

  // Attente d'une reponse en provenance de Modification
  fprintf(stderr,"(CLIENT %d) Attente reponse MODIF1\n",getpid());
  // ...

  // Verification si la modification est possible
  if (strcmp(m.data1,"KO") == 0 && strcmp(m.data2,"KO") == 0 && strcmp(m.texte,"KO") == 0)
  {
    QMessageBox::critical(w,"Problème...","Modification déjà en cours...");
    return;
  }

  // Modification des données par utilisateur
  DialogModification dialogue(this,getNom(),"",m.data2,m.texte);
  dialogue.exec();
  char motDePasse[40];
  char gsm[40];
  char email[40];
  strcpy(motDePasse,dialogue.getMotDePasse());
  strcpy(gsm,dialogue.getGsm());
  strcpy(email,dialogue.getEmail());

  // Envoi des données modifiées au serveur
  // ...
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les checkbox ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_checkBox1_clicked(bool checked)
{
    if (checked)
    {
        ui->checkBox1->setText("Accepté");
        // TO DO (etape 2)
    }
    else
    {
        ui->checkBox1->setText("Refusé");
        // TO DO (etape 2)
    }
}

void WindowClient::on_checkBox2_clicked(bool checked)
{
    if (checked)
    {
        ui->checkBox2->setText("Accepté");
        // TO DO (etape 2)
    }
    else
    {
        ui->checkBox2->setText("Refusé");
        // TO DO (etape 2)
    }
}

void WindowClient::on_checkBox3_clicked(bool checked)
{
    if (checked)
    {
        ui->checkBox3->setText("Accepté");
        // TO DO (etape 2)
    }
    else
    {
        ui->checkBox3->setText("Refusé");
        // TO DO (etape 2)
    }
}

void WindowClient::on_checkBox4_clicked(bool checked)
{
    if (checked)
    {
        ui->checkBox4->setText("Accepté");
        // TO DO (etape 2)
    }
    else
    {
        ui->checkBox4->setText("Refusé");
        // TO DO (etape 2)
    }
}

void WindowClient::on_checkBox5_clicked(bool checked)
{
    if (checked)
    {
        ui->checkBox5->setText("Accepté");
        // TO DO (etape 2)
    }
    else
    {
        ui->checkBox5->setText("Refusé");
        // TO DO (etape 2)
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    MESSAGE m;
  
  if (msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), pidClient, 0) == -1)
  {
    fprintf(stderr, "(CLIENT %d) (ERROR) Erreur de msgrcv\n", pidClient);
    return;
  }
    
      switch(m.requete)
      {
        case LOGIN :
                    printf("request received\n");
                    if (strcmp(m.data1,"OK") == 0)
                    {
                      fprintf(stderr,"(CLIENT %d) Login OK\n",getpid());
                      w->loginOK();
                      w->dialogueMessage("Login reussi",m.texte);
                      
                    }
                    else w->dialogueErreur("Login failed",m.texte);
                    break;

        case ADD_USER :
                    // TO DO
                    break;

        case REMOVE_USER :
                    // TO DO
                    break;

        case SEND :
                    // TO DO
                    break;

        case CONSULT :
                  // TO DO
                  break;
      }
}

void handlerSIGUSR2(int sig)
{
  printf("salut");
  
}