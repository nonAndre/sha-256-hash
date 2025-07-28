#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <time.h>
#include <stdbool.h>
#include "semaphore.h"
#include "sha256File.h"
#include "sha256String.h"
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

int sig_count = 0;
int appoId;
int appoMem;
int appoClientMem;

void clean()
{
  if (semctl(appoId, 0, IPC_RMID, NULL) == -1)
    errExit("semctl IPC_RMID failed");
  shmctl((appoMem), IPC_RMID, NULL);
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

void intHandling(int sig)
{
  if (sig_count == 0)
  {
    write(1, "Pressing ctrl-c twice within 5 seconds to end the game", 54);
    sig_count++;
    sleep(5);
  }
  else
  {
    write(1, "\nGame ended\n", 12);
    clean();
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

  srand(time(NULL));
  // SIGINT Handler
  if (signal(SIGINT, intHandling) == SIG_ERR)
    printf("Error handling server\n");

  // Imposto la memoria condivisa

  key_t chiave = ftok("src/Memoria.txt", 'B');
  if (chiave < 0)
    errExit("Chiave errata server");

  int shmid = shmget(chiave, sizeof(struct SharedMemory), IPC_CREAT | 0666);
  if (shmid == -1)
    errExit("shmget supervisor failed");
  struct SharedMemory *supervisor = (struct SharedMemory *)shmat(shmid, NULL, 0);

  // Imposto i semafori
  key_t key = ftok("src/Semafori.txt", 'A');

  if (key == -1)
    errExit("ftok failed");

  int semid = semget(key, 2, IPC_CREAT | S_IRUSR | S_IWUSR);

  // passo id per funzione clean

  appoId = semid;
  appoMem = shmid;

  if (semid == -1)
    errExit("semget failed");

  // Inizializzo i semafori
  unsigned short semInitVal[] = {0, 0};
  union semun arg;
  arg.array = semInitVal;

  if (semctl(semid, 0, SETALL, arg) == -1)
    errExit("semctl SETALL failed");

  supervisor->activeProcesses = 0;
  supervisor->maxProcesses = 1;
  supervisor->sessionIdtmp = 0;
  supervisor->isBlocked = 0;

  // Check se è possibile creare una sessione
  while (true)
  {
    semOp(semid, (unsigned short)0, -1);

    if (supervisor->activeProcesses < supervisor->maxProcesses)
    {
      supervisor->sessionIdtmp = rand() % 1000000;

      supervisor->activeProcesses++;

      pid_t pid;

      pid = fork();

      if (pid < 0)
      {
        errExit("\nFork failed");
      }
      else if (pid == 0)
      {
        // Imposto sessioId
        // Set semafori per comunicazione figlio
        // Imposto i semafori
        int sessionID = supervisor->sessionIdtmp;

        if (sessionID == -1)
          errExit("ftok failed");

        int semidChildren = semget(sessionID, 2, IPC_CREAT | S_IRUSR | S_IWUSR);

        if (semidChildren == -1)
          errExit("semget failed");

        // Inizializzo i semafori
        unsigned short semInitValues[] = {0, 0};
        union semun arg2;
        arg2.array = semInitValues;

        if (semctl(semidChildren, 0, SETALL, arg2) == -1)
          errExit("semctl SETALL fork failed");

        // Alloco la memoria condivisa per la sessione

        int sdmClientID = shmget(sessionID, sizeof(struct requestedResources), IPC_CREAT | 0666);
        if (sdmClientID == -1)
          errExit("shmget sdmClientID failed");
        struct requestedResources *clientMem = (struct requestedResources *)shmat(sdmClientID, NULL, 0);

        clientMem->sessionId = supervisor->sessionIdtmp;

        printf("\nSession id: %d , mem: %d , sem: %d\n", clientMem->sessionId, sdmClientID, semidChildren);

        semOp(semid, (unsigned short)1, 1);
        semOp(semidChildren, (unsigned short)0, -1);

        // Elaboro richiesta
        if (clientMem->isString == 1)
        {
          digestString(clientMem->stringToConvert, clientMem->hash);
        }
        else if (clientMem->isString == 0)
        {
          digestFile(clientMem->stringToConvert, clientMem->hash);
        }
        else
        {
          semOp(semidChildren, (unsigned short)1, 1);
          exit(0);
        }

        if (clientMem->isString != 3)
        {
          semOp(semidChildren, (unsigned short)1, 1);
        }
        supervisor->activeProcesses--;
        exit(0);
      }
      else
      {
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0)
        {
          printf("\nSessione completata ");
        }
      }
    }
    else
    {
      supervisor->isBlocked = 1;
      semOp(semid, (unsigned short)1, 1);
    }
  }

  // Pulizia memoria condivisa e semafori

  if (semctl(semid, 0, IPC_RMID, NULL) == -1)
    errExit("semctl IPC_RMID failed");

  shmdt((void *)supervisor);
  shmctl((shmid), IPC_RMID, NULL);

  return 0;
}
