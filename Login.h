#ifndef FICHIER_UTILISATEUR_H
#define FICHIER_UTILISATEUR_H

typedef struct
{
  char identifiant[25];
  int hash;
} USER;
int rechercheUser(const char* identifiant);

int hash(const char* password);

void addUser(const char* identifiant, const char* password);

bool authenticate(const char* identifiant, const char* password);

#endif