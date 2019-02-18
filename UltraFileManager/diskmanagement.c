#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//STRUCTS IMPORTANTES

struct PARTITION
{
    char status;
    char type;
    char fit;
    int start;
    int size;
    char name[32];
};
typedef struct PARTITION Partition;

struct MASTER_BOOT_RECORD
{
    int size;
    char date[32];
    int disksignature;
    char fit;
    Partition partitions[4];
};

typedef struct MASTER_BOOT_RECORD MasterBootRecord;

struct FREE_SPACE
{
    int start;
    int size;
    struct FREE_SPACE *next;
};

typedef struct FREE_SPACE FreeSpaceBlock;

//FIN STRUCTS


//PROTOTIPOS

void createNewDisk(int size, char fit, char unit, char path[512]);
int paramatersDiskCreation(int size, char fit, char unit);
int getRealSize(char unit, int size);
MasterBootRecord getMbr(int size, char fit, char unit); //metodos para crear discos
void deleteDisk(char path[]); //metodo para eliminar discos)
void addPartition(int size, char unit, char type, char fit, char name[], char path[]);
int paramatersAddingPartition(int size, char unit, char type, char fit, char name[]);
int getAddresToNewPartition(MasterBootRecord mbr, char type);
FreeSpaceBlock *getFreeSpaceBlocks(MasterBootRecord mbr);
FreeSpaceBlock *newFreeSpaceBlock(int start, int size);
void addFreeSpaceBlock(FreeSpaceBlock **aux, int start, int size);
int getNextPartition(Partition parts[], int ptr);
void deleteFreeSpaceBlock(FreeSpaceBlock *aux);


//FIN PROTOTIPOS




void createNewDisk(int size, char fit, char unit, char path[512])
{
    if (!paramatersDiskCreation(size, fit, unit)) return;
    if (fit == '\0') fit = 'F';
    if (unit == '0') unit = 'M';
    MasterBootRecord mbr = getMbr(size, fit, unit);
    FILE *diskFile = fopen(path, "wb");
    if (diskFile == NULL)
    {
        printf("Error, el archivo %s no pudo abrirse para su escritura.\n", path);
        return;
    }
    fseek(diskFile, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MasterBootRecord), 1, diskFile);
    fclose(diskFile);
}

int paramatersDiskCreation(int size, char fit, char unit)
{
    if (size < 0)
    {
        printf("Error, parametro _size_ incompatible.\n");
        return 0;
    }
    if (fit != 'B' && fit != 'b' && fit != 'F' && fit != 'f' && fit != 'W' && fit != 'w' && fit != '\0')
    {
        printf("Error, parametro _fit_ incompatible.\n");
        return 0;
    }
    if (unit != 'k' && unit != 'K' && unit != 'M' && unit != 'm' && unit != '\0')
    {
        printf("Error, parametro _unit_ incompatible.\n");
        return 0;
    }
    return 1;
}

int getRealSize(char unit, int size)
{
    int realSize = 0;
    if (unit == 'b' || unit == 'B') realSize = size;
    else if (unit == 'k' || unit == 'K') realSize = size * 1024;
    else realSize = size * 1024 * 1024;
    return realSize;
}

MasterBootRecord getMbr(int size, char fit, char unit)
{
    int currentSize = getRealSize(unit, size);
    MasterBootRecord mbr;
    mbr.size = currentSize;
    mbr.disksignature = rand();
    mbr.fit = fit;
    time_t date = time(0);
    struct tm *tlocal = localtime(&date);
    strftime(mbr.date, 128, "%d/%m/%y %H:%M:%S", tlocal);
    for (int i = 0; i < 4; i++)
    {
        mbr.partitions[i].fit = 'F';
        mbr.partitions[i].size = 0;
        mbr.partitions[i].start = -1;
        mbr.partitions[i].status = 0;
        mbr.partitions[i].type = 'P';
        strcpy(mbr.partitions[i].name, "");
    }
    return mbr;
}

void deleteDisk(char path[])
{
    if (path[0] == '\0')
    {
        printf("Error, el parametro _path_ esta vacio.\n");
    }
    if (remove(path)) printf("Error, no fue posible eliminar el archivo %s \n", path);
}

