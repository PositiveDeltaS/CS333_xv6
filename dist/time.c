#ifdef CS333_P2
#include "types.h"
#include "stat.h"
#include "user.h"



int
main(int argc, char *argv[])
{
  int start_time = uptime();
  int pid;
  //char * prog = safestrcpy(prog, argv, strlen(*argv));
  switch (pid = fork())
  {
  	case -1: printf(1, "Fork failed\n"); 
	  exit();
	case 0:
	  argv++;
	  exec(*argv, argv);
	default: wait();
	  int time_elapsed = uptime() - start_time;
      int ex_ms = time_elapsed%1000;
      int lhs = 0; 
      if(time_elapsed >=1000) 
	  {
        lhs = time_elapsed - ex_ms; 
        lhs = lhs/1000;
      }
      if(ex_ms < 10)
        printf(1, "Time elapsed for %s: %d.00%d\n", argv, lhs, ex_ms);
      else if(ex_ms < 100)
        printf(1, "Time elapsed for %s: %d.0%d\n", argv, lhs, ex_ms);
      else
        printf(1, "Time elapsed for %s: %d.%d\n", argv, lhs, ex_ms);
  } 
  exit();
    
	
}
#endif
