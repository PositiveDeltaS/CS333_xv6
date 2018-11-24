// chmod user program
// changes mode of inode

#include "types.h"
#include "user.h"

#ifdef CS333_P5

#define USAGE "Usage: %s <mode><target>\n"
void
bail(char *s)
{
	printf(2, "Error: invalid mode\n");
	printf(2, USAGE, s);
	exit();
}
#endif //CS333_P5

int
main(int argc, char * argv[])
{
#ifdef CS333_P5
  if(argc != 3) {
    printf(1, "Invalid command\n");
		exit();
	}
  int mode = atoo(argv[1]);
	if(mode < 0 || mode > 01777)
	  bail(argv[0]);
   
	if(chmod(argv[2], mode) < 0)
	  printf(1, "Could not set mode\n");

#endif //CS333_P5
  exit();
}
