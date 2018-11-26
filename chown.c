// chown user program
// changes uid of inode

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
  int owner = atoi(argv[1]);
	if(owner < 0 || owner > 32767) {
	  printf(1, "UID out of bounds\n");
		exit();
	}
   
	if(chown(argv[2], owner) < 0)
	  printf(1, "Could not set UID\n");

#endif //CS333_P5
  exit();
}
