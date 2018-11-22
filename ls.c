#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

#ifdef CS333_P5
// this is an ugly series of if statements but it works
void
print_mode(struct stat* st)
{
  switch (st->type) {
    case T_DIR: printf(1, "d"); break;
    case T_FILE: printf(1, "-"); break;
    case T_DEV: printf(1, "c"); break;
    default: printf(1, "?");
  }

  if (st->mode.flags.u_r)
    printf(1, "r");
  else
    printf(1, "-");

  if (st->mode.flags.u_w)
    printf(1, "w");
  else
    printf(1, "-");

  if ((st->mode.flags.u_x) & (st->mode.flags.setuid))
    printf(1, "S");
  else if (st->mode.flags.u_x)
    printf(1, "x");
  else
    printf(1, "-");

  if (st->mode.flags.g_r)
    printf(1, "r");
  else
    printf(1, "-");

  if (st->mode.flags.g_w)
    printf(1, "w");
  else
    printf(1, "-");

  if (st->mode.flags.g_x)
    printf(1, "x");
  else
    printf(1, "-");

  if (st->mode.flags.o_r)
    printf(1, "r");
  else
    printf(1, "-");

  if (st->mode.flags.o_w)
    printf(1, "w");
  else
    printf(1, "-");

  if (st->mode.flags.o_x)
    printf(1, "x");
  else
    printf(1, "-");

  return;
}
#endif

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }
#ifdef CS333_P5
  printf(1, "mode\t\tname\t\tuid\tgid\tinode\tsize\t\n"); 
#endif //CS333_P5
  switch(st.type){
  case T_FILE:
  case T_DEV:
#ifdef CS333_P5
    print_mode(&st);
    printf(1, "\t%s\t%d\t%d\t%d\t%d\n", fmtname(path), st.uid, st.gid, st.ino, st.size);
#else
    printf(1, "%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
#endif //CS333_P5
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
#ifdef CS333_P5
      print_mode(&st);
      printf(1, "\t%s\t%d\t%d\t%d\t%d\n", fmtname(buf), st.uid, st.gid, st.ino, st.size);
#else
      printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
#endif //CS333_P5
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit();
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit();
}
