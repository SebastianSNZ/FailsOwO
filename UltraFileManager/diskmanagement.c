#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

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

struct EXTENDED_BOOT_RECORD
{
    char status;
    char fit;
    int start;
    int size;
    int next;
    char name[32];
};

typedef struct EXTENDED_BOOT_RECORD ExtendedBootRecord;

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
int getAddressWithName(Partition parts[], char name[]);
int getAddresToAdd(FreeSpaceBlock *list, int currentSize, char fit);
int getWithBestFit(FreeSpaceBlock *list, int currentSize);
int getWithWorstFit(FreeSpaceBlock *list, int currentSize);
int getWithFirstFit(FreeSpaceBlock *list, int currentSize);
void deletePartition(char path[], char name[], char value[]);
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
    char arr[1024] = {0};
    int initial = sizeof(MasterBootRecord);
    int final = mbr.size + initial;
    for (int i = initial; i < final; i += 1024)
    {
        fseek(diskFile, i, SEEK_SET);
        fwrite(arr, sizeof(char) * 1024, 1, diskFile);
    }
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
    FILE *diskFile = fopen(path, "rb+");
    if (!diskFile)
    {
        printf("Error, el archivo %s no pudo abrirse para su lectura.\n", path);
        return;
    }
    MasterBootRecord mbr;
    fseek(diskFile, 0, SEEK_SET);
    fread(&mbr, sizeof(MasterBootRecord), 1, diskFile);
    if (type == 'L' || type == 'l') {
        //LOGICA
        return;
    }
    int address = getAddresToNewPartition(mbr, type);
    if (address < 0)
    {
        printf("Error, no hay posición disponible para otra particion en %s.\n", path);
        return;
    }
    if (getAddressWithName(mbr.partitions, name) >= 0)
    {
        printf("Error, ya existe una particion %s en %s.\n", name, path);
        return;
    }
    FreeSpaceBlock *fs = getFreeSpaceBlocks(mbr);
    int pos = getAddresToAdd(fs, realSize, mbr.fit);
    deleteFreeSpaceBlock(fs);
    if (pos < 0)
    {
        printf("Error, no hay espacio disponible para otra particion en %s.\n", path);
        return;
    }
    mbr.partitions[address].status = '1';
    mbr.partitions[address].fit = fit;
    strcpy(mbr.partitions[address].name, name);
    mbr.partitions[address].start = pos;
    mbr.partitions[address].size = realSize;
    mbr.partitions[address].type = type;
    fseek(diskFile, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MasterBootRecord), 1, diskFile);
    //SI ES EXTENDIDA HAY QUE METERLE EL COSO
    fclose(diskFile);
}

int getAddressWithName(Partition parts[], char name[])
{
    for (int i = 0; i < 4; i++)
    {
        if (!strcmp(parts[i].name, name)) return i;
    }
    return -1;
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

int getAddresToAdd(FreeSpaceBlock *list, int currentSize, char fit)
{
    if (fit == 'F' || fit == 'f')
    {
        return getWithFirstFit(list, currentSize);
    }
    if (fit == 'W' || fit == 'w')
    {
        return getWithWorstFit(list, currentSize);
    }
    return getWithBestFit(list, currentSize);
}

int getWithBestFit(FreeSpaceBlock *list, int currentSize)
{
    int x = INT_MAX;
    int y = -1;
    while (list != NULL)
    {
        if (list->size > currentSize && list->size - currentSize < x)
        {
            x = list->size - currentSize;
            y = list->start;
        }
        list = list->next;
    }
    return y;
}

int getWithWorstFit(FreeSpaceBlock *list, int currentSize)
{
    int x = -1;
    int y = -1;
    while (list != NULL)
    {
        if (list->size > currentSize && list->size > x)
        {
            x = list->size;
            y = list->start;
        }
        list = list->next;
    }
    return y;
}

int getWithFirstFit(FreeSpaceBlock *list, int currentSize)
{
    while (list != NULL)
    {
        if (list->size > currentSize) return list->start;
        list = list->next;
    }
    return -1;
}

void deletePartition(char path[], char name[], char value[])
{
    FILE *toRead = fopen(path, "rb+");
    if (!toRead)
    {
        printf("Error, el archivo %s no pudo abrirse para su lectura\n", path);
        return;
    }
    MasterBootRecord mbr;
    fseek(toRead, 0, SEEK_SET);
    fread(&mbr, sizeof(MasterBootRecord), 1, toRead);
    int address = getAddressWithName(mbr.partitions, name);
    if (address < 0)
    {
        printf("Error, el disco %s no posee una particion %s.\n", path, name);
        return;
    }
    if (mbr.partitions[address].type == 'L' || mbr.partitions[address].type == 'l')
    {
        //eliminar logica
        return;
    }
    if (!strncasecmp(value, "full", 4))
    {
        int initial = mbr.partitions[address].start;
        int final = initial + mbr.partitions[address].size;
        for(int i = initial; i < final; i++)
        {
            char c = '\0';
            fseek(toRead, i, SEEK_SET);
            fwrite(&c, sizeof(char), 1, toRead);
        }
    }
    mbr.partitions[address].fit = 'F';
    mbr.partitions[address].size = 0;
    mbr.partitions[address].start = -1;
    mbr.partitions[address].status = 0;
    mbr.partitions[address].type = 'P';
    strcpy(mbr.partitions[address].name, "");
    fseek(toRead, 0, SEEK_SET);
    fwrite(&mbr, sizeof(MasterBootRecord), 1, toRead);
    fclose(toRead);
}
