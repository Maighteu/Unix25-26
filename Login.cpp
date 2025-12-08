#include "Login.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int rechercheUser(const char* identifiant)
{
  int fd,existingHash;
  USER user;
  if((fd = open("user.dat", O_RDONLY))==-1)
  { 
    printf("erreur d'ouverture\n");
    return -2;
  }
  while (read(fd,&user,sizeof(USER)) != 0)
  if(strcmp(user.identifiant,identifiant) == 0)
  {
    existingHash = user.hash;
    close(fd);
    return existingHash;
  }
  close(fd);
  return -1;
}

int hash(const char* password)
{

  int hash = 0;
  for (int i = 0; i < (int)strlen(password); i++)
  {
      hash += (i + 1) * int(password[i]);
  }

  hash %= 97;

  return hash;
}

void addUser(const char* identifiant, const char* password)
{
    USER user;
    int fd = open("user.dat", O_WRONLY | O_APPEND | O_CREAT, 0774);
    strcpy(user.identifiant, identifiant);
    user.hash = hash(password);
    write(fd, &user, sizeof(user));
    close(fd);
    printf("\nuser added \n");
}

bool authenticate(const char* identifiant, const char* password)
{
  int existingHash;
  if((existingHash=rechercheUser(identifiant))>=0)
  {
    if (existingHash == hash(password))
    {
          printf("user added \n");

      return true;
    }
    return false;
  }
  return false;
}