void addPartition(int size, char unit, char type, char fit, char name[], char path[])
{
    if (!paramatersAddingPartition(size, unit, type, fit, name)) return;
    if (fit == '\0') fit = 'W';
    if (unit == '0') unit = 'K';
    if (type == '\0') type = 'P';
    int realSize = getRealSize(unit, size);
    FILE *diskFile = fopen(path, "rb");
    if (!diskFile)
    {
        printf("Error, el archivo %s no pudo abrirse para su lectura.\n", path);
        return;
    }
    MasterBootRecord mbr;
    fseek(diskFile, 0, SEEK_SET);
    fread(&mbr, sizeof(MasterBootRecord), 1, diskFile);
    fclose(diskFile);
    int address = getAddresToNewPartition(mbr, type);
    if (address < 0)
    {
        printf("Error, no hay espacio en el disco para otra particion en %s.\n", path);
        return;
    }
    //VERIFICAR NOMBRE
    FreeSpaceBlock *fs = getFreeSpaceBlocks(mbr);
    mbr.partitions[address].status = '1';
    mbr.partitions[address].fit = fit;
    strcpy(mbr.partitions[address].name, name);
    mbr.partitions[address].start = fs->start;
    mbr.partitions[address].size = realSize;
    mbr.partitions[address].type = type;
    FILE *toWrite = fopen(path, "wb");
    fseek(toWrite, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MasterBootRecord), 1, toWrite);
    fclose(toWrite);
}

int getAddresToNewPartition(MasterBootRecord mbr, char type)
{
    int address = -1;
    int extended = 0;
    for (int i = 0; i < 4; i++)
    {
        if (mbr.partitions[i].status == 0)
        {
            address = i;
            break;
        }
        if (mbr.partitions[i].type == 'E' || mbr.partitions[i].type == 'e') extended = 1;
    }
    if (type == 'E' && extended) return -1;
    return address;
}

int paramatersAddingPartition(int size, char unit, char type, char fit, char name[])
{
    if (size < 0)
    {
        printf("Error, parametro _size_ incompatible.\n");
        return 0;
    }
    if (unit != 'b' && unit != 'B' && unit != 'k' && unit != 'K' && unit != 'M' && unit != 'm' && unit != '\0')
    {
        printf("Error, parametro _unit_ incompatible.\n");
        return 0;
    }
    if (fit != 'B' && fit != 'b' && fit != 'F' && fit != 'f' && fit != 'W' && fit != 'w' && fit != '\0')
    {
        printf("Error, parametro _fit_ incompatible.\n");
        return 0;
    }
    if (type != 'P' && type != 'p' && type != 'E' && type != 'e' && type != 'L' && type != 'l' && type != '\0')
    {
        printf("Error, parametro _type_ incompatible.\n");
        return 0;
    }
    if (name [0] == '\0')
    {
        printf("Error, parametro _name_ vacio.\n");
        return 0;
    }
    return 1;
}


FreeSpaceBlock *newFreeSpaceBlock(int start, int size)
{
    FreeSpaceBlock *newFS = (FreeSpaceBlock *) malloc(sizeof(FreeSpaceBlock));
    newFS->size = size;
    newFS->start = start;
    newFS->next = NULL;
    return newFS;
}

void addFreeSpaceBlock(FreeSpaceBlock **aux, int start, int size)
{
    if ((*aux) == NULL)
    {
        *aux = newFreeSpaceBlock(start, size);
        return;
    }
    FreeSpaceBlock *node = *aux;
    while (node->next != NULL) node = node->next;
    node->next = newFreeSpaceBlock(start, size);
}

int getNextPartition(Partition parts[], int ptr)
{
    int i = -1;
    int pivot = ptr;
    for (int j = 0; j < 4; j++)
    {
        if (parts[j].status == 0 || parts[j].start < ptr)
        {
            continue;
        }
        if (parts[j].start - ptr < pivot)
        {
            i = j;
            pivot = parts[j].start - ptr;
        }
    }
    return i;
}

FreeSpaceBlock *getFreeSpaceBlocks(MasterBootRecord mbr)
{
    int x = sizeof(MasterBootRecord);
    int y = mbr.size;
    FreeSpaceBlock *list = NULL;
    int i = getNextPartition(mbr.partitions, x);
    while (i != -1)
    {
        Partition p = mbr.partitions[i];
        if (x < p.start) {
            addFreeSpaceBlock(&list, x, p.start - x);
        }
        x = p.start + p.size;
        y = mbr.size - x;
        i = getNextPartition(mbr.partitions, x);
    }
    if (y != 0) addFreeSpaceBlock(&list, x, y);
    return list;
}

void deleteFreeSpaceBlock(FreeSpaceBlock *aux)
{
    if (aux == NULL) return;
    deleteFreeSpaceBlock(aux->next);
    free(aux);
}
