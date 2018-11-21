#ifdef CS333_P4
#include "types.h"
#include "user.h"
#include "param.h"

// Set both TICKS_TO_PROMOTE and BUDGET very high for best results

int
main(int argc, char *argv[])
{
  int i;
  int max = 10;
  int pid[max];
  int rc;

  printf(1, "+=+=+=+=+=+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| Start: Set Priority Test |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=+=+=+=+=+=\n");

  printf(1, "\nCurrent MAXPRIO = %d\n", MAXPRIO);

  // Give a new priority value to a PID that is valid
  // and demenstrate that the budget also resets
  // Also, demenstrate that changing the priority of a processes
  // moves the process to the correct ready list
  printf(1, "\nTEST 1: Changing the priority to values within an acceptable range\n\n");
  printf(1, "Creating %d processes and setting them a new priority of 0...\n", max);
  for(i = 0; i < max; i++){
    rc = fork();
    setpriority(getpid(), 0);
    if(rc == 0)
      while(1);
    pid[i] = rc;
  }

  printf(1, "Waiting for the budget of the active processes to deplete in value ");
  printf(1, "before setting a priority of %d to new processs...\n", MAXPRIO);

  sleep(8000);
  printf(1, "================================================\n");
  printf(1, "Press ctrl-r to show original state of processes\n");
  printf(1, "================================================\n");
  sleep(3000);

  setpriority(getpid(), MAXPRIO);
  printf(1, "Changing priority to %d for PIDs of: ", MAXPRIO);
  for(i = 0; i < max; i++){
    printf(1, " %d ", pid[i]);
    if(MAXPRIO > 0)
      setpriority(pid[i], MAXPRIO-1);
  }

  printf(1, "\n=============================================\n");
  printf(1, "Press ctrl-r to show new state of processes\n");
  printf(1, "===============================================\n");
  sleep(3000);

  for(i = 0; i < max; i++)
    kill(pid[i]);
  while(wait() > 0);

  //Calling setpriority with an invalid PID and/or priority
  //returns an error code and leaves the process priority and budget intact
  printf(1, "\nTEST 2: Setting priority with values above MAXPRIO and below 0\n\n");
  setpriority(getpid(), MAXPRIO +5);
  setpriority(getpid(), -1);

 // Setting and identical priority value to a processes
 // does not change its position on ready list

  printf(1, "\nTEST 3: Setting a PID's current priority value as its new value\n\n");
  for(i = 0; i < max; i++){
    rc = fork();
    setpriority(getpid(), 0);
    if(rc == 0)
      while(1);
    pid[i] = rc;
  }

  printf(1, "%d new processes were created and given a priority of 0\n", max);
  setpriority(rc, getpriority(rc));
  printf(1, "\n===========================================================\n");
  printf(1, "Press ctrl-r to see if the priority is anything other than 0\n");
  printf(1, "============================================================\n");
  sleep(3000);
  for(i = 0; i < max; i++)
    kill(pid[i]);
  while(wait() > 0);

  printf(1, "+=+=+=+=+=+=+=+=+=+=+=+=+=+=\n");
  printf(1, "| End of Set Priority Test |\n");
  printf(1, "+=+=+=+=+=+=+=+=+=+=+=+=+=+=\n");
  exit();
}
#endif
