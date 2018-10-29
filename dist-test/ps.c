#include "types.h"
#include "stat.h"
#include "user.h"
#ifdef CS333_P2
#include "uproc.h"
int
main (int argc, char *argv[])
{
  int max = atoi(argv[1]); 
  if(max < 0)
  {
    printf(1, "Error, val can't be negative.\n");
	exit();
  }
  struct uproc *table = malloc(max*sizeof(struct uproc));
  int x = getprocs(max, table);
  if(x < 0)
    printf(1, "Error when filling table\n");
  else
  {
	  printf(1, "PID\tName\tUID\tGID\tPPID\tElapsed\tTotal\tState\tSize\n");
	  for(int i = 0; i < x; ++i)
	  {
	    printf(1, "%d\t", table[i].pid);
	    printf(1, "%s\t", table[i].name);
	    printf(1, "%d\t", table[i].uid);
	    printf(1, "%d\t", table[i].gid);
	    printf(1, "%d\t", table[i].ppid);
		printf(1, "%d\t", table[i].elapsed_ticks);
		printf(1, "%d\t", table[i].CPU_total_ticks);
	    printf(1, "%s\t", table[i].state);
	    printf(1, "%d\t\n", table[i].size);
	  }
  }

  if(table)
  {
    free(table);
    printf(1, "Table free\n");
  }

  exit();
}
#endif
