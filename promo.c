
#ifdef CS333_P4
#include "types.h"
#include "user.h"
#include "param.h"

void
cleanupProcs(int pid[], int max)
{
  int i;
  for(i = 0; i < max; i++)
    kill(pid[i]);
  while(wait() > 0);
}

int
createInfiniteProc()
{
  int pid = fork();

  setpriority(getpid(), 0);
  if(pid == 0)
    while(1);
  printf(1, "Process %d created...\n", pid);

  return pid;
}

void
sleepMessage(int time, char message[])
{
  printf(1, message);
  sleep(time);
}

void
testprom()
{
  int i;
  int max = 10;
  int pid[max];

  printf(1, "+=+=+=+=+=+=+=+=+=+=+=+=+\n");
  printf(1, "| Start: Promotion Test |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=+=+=+=+\n");

  setpriority(1, 0);
  setpriority(2, 0);

  for(i = 0; i < max; i++){
    pid[i] = createInfiniteProc();
  }

  for(i = 0; i < 3; i++)
    sleepMessage(6000, "Sleeping... ctrl-p\n");

  cleanupProcs(pid, max);

  printf(1, "+=+=+=+=+=+=+=+=+=+=+=+=+\n");
  printf(1, "| End of Promotion Test |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=+=+=+=+\n");

}

int
main(int argc, char *argv[])
{
  printf(1, "\nTest will set processes to the lowest priority to see if they can promot thierselves\n");
  printf(1, "Current settings: MAXPRIO = %d\n", MAXPRIO);

  testprom();

  exit();
}
#endif
