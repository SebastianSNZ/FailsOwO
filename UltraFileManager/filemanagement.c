#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "diskmanagement.c"
#include <time.h>

struct SUPER_BLOCK
{
    int type;
    int inodesCount;
    int blocksCount;
    int freeBlockCount;
    int freeInodesCount;
    char mounTime[16];
    char unmountTime[16];
    int mountCount;
    int magic;
    int inodeSize;
    int blockSize;
    int firstInode;
    int firstBlock;
    int inodeStart;
    int blockStart;
    int bmInodeStart;
    int bmBlockStart;
};

typedef struct SUPER_BLOCK SuperBlock;

struct INODE
{
    int uid;
    int gid;
    int size;
    char aTime[16];
    char cTime[16];
    char mTime[16];
    int block[15];
    char type;
    int perm;
};

typedef struct INODE Inode;

struct CONTENT
{
    char name[12];
    int inode;
};

typedef struct CONTENT Content;

struct FOLDERBLOCK
{
    Content content[4];
};

typedef struct FOLDERBLOCK FolderBlock;

struct FILE_BLOCK
{
    char content[64];
};

typedef struct FILE_BLOCK FileBlock;

struct POINTER_BLOCK
{
    int pointer[16];
};

typedef struct POINTER_BLOCK PointerBlock;

struct JOURNALING
{
    char operation;
    char type;
    char name[12];
    char content[200];
    char date[16];
    int owner;
    int perm;
};

typedef struct JOURNALING Journal;



int getInode3Number(int partSize);
int getInode2Number(int partSize);

int getInode3Number(int partSize)
{
    double ret = (partSize - sizeof(SuperBlock));
    ret = ret / (4 + sizeof(Journal) + sizeof(Inode) + 3 * sizeof(FileBlock));
    return (int) ret;
}

int getInode2Number(int partSize)
{
    double ret = (partSize - sizeof(SuperBlock));
    ret = ret / (4 + sizeof(Inode) + 3 * sizeof(FileBlock));
    return (int) ret;
}

