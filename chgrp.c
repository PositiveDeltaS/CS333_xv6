// chgrp user program
// changes group of inode

#include "types.h"
#include "user.h"

int
main(int argc, char * argv[])
{
#ifdef CS333_P5
  if(argc != 3) {
    printf(1, "Invalid command\n");
		exit();
	}
  int group = atoi(argv[1]);
	if(group < 0 || group > 32767) {
	  printf(1, "GID out of bounds\n");
		exit();
	}
   
	if(chgrp(argv[2], group) < 0)
	  printf(1, "Could not set GID\n");

#endif //CS333_P5
  exit();
}
