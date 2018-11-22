#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#ifdef PDX_XV6
#define FSSIZE       2000  // size of file system in blocks
#else
#define FSSIZE       1000  // size of file system in blocks
#endif // PDX_XV6
#ifdef CS333_P1
#define DEFAULTUID	 0	   // default UID
#define DEFAULTGID	 0	   // default GID
#define DEFAULTMODE  00755
#endif // CS333

