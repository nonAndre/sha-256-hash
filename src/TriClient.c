#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <openssl/sha.h>
#include "semaphore.h"
#include "errExit.h"
#define DIM 2
#define MAX_SIZE 200

struct SharedMemory
{
  int sessionIdtmp;
  int activeProcesses;
  int maxProcesses;
  int isBlocked;
};

struct requestedResources
{
  int sessionId;
  int isString;
  char stringToConvert[MAX_SIZE];
  uint8_t hash[32];
};

int appoMem2;
int appoClientMem;
int clientSem;
int currentSessionId;

void sigHandler(int sig)
{
  if (semctl(clientSem, 0, IPC_RMID, NULL) == -1)
    errExit("semctl IPC_RMID failed");

  shmctl((appoClientMem), IPC_RMID, NULL);
  exit(0);
}

void clean(struct SharedMemory *appo)
{
  printf("\nPulisco\n");
  appo->activeProcesses--;
  if (semctl(clientSem, 0, IPC_RMID, NULL) == -1)
    errExit("semctl IPC_RMID failed");

  shmctl((appoClientMem), IPC_RMID, NULL);
  exit(0);
}

void printSemaphoresValue(int semid)
{
  unsigned short semVal[2];
  union semun arg;
  arg.array = semVal;

  if (semctl(semid, 0 /*ignored*/, GETALL, arg) == -1)
    errExit("semctl GETALL failed");

  printf("semaphore set state:\n");
  for (int i = 0; i < 2; i++)
    printf("id: %d --> %d\n", i, semVal[i]);
}

void setRequest(struct requestedResources *appo, struct SharedMemory *sh)
{
  int choice;
  char name[MAX_SIZE];

  do
  {
    printf("\nDesideri inserire una stringa o un file per la conversione? \n1. Stringa \n2. File \n3.Cambia il numero di max processes");
    printf("\nScelta: ");
    scanf("%d", &choice);
  } while (choice < 1 && choice > 3);

  if (choice == 1)
  {
    appo->isString = 1;
    printf("\nInserisci il nome della stringa da convertire:");
    scanf("%s", name);
    strncpy(appo->stringToConvert, name, MAX_SIZE - 1);
    appo->stringToConvert[MAX_SIZE - 1] = '\0';
  }
  else if (choice == 2)
  {
    appo->isString = 0;
    int file;
    do
    {
      printf("\nInserisci il nome del file da convertire:");
      scanf("%s", name);

      file = access(name, F_OK);

      if (file != 0)
      {
        printf("\nIl file %s non esiste", name);
      }

    } while (file != 0);

    strncpy(appo->stringToConvert, name, MAX_SIZE - 1);
    appo->stringToConvert[MAX_SIZE - 1] = '\0';
  }
  else
  {
    int mProcesses;

    appo->isString = 3;
    printf("\nNew max processes: ");
    scanf("%d", &mProcesses);

    sh->maxProcesses = mProcesses;
  }
}

void print_hash(uint8_t *hash)
{
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
  {
    printf("%02x", hash[i]);
  }
  printf("\n");
}

int main(int argc, char *argv[])
{

  if (signal(SIGINT, sigHandler) == SIG_ERR)
    printf("Errore handler SIGINT\n");

  key_t chiave = ftok("src/Memoria.txt", 'B');
  if (chiave < 0)
    errExit("Chiave errata server");

  int shmid = shmget(chiave, sizeof(struct SharedMemory), 0666);
  if (shmid == -1)
    errExit("shmget supervisor failed");
  struct SharedMemory *supervisor = (struct SharedMemory *)shmat(shmid, NULL, 0);

  // Imposto i semafori

  appoMem2 = shmid;

  //-------------------------------------------------------------------------------------------------------------------------

  key_t key = ftok("src/Semafori.txt", 'A');

  if (key == -1)
    errExit("ftok failed");

  int semid = semget(key, 2, 0);

  if (semid == -1)
    errExit("semget failed");

  // Passo la palla al server per verificare se si può avviare la richiesta
  semOp(semid, (unsigned short)0, 1);
  semOp(semid, (unsigned short)1, -1);

  if (supervisor->isBlocked == 1)
  {
    supervisor->isBlocked = 0;
    printf("\nIl server non puo' gestire la tua richiesta");
    printf("\nRiprova più tardi o aumenta il numero di richiesta possibili contemporaneamente");
    printf("\nNumero massimo processi: %d , Processi attivi al moemento %d ", supervisor->maxProcesses, supervisor->activeProcesses);
    exit(0);
  }

  currentSessionId = supervisor->sessionIdtmp;

  printf("\n[Session Id]: %d\n", currentSessionId);

  if (currentSessionId <= 0)
  {
    errExit("Invalid session ID received from server");
  }

  int semidChildren = semget(currentSessionId, 2, 0);

  if (semidChildren == -1)
    errExit("semget failed");

  clientSem = semidChildren;

  // Se va a buon fine creo mem condivisa com Id sessione

  int sdmClientID = shmget(currentSessionId, sizeof(struct requestedResources), 0666);
  if (sdmClientID == -1)
    errExit("shmget sdmClientID failed");

  appoClientMem = sdmClientID;

  struct requestedResources *clientMem = (struct requestedResources *)shmat(sdmClientID, NULL, 0);

  setRequest(clientMem, supervisor);

  semOp(semidChildren, (unsigned short)0, 1);
  semOp(semidChildren, (unsigned short)1, -1);

  if (clientMem->isString == 3)
  {
    clean(supervisor);
  }

  print_hash(clientMem->hash);

  semOp(semidChildren, (unsigned short)0, 1);

  if (semctl(semidChildren, 0, IPC_RMID, NULL) == -1)
    errExit("semctl IPC_RMID failed");

  shmdt((void *)clientMem);
  shmctl((sdmClientID), IPC_RMID, NULL);

  return 0;
}
