#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

//STRUCTS IMPORTANTES

struct PARTITION
{
    char status;
    char type;
    char fit;
    int start;
    int size;
    char name[16];
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
    char name[16];
};

typedef struct EXTENDED_BOOT_RECORD ExtendedBootRecord;

typedef struct FREE_SPACE FreeSpaceBlock;

struct DISK_FILE_NODE
{
    char letter;
    char path[512];
    int count;
    struct DISK_FILE_NODE *next;
};

typedef struct DISK_FILE_NODE FileDiskNode;

struct PARTITION_NODE
{
    char name[32];
    FileDiskNode *filePtr;
    char id[32];
    int start;
    int size;
    char type;
    struct PARTITION_NODE *next;
};

typedef struct PARTITION_NODE PartitionNode;

//FIN STRUCTS

//VARIABLES GLOBALES

FileDiskNode  *diskList = NULL;
PartitionNode *partList = NULL;
char currentLetter = 'a';

//FIN VARIABLES GLOBALES

//PROTOTIPOS

void createNewDisk(int size, char fit, char unit, char path[512]); //metodo para crear un nuevo disco
int paramatersDiskCreation(int size, char fit, char unit); // verificar parametros
int getRealSize(char unit, int size); // obtener tamaño de acuerdo del unit
MasterBootRecord getMbr(int size, char fit, char unit); //metodos para crear discos
void deleteDisk(char path[]); //metodo para eliminar discos)
void addPartition(int size, char unit, char type, char fit, char name[], char path[]); //agrega una particion
int paramatersAddingPartition(int size, char unit, char type, char fit, char name[]); // verifica parametros
int getAddresToNewPartition(MasterBootRecord mbr, char type); //obtiene direccion de espacio libre
FreeSpaceBlock *getFreeSpaceBlocks(MasterBootRecord mbr); //obtiene los bloques de espacio libre
FreeSpaceBlock *newFreeSpaceBlock(int start, int size); // crea un nuevo bloque
void addFreeSpaceBlock(FreeSpaceBlock **aux, int start, int size); // agrega un bloque a la lista
int getNextPartition(Partition parts[], int ptr); // obtiene la siguiente particion
void deleteFreeSpaceBlock(FreeSpaceBlock *aux); // elimina la lista de bloques
void addLogicalPartition(MasterBootRecord mbr, int size, char unit, char fit, char name[], FILE *writeFile);
int getExtendedIndex(Partition parts[]);
int getAddressWithName(Partition parts[], char name[]);
int getLogicalAddresWithName(MasterBootRecord mbr, char name[], FILE *readFile);
int getAddresToAdd(FreeSpaceBlock *list, int currentSize, char fit);
int getWithBestFit(FreeSpaceBlock *list, int currentSize);
int getWithWorstFit(FreeSpaceBlock *list, int currentSize);
int getWithFirstFit(FreeSpaceBlock *list, int currentSize);
void deletePartition(char path[], char name[], char value[]);
void addSizePartition(int add, char unit, char path[], char name[]);
ExtendedBootRecord getEbr(char name[]);
FreeSpaceBlock *getLogicalFreeSpace(ExtendedBootRecord first, FILE *writeFile, int totalSize);
FileDiskNode *newFileDiskNode(char letter, char path[]);
void addNewFileDiskNode(FileDiskNode **aux, char path[]);
FileDiskNode *getFileDiskNode(FileDiskNode *aux, char path[]);
PartitionNode *newPartitionNode(char name[], char id[], FileDiskNode *ptr, int part, int size, char type);
void addNewPartitionNode(PartitionNode **aux, char name[], char id[], FileDiskNode *ptr, int part, int size, char type);
PartitionNode *getPartitionNode(PartitionNode *aux, char id[]);
void mountPartition(char path[], char name[]);
void deletePartitionNode(PartitionNode **aux, char id[]);
void unMountPartition(char id[]);
void getDirectory(char text[]);
void report(char id[], char path[], char name[], char file[]);
void reportDisk(char filePath[], char destiny[]);
void reportMbr(char filePath[], char destiny[]);

//FIN PROTOTIPOS



void createNewDisk(int size, char fit, char unit, char path[512])
{
    if (!paramatersDiskCreation(size, fit, unit)) return;
    if (fit == '\0') fit = 'F';
    if (unit == '0') unit = 'M';
    char dir[512] = {0};
    char command[512] = {0};
    strcpy(dir, path);
    getDirectory(dir);
    strcat(command, "mkdir -p -m a=rwx ");
    strcat(command, dir);
    system(command);
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

void getDirectory(char text[])
{
    int i;
    for (i = 512; i >= 0; i--)
    {
        if (text[i] == '/')
            break;
        text[i] = '\0';
    }
    i++;
    text[i] = '\"';
    char aux[512] = {0};
    strcpy(aux, text);
    text[0] = '\"';
    for (int j = 1; j < 512; j++)
    {
        text[j] = aux[j - 1];
        if (aux[j - 1] == '\0') break;
    }
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
    int address = getAddresToNewPartition(mbr, type);
    int logicalAddres = getLogicalAddresWithName(mbr, name, diskFile);
    if (getAddressWithName(mbr.partitions, name) >= 0 || logicalAddres > 0 )
    {
        printf("Error, ya existe una particion %s en %s.\n", name, path);
        fclose(diskFile);
        return;
    }
    if (type == 'L' || type == 'l') {
        addLogicalPartition(mbr, size, unit, fit, name, diskFile);
        fclose(diskFile);
        return;
    }
    if (address < 0)
    {
        printf("Error, no hay posición disponible para otra particion en %s.\n", path);
        fclose(diskFile);
        return;
    }
    FreeSpaceBlock *fs = getFreeSpaceBlocks(mbr);
    int pos = getAddresToAdd(fs, realSize, mbr.fit);
    deleteFreeSpaceBlock(fs);
    if (pos < 0)
    {
        printf("Error, no hay espacio disponible para otra particion en %s.\n", path);
        fclose(diskFile);
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
    if (type == 'E' || type == 'e') {
        fseek(diskFile, mbr.partitions[address].start, SEEK_SET);
        ExtendedBootRecord ebr = getEbr("INITIAL_EBR");
        ebr.start = mbr.partitions[address].start;
        ebr.status = '1';
        fwrite(&ebr, sizeof(ExtendedBootRecord), 1, diskFile);
    }
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
    int pivot = INT_MAX;
    for (int j = 0; j < 4; j++)
    {
        //int h = parts[j].start;
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

void probe(ExtendedBootRecord aux, FILE *readFile)
{
    int next = aux.next;
    while (next != -1)
    {
        fseek(readFile, next, SEEK_SET);
        fread(&aux, sizeof(ExtendedBootRecord), 1, readFile);
        printf("%s\n", aux.name);
        next = aux.next;
    }
}

void addLogicalPartition(MasterBootRecord mbr, int size, char unit, char fit, char name[], FILE *writeFile)
{
    int address = getExtendedIndex(mbr.partitions);
    if (address < 0)
    {
        printf("Error, el disco no posee una particion extendida.\n");
        return;
    }
    int dSize = mbr.partitions[address].size;
    int realSize = getRealSize(unit, size);

    ExtendedBootRecord firstEbr;
    fseek(writeFile, mbr.partitions[address].start, SEEK_SET);
    fread(&firstEbr, sizeof(ExtendedBootRecord), 1, writeFile);
    FreeSpaceBlock *fs = getLogicalFreeSpace(firstEbr, writeFile, dSize);

    int direction = getAddresToAdd(fs, realSize, mbr.partitions[address].fit);
    deleteFreeSpaceBlock(fs);
    ExtendedBootRecord prev;
    fseek(writeFile, direction, SEEK_SET);
    fread(&prev, sizeof(ExtendedBootRecord), 1, writeFile);

    ExtendedBootRecord newEBR = getEbr(name);
    newEBR.fit = fit;
    newEBR.size = realSize;
    newEBR.start = prev.start + prev.size;
    newEBR.next = prev.next;
    prev.next = newEBR.start;
    fseek(writeFile, prev.start, SEEK_SET);
    fwrite(&prev, sizeof(ExtendedBootRecord), 1, writeFile);
    fseek(writeFile, newEBR.start, SEEK_SET);
    fwrite(&newEBR, sizeof(ExtendedBootRecord), 1, writeFile);
    //probe(firstEbr, writeFile);
    //printf("A\n\n");
}

FreeSpaceBlock *getLogicalFreeSpace(ExtendedBootRecord first, FILE *writeFile, int totalSize)
{
    int x = first.start;
    int s = totalSize - first.size;
    int y = first.size;
    int next = first.next;
    FreeSpaceBlock *aux = NULL;
    while (next != -1)
    {
        ExtendedBootRecord newEbr;
        fseek(writeFile, next, SEEK_SET);
        fread(&newEbr, sizeof(ExtendedBootRecord), 1, writeFile);
        if (newEbr.start > x + y)
        {
            addFreeSpaceBlock(&aux, x, newEbr.start - (x + y));
        }
        x = newEbr.start;
        y = newEbr.size;
        s = (totalSize - first.size) - y;
        next = newEbr.next;
    }
    if (s != 0)
    {
        addFreeSpaceBlock(&aux, x, s);
    }
    return aux;
}

int getLogicalAddresWithName(MasterBootRecord mbr, char name[], FILE *readFile)
{
    int ad = getExtendedIndex(mbr.partitions);
    if (ad < 0) return -1;
    ExtendedBootRecord aux;
    fseek(readFile, mbr.partitions[ad].start, SEEK_SET);
    fread(&aux, sizeof(ExtendedBootRecord), 1, readFile);
    int next = aux.next;
    int prev = aux.start;
    while (next != -1)
    {
        fseek(readFile, next, SEEK_SET);
        fread(&aux, sizeof(ExtendedBootRecord), 1, readFile);
        if (!strcmp(aux.name, name)) return prev;
        next = aux.next;
        prev = aux.start;
    }
    return -1;
}

int getExtendedIndex(Partition parts[])
{
    for (int i = 0; i < 4; i++)
    {
        if (parts[i].type == 'E' || parts[i].type == 'e') return i;
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
    int logicalAddress = getLogicalAddresWithName(mbr, name, toRead);
    if (address < 0 && logicalAddress < 0)
    {
        printf("Error, el disco %s no posee una particion %s.\n", path, name);
        fclose(toRead);
        return;
    }
    if (address >= 0)
    {
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
        return;
    }
    ExtendedBootRecord prev;
    fseek(toRead, logicalAddress, SEEK_SET);
    fread(&prev, sizeof(ExtendedBootRecord), 1, toRead);
    ExtendedBootRecord toDel;
    fseek(toRead, prev.next, SEEK_SET);
    fread(&toDel, sizeof(ExtendedBootRecord), 1, toRead);

    prev.next = toDel.next;

    if (!strncasecmp(value, "full", 4))
    {
        int initial = toDel.start;
        int final = initial + toDel.size;
        for(int i = initial; i < final; i++)
        {
            char c = '\0';
            fseek(toRead, i, SEEK_SET);
            fwrite(&c, sizeof(char), 1, toRead);
        }
    }

    fseek(toRead, prev.start, SEEK_SET);
    fwrite(&prev, sizeof(ExtendedBootRecord), 1, toRead);
    fclose (toRead);
}


void addSizePartition(int add, char unit, char path[], char name[])
{
    FILE *diskFile = fopen(path, "rb+");
    if (!diskFile)
    {
        printf("Error, el archivo %s no pudo abrirse para su lectura\n", path);
        return;
    }
    MasterBootRecord mbr;
    fseek(diskFile, 0, SEEK_SET);
    fread(&mbr, sizeof(MasterBootRecord), 1, diskFile);
    int sizeToAdd = getRealSize(unit, add);
    int address = getAddressWithName(mbr.partitions, name);
    int logicalAddress = getLogicalAddresWithName(mbr, name, diskFile);
    if (address < 0 && logicalAddress < 0)
    {
        printf("Error, el disco %s no posee una particion %s.\n", path, name);
        fclose(diskFile);
        return;
    }
    if (address >= 0)
    {
        if (sizeToAdd < 0 && sizeToAdd + mbr.partitions[address].size < 0)
        {
            printf("Error, no es posible quitar espacio en la particion %s\n", name);
            fclose(diskFile);
            return;
        }
        else if (sizeToAdd >= 0)
        {
            int space = 0;
            int j = getNextPartition(mbr.partitions, mbr.partitions[address].start + mbr.partitions[address].size);
            if (j < 0) space = mbr.size - (mbr.partitions[address].start + mbr.partitions[address].size);
            else space = mbr.partitions[j].start - (mbr.partitions[address].start + mbr.partitions[address].size);
            if (space < sizeToAdd)
            {
                printf("Error, no es posible agregar espacio en la particion %s\n", name);
                fclose(diskFile);
                return;
            }
        }
        mbr.partitions[address].size = mbr.partitions[address].size + sizeToAdd;
        fseek(diskFile, 0, SEEK_SET);
        fwrite(&mbr, sizeof(MasterBootRecord), 1, diskFile);
        fclose(diskFile);
        return;
    }
    ExtendedBootRecord ebr;
    fseek(diskFile, logicalAddress, SEEK_SET);
    fread(&ebr, sizeof(ExtendedBootRecord), 1, diskFile);
    fseek(diskFile, ebr.next, SEEK_SET);
    fread(&ebr, sizeof(ExtendedBootRecord), 1, diskFile);
    if (sizeToAdd < 0 && sizeToAdd + ebr.size < 0)
    {
        printf("Error, no es posible quitar espacio en la particion %s\n", name);
        fclose(diskFile);
        return;
    }
    else if (sizeToAdd >= 0)
    {
        int space = ebr.next - (ebr.size + ebr.start);
        if (space < sizeToAdd)
        {
            printf("Error, no es posible agregar espacio en la particion %s\n", name);
            fclose(diskFile);
            return;
        }
    }
    ebr.size = ebr.size + sizeToAdd;
    fseek(diskFile, ebr.start, SEEK_SET);
    fwrite(&ebr, sizeof(ExtendedBootRecord), 1, diskFile);
    fclose(diskFile);
}



ExtendedBootRecord getEbr(char name[])
{
    ExtendedBootRecord ebr;
    ebr.fit = 'F';
    ebr.next = -1;
    ebr.size = sizeof(ExtendedBootRecord);
    ebr.start = -1;
    ebr.status = 0;
    strcpy(ebr.name, name);
    return ebr;
}


FileDiskNode *newFileDiskNode(char letter, char path[])
{
    FileDiskNode *newFD = (FileDiskNode *) malloc(sizeof(FileDiskNode));
    newFD->letter = letter;
    strcpy(newFD->path, path);
    newFD->next = NULL;
    newFD->count = 0;
    return newFD;
}

void addNewFileDiskNode(FileDiskNode **aux, char path[])
{
    if ((*aux) == NULL)
    {
        (*aux) = newFileDiskNode(currentLetter, path);
        currentLetter++;
        return;
    }
    FileDiskNode *piv = *aux;
    while (piv->next != NULL) piv = piv->next;
    piv->next = newFileDiskNode(currentLetter, path);
    currentLetter++;
}

FileDiskNode *getFileDiskNode(FileDiskNode *aux, char path[])
{
    while (aux != NULL)
    {
        if (!strcmp(aux->path, path)) return aux;
        aux = aux->next;
    }
    return NULL;
}


PartitionNode *newPartitionNode(char name[], char id[], FileDiskNode *ptr, int part, int size, char type)
{
    PartitionNode *pNode = (PartitionNode *) malloc(sizeof(PartitionNode));
    strcpy(pNode->id, id);
    strcpy(pNode->name, name);
    pNode->filePtr = ptr;
    pNode->next = NULL;
    pNode->start = part;
    pNode->size = size;
    pNode->type = type;
    return pNode;
}


void addNewPartitionNode(PartitionNode **aux, char name[], char id[], FileDiskNode *ptr, int part, int size, char type)
{
    if ((*aux) == NULL)
    {
        (*aux) = newPartitionNode(name, id, ptr, part, size, type);
        return;
    }
    PartitionNode *piv = *aux;
    while (piv->next != NULL) piv = piv->next;
    piv->next = newPartitionNode(name, id, ptr, part, size, type);
}

PartitionNode *getPartitionNode(PartitionNode *aux, char id[])
{
    while (aux != NULL)
    {
        if (!strcmp(aux->id, id)) return aux;
        aux = aux->next;
    }
    return NULL;
}

void deletePartitionNode(PartitionNode **aux, char id[])
{
    if (!strcmp((*aux)->id, id))
    {
        *aux = (*aux)->next;
        return;
    }
    PartitionNode *pivot = *aux;
    while(pivot->next != NULL)
    {
        if (!strcmp(pivot->next->id, id)) break;
        pivot = pivot->next;
    }
    pivot->next = pivot->next->next;
}

void mountPartition(char path[], char name[])
{
    if (path[0] == '\0' || name[0] == '\0')
    {
        printf("Error, parametros incorrectos.\n");
    }
    FILE *diskFile = fopen(path, "rb+");
    if (!diskFile)
    {
        printf("Error, no fue posible abrir el archivo %s\n", path);
        return;
    }
    MasterBootRecord mbr;
    fseek(diskFile, 0, SEEK_SET);
    fread(&mbr, sizeof(MasterBootRecord), 1, diskFile);
    int address = getAddressWithName(mbr.partitions, name);
    int logicalAddress = getLogicalAddresWithName(mbr, name, diskFile);
    int size = 0, start = 0;
    char type = 'P';
    if (address < 0 && logicalAddress < 0)
    {
        printf("Error, %s no posee una particion %s\n", path, name);
        fclose(diskFile);
        return;
    }
    else if (logicalAddress >= 0)
    {
        ExtendedBootRecord ebr;
        fseek(diskFile, logicalAddress, SEEK_SET);
        fread(&ebr, sizeof(ExtendedBootRecord), 1, diskFile);
        start = logicalAddress;
        size = ebr.size;
        type = 'L';
    }
    else if (address >= 0)
    {
        start = mbr.partitions[address].start;
        size = mbr.partitions[address].size;
        type = mbr.partitions[address].type;
    }
    FileDiskNode *dNode = getFileDiskNode(diskList, path);
    if (dNode == NULL)
    {
        addNewFileDiskNode(&diskList, path);
        dNode = getFileDiskNode(diskList, path);
    }
    char id[8] = {'v', 'd', dNode->letter, '\0'};
    char num[4];
    sprintf(num, "%i", dNode->count);
    dNode->count = dNode->count + 1;
    strcat(id, num);
    addNewPartitionNode(&partList, name, id, dNode, start, size, type);
    fclose(diskFile);
}

void unMountPartition(char id[])
{
    if (id[0] == '\0')
    {
        printf("Error, parametros incorrectos.\n");
        return;
    }
    PartitionNode *p = getPartitionNode(partList, id);
    if (p == NULL)
    {
        printf("Error, particion %s no se encuentra montada.\n", id);
        return;
    }
    deletePartitionNode(&partList, id);
    p = partList;
}

void reportDisk(char filePath[], char destiny[])
{
    char dir[512] = {0};
    char command[512] = {0};
    strcpy(dir, destiny);
    getDirectory(dir);
    strcat(command, "mkdir -p -m a=rwx ");
    strcat(command, dir);
    system(command);
    FILE *graph = fopen("/home/sebastian/graph/DiskGraph.dot", "w");
    FILE *disk = fopen(filePath, "rb+");
    MasterBootRecord mbr;
    fread(&mbr, sizeof(MasterBootRecord), 1, disk);

    fprintf(graph, "digraph G{\n");
    fprintf(graph, "node [shape=record, fontname = \"arial\"];\n");
    fprintf(graph, "simple [shape = record, label = \"");
    fprintf(graph, "MBR | ");
    float realSize = mbr.size;
    int pos = sizeof(MasterBootRecord);
    int index = getNextPartition(mbr.partitions, pos);
    while (index != -1)
    {

        Partition p = mbr.partitions[index];
        if (p.start != pos)
        {
            float a = (p.start - pos);
            fprintf(graph, " Libre\\n%.2f%% | ",  a / realSize * 100.0);
        }
        if (p.type == 'P' || p.type == 'p')
        {
            float a = p.size;
            fprintf(graph, " Particion Primaria\\n%.2f%%", a / realSize * 100.0);
        }
        else
        {
            float a = p.size;
            fprintf(graph, " { Particion Extendida\\n%.2f%% | {", a / realSize * 100.0);
            ExtendedBootRecord ebr;
            fseek(disk, p.start, SEEK_SET);
            fread(&ebr, sizeof(ExtendedBootRecord), 1, disk);
            int posE = ebr.start + ebr.size;
            int nextE = ebr.next;
            while(nextE != -1)
            {
                fseek(disk, nextE, SEEK_SET);
                fread(&ebr, sizeof(ExtendedBootRecord), 1, disk);
                if (nextE > posE)
                {
                    float a = nextE - posE;
                    fprintf(graph, " Libre\\n%.2f%% | ",  a / realSize * 100.0);
                }
                float a = ebr.size;
                fprintf(graph, " EBR | Logica\\n%.2f%%", a / realSize * 100.0);
                posE = ebr.start + ebr.size;
                nextE = ebr.next;
                if (nextE != -1)
                {
                    fprintf(graph, " | ");
                }
            }
            if (p.start + p.size > ebr.start + ebr.size)
            {
                float a = (p.start + p.size) - (ebr.start + ebr.size);
                fprintf(graph, " | Libre\\n%.2f%% ", a / realSize * 100.0);
            }
            fprintf(graph, " } } ");
        }
        pos = p.start + p.size;
        index = getNextPartition(mbr.partitions, pos);
        if (index > 0)
        {
            fprintf(graph, " | ");
        }
    }
    if (pos != realSize + sizeof(MasterBootRecord))
    {
        float a = (realSize + sizeof(MasterBootRecord) - pos);
        fprintf(graph, " | Libre\\n%.2f%% ", a / realSize * 100.0);
    }
    fprintf(graph, "\"];\n");
    fprintf(graph, "}");
    fclose(graph);
    fclose(disk);
    char com[512] = {0};
    strcat(com, "dot /home/sebastian/graph/DiskGraph.dot -o ");
    strcat(com, destiny);
    strcat(com, " -Tpng");
    system(com);

}
void reportMbr(char filePath[], char destiny[])
{
    char dir[512] = {0};
    char command[512];
    strcpy(dir, destiny);
    getDirectory(dir);
    strcat(command, "mkdir -p -m a=rwx ");
    strcat(command, dir);
    system(command);
    FILE *graph = fopen("/home/sebastian/graph/DiskRep.dot", "w");
    FILE *disk = fopen(filePath, "rb+");
    MasterBootRecord mbr;
    fread(&mbr, sizeof(MasterBootRecord), 1, disk);

    fprintf(graph, "digraph G{\n");
    fprintf(graph, "MBR [shape=none, margin=0, label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n");
    fprintf(graph, "<TR> <TD COLSPAN =\"2\" >  <FONT FACE=\"boldfontname\"> %s </FONT> </TD> </TR>\n", filePath);
    fprintf(graph, "<TR> <TD> Fit </TD> <TD> %c </TD> </TR>\n", mbr.fit);
    fprintf(graph, "<TR> <TD> Signature </TD> <TD> %i </TD> </TR>\n", mbr.disksignature);
    fprintf(graph, "<TR> <TD> Size </TD> <TD> %i </TD> </TR>\n", mbr.size);
    fprintf(graph, "<TR> <TD> Date </TD> <TD> %s </TD> </TR>\n", mbr.date);
    for (int i = 0; i < 4; i++)
    {
        fprintf(graph, "<TR> <TD COLSPAN =\"2\" >  <FONT FACE=\"boldfontname\"> Partition[%i] </FONT> </TD> </TR>\n", i);
        int st = !(mbr.partitions[i].status == 0);
        fprintf(graph, "<TR> <TD> Status </TD> <TD> %i </TD> </TR>\n", st);
        fprintf(graph, "<TR> <TD> Name </TD> <TD> %s </TD> </TR>\n", mbr.partitions[i].name);
        fprintf(graph, "<TR> <TD> Type </TD> <TD> %c </TD> </TR>\n", mbr.partitions[i].type);
        fprintf(graph, "<TR> <TD> Fit </TD> <TD> %c </TD> </TR>\n", mbr.partitions[i].fit);
        fprintf(graph, "<TR> <TD> Start </TD> <TD> %i </TD> </TR>\n", mbr.partitions[i].start);
        fprintf(graph, "<TR> <TD> Size </TD> <TD> %i </TD> </TR>\n", mbr.partitions[i].size);
    }
    fprintf(graph, "</TABLE>>];\n\n");
    int address = getExtendedIndex(mbr.partitions);
    int next = -1;
    if (address != -1)
    {
        next = mbr.partitions[address].start;
        ExtendedBootRecord ebr;
        fseek(disk, next, SEEK_SET);
        fread(&ebr, sizeof(ExtendedBootRecord), 1, disk);
        next = ebr.next;
    }
    int x = 0;
    while (next != -1)
    {
        ExtendedBootRecord ebr;
        fseek(disk, next, SEEK_SET);
        fread(&ebr, sizeof(ExtendedBootRecord), 1, disk);
        fprintf(graph, "EBR%i [shape=none, margin=0, label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n", x);
        fprintf(graph, "<TR> <TD COLSPAN =\"2\" >  <FONT FACE=\"boldfontname\"> ExtendedBootRecord </FONT> </TD> </TR>\n");
        int st = !(ebr.status == 0);
        fprintf(graph, "<TR> <TD> Status </TD> <TD> %i </TD> </TR>\n", st);
        fprintf(graph, "<TR> <TD> Name </TD> <TD> %s </TD> </TR>\n", ebr.name);
        fprintf(graph, "<TR> <TD> Fit </TD> <TD> %c </TD> </TR>\n", ebr.fit);
        fprintf(graph, "<TR> <TD> Start </TD> <TD> %i </TD> </TR>\n", ebr.start);
        fprintf(graph, "<TR> <TD> Size </TD> <TD> %i </TD> </TR>\n", ebr.size);
        fprintf(graph, "<TR> <TD> Next </TD> <TD> %i </TD> </TR>\n", ebr.next);
        fprintf(graph, "</TABLE>>];\n\n");
        next = ebr.next;
        x++;
    }
    fprintf(graph, "}");
    fclose(graph);
    fclose(disk);
    char com[512] = {0};
    strcat(com, "dot /home/sebastian/graph/DiskRep.dot -o ");
    strcat(com, destiny);
    strcat(com, " -Tpng");
    system(com);
}



/*
 *
 * INICIO DE LA SEGUNDA FASE
 *
 */



struct USER
{
    int nullUser;
    int rootUser;
    int gid;
    int uid;
    PartitionNode *pNode;
};

typedef struct USER User;

struct SUPER_BLOCK
{
    int type;
    int inodesCount;
    int blocksCount;
    int freeBlockCount;
    int freeInodesCount;
    char mountTime[16];
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
    char content[128];
    char date[16];
    int owner;
    int group;
    int perm;
    int next;
    int father;
};

typedef struct JOURNALING Journal;


struct STRING_NODE
{
    char text[16];
    struct STRING_NODE *next;
};

typedef struct STRING_NODE StringNode;

struct STRING_LIST
{
    StringNode *start;
};

typedef struct STRING_LIST StringList;

int getInode3Number(int partSize);
int getInode2Number(int partSize);
void makeFileSystem(char id[], char type[], char fs[]);
void makeExt2(PartitionNode *node, char path[], int n);
void makeExt3(PartitionNode *node, char path[], int n);
void deleteFull(PartitionNode *node, char path[]);
void getDate(char cad[]);
void createFirstFolder(SuperBlock sb, FILE *disk);
void setNextFreeBlock(SuperBlock *sb, FILE *disk);
void setNextFreeInode(SuperBlock *sb, FILE *disk);
int searchInFolder(SuperBlock *sb, FILE *disk, int index, char name[]);
int searchInFolderByLevel(SuperBlock *sb, FILE *disk, int direction, char name[], int level);
int findItemInSystem(SuperBlock *sb, FILE *disk, StringList *list, int address);
int getInodeAddressByIndex(SuperBlock *sb, int index);
int getBlockAddressByIndex(SuperBlock *sb, int index);
void writeBlock(SuperBlock *sb, FILE *disk, void *block, int index);
void writeInode(SuperBlock *sb, FILE *disk, Inode *inode, int index);
void writeInBlockBitmap(SuperBlock *sb, FILE *disk, char value, int index);
void writeInInodeBitmap(SuperBlock *sb, FILE *disk, char value, int index);
int addNewItem(SuperBlock *sb, FILE *disk, int father, char name[], int perm, char content[], int size, int type, int flag);
int addNewItemByLevel(SuperBlock *sb, FILE *disk, int direction, char name[], int father, int level, int perm, char content[], int size, int type);
void createFolder(SuperBlock *sb, FILE *disk, int index, int father, int perm);
int createFile(SuperBlock *sb, FILE *disk, int index, int perm, int size, char path[]);
void pushStringList(StringList *list, char text[]);
void popStringList(StringList *list);
StringList* makePathToList(char path[]);
int makeNewItemInSystem(SuperBlock *sb, FILE *disk, StringList *list, int p, int address, int size, char content[], int perm, int type);
void makeNewDirectory(char path[], int p, int perm, PartitionNode *node);
void makeNewFile(char path[], char content[], int size, int p, int perm, PartitionNode *node);
void addFileBlockFromFile(SuperBlock *sb, FILE *disk, char path[], Inode *inode);
void addFileBlockFromSize(SuperBlock *sb, FILE *disk, int size, Inode *inode);
void addFileBlock(SuperBlock *sb, FILE *disk, FileBlock *block, Inode *node);
void treeReport(PartitionNode *node);
void treeReportInode(SuperBlock *sb, FILE *disk, FILE *dot, int index);
void treeReportFileBlock(SuperBlock *sb, FILE *disk, FILE *dot, int index);
void treeReportFolderBlock(SuperBlock *sb, FILE *disk, FILE *dot, int index);
void bitmapInodeReport(PartitionNode *node, char path[]);
void bitmapBlockReport(PartitionNode *node, char path[]);
void journalReport(PartitionNode *node);
void listInodeReport(PartitionNode *node);
void listBlockReport(PartitionNode *node);
void superBlockReport(PartitionNode *node);
void fileReport(PartitionNode *partNode, char systemPath[], char destinyPath[]);
char *getUserValue(SuperBlock *sb, FILE *disk, int offset, int *size);
void makeGroup(char name[], PartitionNode *node);
void makeUser(char group[], char name[], char pass[], PartitionNode *node);
void writeUserFile(SuperBlock *sb, FILE *disk, char *cont, int size);
void deleteGroup();
Inode newInode();
FolderBlock newFolderBlock();
void loginUser(char name[], char pass[], char id[]);
int getUserInfo(char *cont, char name[], char pass[], int *uid, int *gid);
void logout();
User currentUser;
void pauseThread();
Journal getNewJournal(char content[], char type, char name[], char operation, int owner, int perm, int father);
void writeInJournal(SuperBlock *sb, FILE *disk, char content[], int size, char type, char name[], char operation, int owner, int perm, int father);
void gettingLossed(char id[]);
void recoverDisk(char id[]);
void convertEXT(char id[]);


void report(char id[], char path[], char name[], char file[])
{
    if (id[0] == '\0' || path[0] == '\0' || name[0] == '\0')
    {
        printf("Error, parametros incorrectos.\n");
        return;
    }
    PartitionNode *result = getPartitionNode(partList, id);
    if (!result)
    {
        printf("Error, particion %s no se encuentra montada.\n", id);
        return;
    }
    char diskPath[512] = {0};
    strcpy(diskPath, result->filePtr->path);
    if (!strncasecmp(name, "disk", 4))
    {
        reportDisk(diskPath, path);
    }
    else if (!strncasecmp(name, "mbr", 3))
    {
        reportMbr(diskPath, path);
    }
    else if (!strcasecmp(name, "file")) {
        fileReport(result, file, path);
    }
    else {
        char dir[512] = {0};
        char command[512] = {0};
        strcpy(dir, path);
        getDirectory(dir);
        strcat(command, "mkdir -p -m a=rwx ");
        strcat(command, dir);
        system(command);
        char dest[256] = {0};
        if (!strcasecmp(name, "bm_inode"))
        {
            bitmapInodeReport(result, path);
            return;
        }
        else if (!strcasecmp(name, "bm_block")) {
            bitmapBlockReport(result, path);
            return;
        }
        else if (!strcasecmp(name, "inode")) {
            listInodeReport(result);
            strcpy(dest, "inode_list.dot");
        }
        else if (!strcasecmp(name, "block")) {
            listBlockReport(result);
            strcpy(dest, "block_list.dot");
        }
        else if (!strcasecmp(name, "sb")) {
            superBlockReport(result);
            strcpy(dest, "superblock.dot");
        }
        else if (!strcasecmp(name, "tree")) {
            treeReport(result);
            strcpy(dest, "tree.dot");
        }
        else if (!strcasecmp(name, "journaling")) {
            journalReport(result);
            strcpy(dest, "journal.dot");
        }
        char com[512] = {0};
        strcpy(com, "dot ");
        strcat(com, dest);
        strcat(com, " -o ");
        strcat(com, path);
        strcat(com, " -Tpng");
        system(com);
    }

}


void makeFileSystem(char id[], char type[], char fs[])
{
    PartitionNode *part = getPartitionNode(partList, id);
    if (part == NULL)
    {
        printf("Error, la particion %s no se encuentra montada.\n", id);
        return;
    }
    char path[512] = {0};
    strcpy(path, part->filePtr->path);
    if (strcasecmp(type, "fast"))
    {
        deleteFull(part, path);
    }
    if (fs[0] == '3')
    {
        makeExt3(part, path, getInode3Number(part->size));
        return;
    }
    makeExt2(part, path, getInode2Number(part->size));
}

void pauseThread() {
    printf("Presione cualquier tecla para continuar.\n");
    getchar();
}


void makeExt2(PartitionNode *node, char path[], int n)
{
    SuperBlock sb;
    sb.type = 2;
    sb.inodesCount = n;
    sb.blocksCount = 3 * n;
    sb.freeInodesCount = n - 1;
    sb.freeBlockCount = 3 * n - 1;
    sb.mountCount = 1;
    sb.magic = 0xEF53;
    sb.inodeSize = sizeof(Inode);
    sb.blockSize = sizeof(FolderBlock);
    sb.firstInode = 1;
    sb.firstBlock = 1;
    sb.bmInodeStart = node->start + sizeof(SuperBlock);
    sb.bmBlockStart = sb.bmInodeStart + n;
    sb.inodeStart = sb.bmBlockStart + 3 * n;
    sb.blockStart = sb.inodeStart + sizeof(Inode) * n;
    char date[16] = {0};
    getDate(date);
    strcpy(sb.mountTime, date);
    strcpy(sb.unmountTime, date);
    FILE *disk = fopen(path, "rb+");
    if (disk == NULL) return;
    fseek(disk, node->start, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, disk);
    createFirstFolder(sb, disk);
    fclose(disk);
    makeNewFile("/users.txt", "start.txt", 0, 1, 000, node);
}


void loginUser(char name[], char pass[], char id[])
{
    if (!currentUser.nullUser)
    {
        printf("Error, usuario con sesion iniciado.\n");
    }
    PartitionNode *node = getPartitionNode(partList, id);
    if (node == NULL)
    {
        printf("Error, la particion %s no se encuentra montada.\n", id);
        return;
    }
    SuperBlock sb;
    FILE *disk = fopen(node->filePtr->path, "rb+");
    if (!disk) return;
    fseek(disk, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, disk);
    int size = 0;
    int uid = 0;
    int gid = 0;
    char *content = getUserValue(&sb, disk, 1, &size);
    if (!getUserInfo(content, name, pass, &uid, &gid))
    {
        free(content);
        fclose(disk);
        printf("Error, usuario no encontrado.\n");
        return;
    }
    currentUser.nullUser = 0;
    currentUser.pNode = node;
    currentUser.gid = gid;
    currentUser.uid = uid;
    currentUser.rootUser = strcasecmp(name, "root") ? 0 : 1;
    free(content);
    fclose(disk);
}



int getUserInfo(char *cont, char name[], char pass[], int *uid, int *gid)
{
    char text[1024] = {0};
    strcpy(text, cont);
    char text2[1024] = {0};
    strcpy(text2, cont);
    char newName[11] = {0};
    strcpy(newName, name);
    for (int k = 0; k < 10; k++)
        if (newName[k] == 0) newName[k] = ' ';
    char newPass[11] = {0};
    strcpy(newPass, pass);
    for (int k = 0; k < 10; k++)
        if (newPass[k] == 0) newPass[k] = ' ';
    char *lineToken = strtok(text, "\n");
    int gNumber = -1;
    int uNumber = -1;
    char groupValue[11] = {0};
    for(;lineToken != NULL; lineToken = strtok(NULL, "\n"))
    {
        int number = lineToken[0] - '0';
        if (number == 0) continue;
        char type = lineToken[2];
        if (type == 'G') continue;
        strncpy(groupValue, lineToken + 4, 10);
        char nameValue[11] = {0};
        strncpy(nameValue, lineToken + 15, 10);
        char passValue[11] = {0};
        strncpy(passValue, lineToken + 26, 10);
        if(!strcasecmp(nameValue, newName) && !strcasecmp(passValue, newPass))
        {
            uNumber = number;
            break;
        }
    }
    lineToken = strtok(text2, "\n");
    for(;lineToken != NULL; lineToken = strtok(NULL, "\n"))
    {
        int number = lineToken[0] - '0';
        if (number == 0) continue;
        char type = lineToken[2];
        if (type == 'U') continue;
        char nameValue[11] = {0};
        strncpy(nameValue, lineToken + 4, 10);
        if (!strcasecmp(nameValue, groupValue))
        {
            gNumber = number;
            break;
        }
    }
    if (gNumber < 0 || uNumber < 0)
    {
        return 0;
    }
    *uid = uNumber;
    *gid = gNumber;
    return 1;
}

void logout()
{
    if (currentUser.nullUser)
    {
        printf("Error, no hay ninguna sesion iniciada.\n");
        return;
    }
    currentUser.nullUser = 1;
    currentUser.pNode = NULL;
}


char *getUserValue(SuperBlock *sb, FILE *disk, int offset, int *size)
{
    Inode inode;
    fseek(disk, getInodeAddressByIndex(sb, 1), SEEK_SET);
    fread(&inode, sizeof(Inode), 1, disk);
    char *cont = (char *) malloc(sizeof(char) * (inode.size + offset));
    int it = 0;
    for (int i = 0; i < 12; i++)
    {
        if (inode.block[i] == -1) continue;
        FileBlock fb;
        fseek(disk, getBlockAddressByIndex(sb, inode.block[i]), SEEK_SET);
        fread(&fb, sizeof(FileBlock), 1, disk);
        for(int j = 0; j < 64 && fb.content[j] != 0; j++)
        {
            cont[it] = fb.content[j];
            it++;
        }
    }
    cont[inode.size] = 0;
    *size = inode.size;
    return cont;
}

void makeGroup(char name[], PartitionNode *node)
{
    if (!node) {
        printf("Error, no se ha iniciado sesión en ninguna partición.\n");
        return;
    }
    if (!currentUser.rootUser) {
        printf("Error, no puedes realizar esta acción en este momento.\n");
        return;
    }
    char newGroup[11] = {0};
    strcpy(newGroup, name);
    for (int k = 0; k < 10; k++)
    {
        if (newGroup[k] == 0) newGroup[k] = ' ';
    }
    int size = 0;
    SuperBlock sb;
    FILE *disk = fopen(node->filePtr->path, "rb+");
    fseek(disk, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, disk);
    char *content = getUserValue(&sb, disk, 16, &size);
    char *aux = (char *) malloc(sizeof(char) * (size + 16));
    strcpy(aux, content);
    int next = 1;
    int doit = 1;
    char *lineToken = strtok(aux, "\n");
    for(;lineToken != NULL; lineToken = strtok(NULL, "\n"))
    {
        int number = lineToken[0] - '0';
        if (number == 0) continue;
        char type = lineToken[2];
        char nameValue[11] = {0};
        strncpy(nameValue, lineToken + 4, 10);
        if (type == 'U') continue;
        if (!strcasecmp(newGroup, nameValue))
        {
            doit = 0;
            break;
        }
        if (number >= next) next = number + 1;
    }
    if (!doit)
    {
        printf("Erorr, el grupo ya existe.\n");
        free(content);
        free(aux);
        fclose(disk);
        return;
    }
    char newV[16] ={0};
    newV[0] = next + '0';
    newV[1] = ',';
    newV[2] = 'G';
    newV[3] = ',';
    strcat(newV, newGroup);
    strcat(newV, "\n");
    strcat(content, newV);
    writeUserFile(&sb, disk, content, size + 16);
    free(content);
    free(aux);
    fclose(disk);
}

void makeUser(char group[], char name[], char pass[], PartitionNode *node)
{
    if (!node) {
        printf("Error, no se ha iniciado sesión en ninguna partición.\n");
        return;
    }
    if (!currentUser.rootUser) {
        printf("Error, no puedes realizar esta acción en este momento.\n");
        return;
    }
    char newGroup[11] = {0};
    strcpy(newGroup, group);
    for (int k = 0; k < 10; k++)
        if (newGroup[k] == 0) newGroup[k] = ' ';
    char newName[11] = {0};
    strcpy(newName, name);
    for (int k = 0; k < 10; k++)
        if (newName[k] == 0) newName[k] = ' ';
    char newPass[11] = {0};
    strcpy(newPass, pass);
    for (int k = 0; k < 10; k++)
        if (newPass[k] == 0) newPass[k] = ' ';
    int size = 0;
    SuperBlock sb;
    FILE *disk = fopen(node->filePtr->path, "rb+");
    fseek(disk, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, disk);
    char *content = getUserValue(&sb, disk, 38, &size);
    char *aux = (char *) malloc(sizeof(char) * (size + 38));
    strcpy(aux, content);
    int next = 1;
    int doit = 1;
    int grp = 0;
    char *lineToken = strtok(aux, "\n");
    for(;lineToken != NULL; lineToken = strtok(NULL, "\n"))
    {
        int number = lineToken[0] - '0';
        if (number == 0) continue;
        char type = lineToken[2];
        if (type == 'G')
        {
            char nameValue[11] = {0};
            strncpy(nameValue, lineToken + 4, 10);
            if (!strcasecmp(nameValue, newGroup))
            {
                grp = 1;
            }
            continue;
        }
        char groupValue[11] = {0};
        strncpy(groupValue, lineToken + 4, 10);
        char nameValue[11] = {0};
        strncpy(nameValue, lineToken + 15, 10);
        if(!strcasecmp(nameValue, newName))
        {
            doit = 0;
            break;
        }
        if (number >= next) next = number + 1;

    }
    if (!doit)
    {
        printf("Error, el usuario no existe.\n");
        free(content);
        free(aux);
        fclose(disk);
        return;
    }
    if (!grp)
    {
        printf("Error, el grupo no existe.\n");
        free(content);
        free(aux);
        fclose(disk);
        return;
    }
    char newV[38] ={0};
    newV[0] = next + '0';
    newV[1] = ',';
    newV[2] = 'U';
    newV[3] = ',';
    strcat(newV, newGroup);
    strcat(newV, ",");
    strcat(newV, newName);
    strcat(newV, ",");
    strcat(newV, newPass);
    strcat(newV, "\n");
    strcat(content, newV);
    writeUserFile(&sb, disk, content, size + 38);
    free(content);
    free(aux);
    fclose(disk);
}

void writeUserFile(SuperBlock *sb, FILE *disk, char *cont, int size)
{
    Inode inode;
    fseek(disk, getInodeAddressByIndex(sb, 1), SEEK_SET);
    fread(&inode, sizeof(inode), 1, disk);
    for(int i = 0; i < 12; i++)
        if (inode.block[i] != -1)
        {
            writeInBlockBitmap(sb, disk, 0, inode.block[i]);
            inode.block[i] = -1;
        }
    int a = 0;
    int k = 0;
    FileBlock fb;
    for (; k < size; k++, a++)
    {
        if (a == 64)
        {
            addFileBlock(sb, disk, &fb, &inode);
            a = 0;
        }
        fb.content[a] = cont[k];
    }
    int b = a != 0;
    for (; a < 64 && b; a++)
    {
        fb.content[a] = 0;
    }
    if (b) addFileBlock(sb, disk, &fb, &inode);
    inode.size = size - 1;
    writeInode(sb, disk, &inode, 1);
}

void gettingLossed(char id[])
{
    PartitionNode *node = getPartitionNode(partList, id);
    if (!node)
    {
        printf("Error, la particion %s no se encuentra montada.\n", id);
        return;
    }
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    if (!diskFile) return;
    SuperBlock sb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    if (sb.type != 3) {
        printf("Error, no puede simularse la perdida de esta particion.\n");
        fclose(diskFile);
        return;
    }
    int n = getInode3Number(node->size);
    sb.inodesCount = n;
    sb.blocksCount = 3 * n;
    sb.freeInodesCount = n - 1;
    sb.freeBlockCount = 3 * n - 1;
    sb.firstInode = 1;
    sb.firstBlock = 1;
    fseek(diskFile, node->start, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, diskFile);
    int first = sb.bmInodeStart;
    int last = node->start + node->size;
    char zero[1024] = {0};
    for (int i = first; i < last; i += 1024)
    {
        fseek(diskFile, i, SEEK_SET);
        fwrite(zero, sizeof(char) * 1024, 1, diskFile);
    }
    createFirstFolder(sb, diskFile);
    fclose(diskFile);
    int flag = currentUser.nullUser;
    if (!flag) currentUser.nullUser = 1;
    makeNewFile("/users.txt", "start.txt", 0, 1, 000, node);
    if (!flag) currentUser.nullUser = 0;
}

void recoverDisk(char id[])
{
    PartitionNode *node = getPartitionNode(partList, id);
    if (!node)
    {
        printf("Error, la particion %s no se encuentra montada.\n", id);
        return;
    }
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    if (!diskFile) return;
    SuperBlock sb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    if (sb.type != 3) {
        printf("Error, no puede recuperarse esta esta particion.\n");
        fclose(diskFile);
        return;
    }
    Journal aux;
    fseek(diskFile, node->start + sizeof(SuperBlock), SEEK_SET);
    fread(&aux, sizeof(Journal), 1, diskFile);
    while (aux.next != -1) {
        fseek(diskFile, aux.next, SEEK_SET);
        fread(&aux, sizeof(Journal), 1, diskFile);
        int size = 0;
        char cont[128] = {0};
        if (isdigit(aux.content[0]) && aux.type) {
            size = atoi(aux.content);
        }
        else if (aux.type) {
            strcpy(cont, aux.content);
        }
        int add = addNewItem(&sb, diskFile, aux.father, aux.name, aux.perm, cont, size, aux.type, 0);
        Inode inode;
        int sek = getInodeAddressByIndex(&sb, add);
        fseek(diskFile, sek, SEEK_SET);
        fread(&inode, sizeof(Inode), 1, diskFile);
        inode.uid = aux.owner;
        inode.gid = aux.group;
        fseek(diskFile, sek, SEEK_SET);
        fwrite(&inode, sizeof(Inode), 1, diskFile);
    }
    fseek(diskFile, node->start, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, diskFile);
    fclose(diskFile);
}

void convertEXT(char id[])
{
    PartitionNode *node = getPartitionNode(partList, id);
    if (!node)
    {
        printf("Error, la particion %s no se encuentra montada.\n", id);
        return;
    }
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    if (!diskFile) return;
    SuperBlock sb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    if (sb.type != 2) {
        printf("Error, no puede convertirse esta particion.\n");
        fclose(diskFile);
        return;
    }
    sb.type = 3;
    int n = getInode3Number(node->size);
    int newInodeCount = n;
    int newBlockCount = 3 * n;
    int newFreeInode = newInodeCount - (sb.inodesCount - sb.freeInodesCount);
    int newFreeBlock = newBlockCount - (sb.blocksCount - sb.freeBlockCount);
    int newBmInodeStart = node->start + sizeof(SuperBlock) + sizeof(Journal) * n;
    int newBmBlockStart = newBmInodeStart + n;
    int newInodeStart = newBmBlockStart + 3 * n;
    int newBlockStart = newInodeStart + sizeof(Inode) * n;
    int lastInode = n - 1;
    int lastBlock = 3 * n - 1;
    int finaln = -1;
    int startn = -1;
    int finalo = -1;
    finaln = newBlockStart + 3 * n * sizeof(FileBlock) - (sizeof(FileBlock));
    startn = newBlockStart;
    finalo = getBlockAddressByIndex(&sb, lastBlock);
    int j = finalo;
    for (int i = finaln; i >= startn; i -= sizeof(FileBlock), j -= sizeof(FileBlock))
    {
        FileBlock origin;
        fseek(diskFile, j, SEEK_SET);
        fread(&origin, sizeof(FileBlock), 1, diskFile);
        fseek(diskFile, i, SEEK_SET);
        fwrite(&origin, sizeof(FileBlock), 1, diskFile);
    }
    finaln = newBlockStart - (sizeof(Inode));
    startn = newInodeStart;
    finalo = getInodeAddressByIndex(&sb, lastInode);
    j = finalo;
    for (int i = finaln; i >= startn; i -= sizeof(Inode), j -= sizeof(Inode))
    {
        Inode origin;
        fseek(diskFile, j, SEEK_SET);
        fread(&origin, sizeof(Inode), 1, diskFile);
        fseek(diskFile, i, SEEK_SET);
        fwrite(&origin, sizeof(Inode), 1, diskFile);
    }
    finaln = newInodeStart - 1;
    startn = newBmBlockStart;
    finalo = sb.bmBlockStart + lastBlock;
    j = finalo;
    for (int i = finaln; i >= startn; i--, j--)
    {
        char origin;
        fseek(diskFile, j, SEEK_SET);
        fread(&origin, sizeof(char), 1, diskFile);
        fseek(diskFile, i, SEEK_SET);
        fwrite(&origin, sizeof(char), 1, diskFile);
    }
    finaln = newBmBlockStart - 1;
    startn = newBmInodeStart;
    finalo = sb.bmInodeStart + lastInode;
    j = finalo;
    for (int i = finaln; i >= startn; i--, j--)
    {
        char origin;
        fseek(diskFile, j, SEEK_SET);
        fread(&origin, sizeof(char), 1, diskFile);
        fseek(diskFile, i, SEEK_SET);
        fwrite(&origin, sizeof(char), 1, diskFile);
    }
    sb.blocksCount = newBlockCount;
    sb.inodesCount = newInodeCount;
    sb.freeInodesCount = newFreeInode;
    sb.freeBlockCount = newFreeBlock;
    sb.bmInodeStart = newBmInodeStart;
    sb.bmBlockStart = newBmBlockStart;
    sb.inodeStart = newInodeStart;
    sb.blockStart = newBlockStart;
    fseek(diskFile, node->start, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, diskFile);
    Journal jour = getNewJournal("0", 0, "#START", 0, 0, 0, 0);
    fseek(diskFile, node->start + sizeof(SuperBlock), SEEK_SET);
    fwrite(&jour, sizeof(Journal), 1, diskFile);
    fclose(diskFile);
}

void makeExt3(PartitionNode *node, char path[], int n)
{
    SuperBlock sb;
    sb.type = 3;
    sb.inodesCount = n;
    sb.blocksCount = 3 * n;
    sb.freeInodesCount = n - 1;
    sb.freeBlockCount = 3 * n - 1;
    sb.mountCount = 1;
    sb.magic = 0xEF53;
    sb.inodeSize = sizeof(Inode);
    sb.blockSize = sizeof(FolderBlock);
    sb.firstInode = 1;
    sb.firstBlock = 1;
    sb.bmInodeStart = node->start + sizeof(SuperBlock) + sizeof(Journal) * n;
    sb.bmBlockStart = sb.bmInodeStart + n;
    sb.inodeStart = sb.bmBlockStart + 3 * n;
    sb.blockStart = sb.inodeStart + sizeof(Inode) * n;
    char date[16] = {0};
    getDate(date);
    strcpy(sb.mountTime, date);
    strcpy(sb.unmountTime, date);
    FILE *disk = fopen(path, "rb+");
    if (disk == NULL) return;
    fseek(disk, node->start, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, disk);
    createFirstFolder(sb, disk);
    Journal j = getNewJournal("0", 0, "#START", 0, 0, 0, 0);
    fseek(disk, node->start + sizeof(SuperBlock), SEEK_SET);
    fwrite(&j, sizeof(Journal), 1, disk);
    fclose(disk);
    makeNewFile("/users.txt", "start.txt", 0, 1, 000, node);
}

void createFirstFolder(SuperBlock sb, FILE *disk)
{
    Inode rootNode = newInode();
    rootNode.gid = 1;
    rootNode.uid = 1;
    rootNode.block[0] = 0;
    fseek(disk, sb.inodeStart, SEEK_SET);
    fwrite(&rootNode, sizeof(Inode), 1, disk);
    FolderBlock fb = newFolderBlock();
    fb.content[0].inode = 0;
    strcpy(fb.content[0].name, ".");
    fb.content[1].inode = 0;
    strcpy(fb.content[1].name, "..");
    fseek(disk, sb.blockStart, SEEK_SET);
    fwrite(&fb, sizeof(FolderBlock), 1, disk);
    char w = 1;
    fseek(disk, sb.bmBlockStart, SEEK_SET);
    fwrite(&w, sizeof(char), 1, disk);
    fseek(disk, sb.bmInodeStart, SEEK_SET);
    fwrite(&w, sizeof(char), 1, disk);
}

FolderBlock newFolderBlock()
{
    FolderBlock block;
    for(int i = 0; i < 4; i++)
    {
        block.content[i].inode = -1;
        strcpy(block.content[i].name, "");
    }
    return block;
}

Inode newInode()
{
    Inode node;
    node.uid = 0;
    node.gid = 0;
    node.size = 0;
    char date[16] = {0};
    getDate(date);
    strcpy(node.aTime, date);
    strcpy(node.cTime, date);
    strcpy(node.mTime, date);
    node.type = 0;
    node.perm = 0;
    for(int i = 0; i < 15; i++)
    {
        node.block[i] = -1;
    }
    return node;
}


Journal getNewJournal(char content[], char type, char name[], char operation, int owner, int perm, int father) {
    Journal j;
    strcpy(j.content, content);
    char date[16] = {0};
    getDate(date);
    strcpy(j.date, date);
    strcpy(j.name, name);
    j.type = type;
    j.next = -1;
    j.operation = operation;
    j.owner = owner;
    j.perm = perm;
    j.father = father;
    return j;
}

void getDate(char cad[])
{
    time_t t = time(0);
    struct tm *current = localtime(&t);
    strftime(cad, 128, "%d/%m/%y %H:%M", current);
}

void deleteFull(PartitionNode *node, char path[])
{
    char zero[1024] = {'\0'};
    FILE *disk = fopen(path, "rb+");
    if (disk == NULL) return;
    int first = node->start;
    int last = node->start + node->size;
    for (int i = first; i < last; i += 1024)
    {
        fseek(disk, i, SEEK_SET);
        fwrite(zero, sizeof(char) * 1024, 1, disk);
    }
    fclose(disk);
}

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

void setNextFreeBlock(SuperBlock *sb, FILE *disk)
{
    char aux = 0;
    for (int i = 0; i < sb->blocksCount; i++)
    {
        fseek(disk, i + sb->bmBlockStart, SEEK_SET);
        fread(&aux, sizeof(char), 1, disk);
        if (aux == 0)
        {
            sb->firstBlock = i;
            break;
        }
    }
}

void setNextFreeInode(SuperBlock *sb, FILE *disk)
{
    char aux = 0;
    for (int i = 0; i < sb->inodesCount; i++)
    {
        fseek(disk, i + sb->bmInodeStart, SEEK_SET);
        fread(&aux, sizeof(char), 1, disk);
        if (aux == 0)
        {
            sb->firstInode = i;
            break;
        }
    }
}

int searchInFolder(SuperBlock *sb, FILE *disk, int index, char name[])
{
    int adrs = getInodeAddressByIndex(sb, index);
    Inode in;
    fseek(disk, adrs, SEEK_SET);
    fread(&in, sizeof(Inode), 1, disk);
    if (in.type != 0) return -1;
    for (int i = 0; i < 15; i++)
    {
        int level = 0;
        int res = -1;
        if (i == 12) level = 1;
        else if (i == 13) level = 2;
        else if (i == 14) level = 3;
        if (in.block[i] == -1) continue;
        res = searchInFolderByLevel(sb, disk, in.block[i], name, level);
        if (res != -1) return res;
    }
    return -1;
}

int searchInFolderByLevel(SuperBlock *sb, FILE *disk, int direction, char name[], int level)
{
    if (level == 0)
    {
        int adrs = getBlockAddressByIndex(sb, direction);
        FolderBlock folbl;
        fseek(disk, adrs, SEEK_SET);
        fread(&folbl, sizeof(FolderBlock), 1, disk);
        for (int i = 0; i < 4; i++)
        {
            if (!strcmp(folbl.content[i].name, name))
                return folbl.content[i].inode;
        }
        return -1;
    }
    return -1;
}

void writeBlock(SuperBlock *sb, FILE *disk, void *block, int index)
{
    int adrs = getBlockAddressByIndex(sb, index);
    fseek(disk, adrs, SEEK_SET);
    fwrite(block, sizeof(FileBlock), 1, disk);
}

void writeInode(SuperBlock *sb, FILE *disk, Inode *inode, int index)
{
    int adrs = getInodeAddressByIndex(sb, index);
    fseek(disk, adrs, SEEK_SET);
    fwrite(inode, sizeof(Inode), 1, disk);
}

int getInodeAddressByIndex(SuperBlock *sb, int index)
{
    return sb->inodeStart + sizeof(Inode) * index;
}

int getBlockAddressByIndex(SuperBlock *sb, int index)
{
    return sb->blockStart + sizeof(FolderBlock) * index;
}

void writeInBlockBitmap(SuperBlock *sb, FILE *disk, char value, int index)
{
    int adrs = sb->bmBlockStart + index;
    fseek(disk, adrs, SEEK_SET);
    fwrite(&value, sizeof(char), 1, disk);
    setNextFreeBlock(sb, disk);
    if (value == 1) sb->freeBlockCount = sb->freeBlockCount - 1;
    else sb->freeBlockCount = sb->freeBlockCount + 1;
}

void writeInInodeBitmap(SuperBlock *sb, FILE *disk, char value, int index)
{
    int adrs = sb->bmInodeStart + index;
    fseek(disk, adrs, SEEK_SET);
    fwrite(&value, sizeof(char), 1, disk);
    setNextFreeInode(sb, disk);
    if (value == 1) sb->freeInodesCount = sb->freeInodesCount - 1;
    else sb->freeInodesCount = sb->freeInodesCount + 1;
}

void writeInJournal(SuperBlock *sb, FILE *disk, char content[], int size, char type, char name[], char operation, int owner, int perm, int father)
{
    if (sb->type != 3) return;
    if (currentUser.nullUser) return;
    size = size * type;
    char newContent[128] = {0};
    if (!type) strcpy(newContent, "");
    else if (content[0] == 0) sprintf(newContent, "%i", size);
    else strcpy(newContent, content);
    int start = currentUser.pNode->start + sizeof(SuperBlock);
    Journal journal;
    Journal newJournal = getNewJournal(newContent, type, name, operation, owner, perm, father);
    newJournal.group = currentUser.gid;
    fseek(disk, start, SEEK_SET);
    fread(&journal, sizeof(Journal), 1, disk);
    while (journal.next != -1)
    {
        start += sizeof(Journal);
        fseek(disk, start, SEEK_SET);
        fread(&journal, sizeof(Journal), 1, disk);
    }
    journal.next = start + sizeof(Journal);
    fseek(disk, start, SEEK_SET);
    fwrite(&journal, sizeof(Journal), 1, disk);
    fseek(disk, start + sizeof(Journal), SEEK_SET);
    fwrite(&newJournal, sizeof(Journal), 1, disk);
}

int addNewItem(SuperBlock *sb, FILE *disk, int father, char name[], int perm, char content[], int size, int type, int flag)
{
    int adrs = getInodeAddressByIndex(sb, father);
    Inode in;
    fseek(disk, adrs, SEEK_SET);
    fread(&in, sizeof(Inode), 1, disk);
    if (in.type != 0) return -1;
    //VERIFICAR PERMISOS
    if (flag) writeInJournal(sb, disk, content, size, type, name, 0, currentUser.uid, perm, father);
    for (int i = 0; i < 15; i++)
    {
        int level = 0;
        int res = -1;
        if (i == 12) level = 1;
        else if (i == 13) level = 2;
        else if (i == 14) level = 3;
        if (in.block[i] == -1 && i < 12)
        {
            int new = sb->firstBlock;
            in.block[i] = new;
            writeInode(sb, disk, &in, father);
            FolderBlock fb = newFolderBlock();
            writeInBlockBitmap(sb, disk, 1, new);
            writeBlock(sb, disk, &fb, new);
            res = addNewItemByLevel(sb, disk, in.block[i], name, father, level, perm, content, size, type);
        }
        else
        {
            res = addNewItemByLevel(sb, disk, in.block[i], name, father, level, perm, content, size, type);
        }
        if (res != -1) return res;
    }
    return -1;
}

int addNewItemByLevel(SuperBlock *sb, FILE *disk, int direction, char name[], int father, int level, int perm, char content[], int size, int type)
{
    if (level == 0)
    {
        int adrs = getBlockAddressByIndex(sb, direction);
        FolderBlock folbl;
        fseek(disk, adrs, SEEK_SET);
        fread(&folbl, sizeof(FolderBlock), 1, disk);
        for (int i = 0; i < 4; i++)
        {
            if (folbl.content[i].inode == -1)
            {
                int new = sb->firstInode;
                folbl.content[i].inode = new;
                strcpy(folbl.content[i].name, name);
                if (type == 1)
                {
                    if (!createFile(sb, disk, new, perm, size, content)) return -1;
                }
                else createFolder(sb, disk, new, father, perm);
                writeBlock(sb, disk, &folbl, direction);
                return new;
            }
        }
        return -1;
    }
    return -1;
}


void createFolder(SuperBlock *sb, FILE *disk, int index, int father, int perm)
{
    Inode node = newInode();
    node.perm = perm;
    node.gid = currentUser.gid;
    node.uid = currentUser.gid;
    FolderBlock fb = newFolderBlock();
    strcpy(fb.content[0].name, ".");
    strcpy(fb.content[1].name, "..");
    fb.content[0].inode = index;
    fb.content[1].inode = father;
    int newblc = sb->firstBlock;
    node.block[0] = newblc;
    writeInInodeBitmap(sb, disk, 1, index);
    writeInBlockBitmap(sb, disk, 1, newblc);
    writeInode(sb, disk, &node, index);
    writeBlock(sb, disk, &fb, newblc);
}

int makeNewItemInSystem(SuperBlock *sb, FILE *disk, StringList *list, int p, int address, int size, char content[], int perm, int type)
{
    Inode node;
    int a = getInodeAddressByIndex(sb, address);
    fseek(disk, a, SEEK_SET);
    fread(&node, sizeof(Inode), 1, disk);
    if (node.type != 0) return - 1;
    if (list->start != NULL && list->start->next == NULL)
    {
        int val = searchInFolder(sb, disk, address, list->start->text);
        if (val != -1) return -1;
        return addNewItem(sb, disk, address, list->start->text, perm, content, size, type, 1);
    }
    int nextAddress = searchInFolder(sb, disk, address, list->start->text);
    if (nextAddress < 0 && p) nextAddress = addNewItem(sb, disk, address, list->start->text, perm, content, size, 0, 1);
    if (nextAddress < 0) return -1;
    popStringList(list);
    return makeNewItemInSystem(sb, disk, list, p, nextAddress, size, content, perm, type);
}

int findItemInSystem(SuperBlock *sb, FILE *disk, StringList *list, int address)
{
    if (list->start == NULL) return address;
    Inode node;
    int a = getInodeAddressByIndex(sb, address);
    fseek(disk, a, SEEK_SET);
    fread(&node, sizeof(Inode), 1, disk);
    if (node.type != 0) return address;
    int next = searchInFolder(sb, disk, address, list->start->text);
    if (next < 0) return -1;
    popStringList(list);
    return findItemInSystem(sb, disk, list, next);
}

void makeNewDirectory(char path[], int p, int perm, PartitionNode *node)
{
    if (!node) {
        printf("Error, no hay ninguna sesion iniciada.\n");
        return;
    }
    if (path[0] == 0)
    {
        printf("Error, el parametro _path_ se encuentra vacio.\n");
        return;
    }
    SuperBlock sb;
    StringList *slist = makePathToList(path);
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    if (!diskFile) return;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    makeNewItemInSystem(&sb, diskFile, slist, p, 0, 0, "", perm, 0);
    fseek(diskFile, node->start, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, diskFile);
    fclose(diskFile);
}

void makeNewFile(char path[], char content[], int size, int p, int perm, PartitionNode *node)
{
    if (!node) {
        printf("Error, no hay ninguna sesion iniciada.\n");
        return;
    }
    if (path[0] == 0)
    {
        printf("Error, el parametro _path_ se encuentra vacio.\n");
        return;
    }
    SuperBlock sb;
    StringList *slist = makePathToList(path);
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    if (!diskFile) return;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    makeNewItemInSystem(&sb, diskFile, slist, p, 0, size, content, perm, 1);
    fseek(diskFile, node->start, SEEK_SET);
    fwrite(&sb, sizeof(SuperBlock), 1, diskFile);
    fclose(diskFile);
}

void pushStringList(StringList *list, char text[])
{
    StringNode *node = (StringNode*) malloc(sizeof(StringNode));
    node->next = NULL;
    strcpy(node->text, text);
    if (list->start == NULL)
    {
        list->start = node;
        return;
    }
    StringNode *aux = list->start;
    while(aux->next != NULL) aux = aux->next;
    aux->next = node;
}

void popStringList(StringList *list)
{
    StringNode *aux = list->start;
    list->start = list->start->next;
    free(aux);
}

StringList* makePathToList(char path[])
{
    int i = 0;
    StringList *slist = (StringList*) malloc(sizeof(StringList));
    slist->start = NULL;
    if (path[i] == '/') i++;
    char aux[16] = {0};
    for (; i < 512 && path[i] != 0; i++)
    {
        if (path[i] == '/')
        {
            pushStringList(slist, aux);
            memset(&aux, '\0', sizeof(aux));
            continue;
        }
        char ccat[2] = {path[i], 0};
        strcat(aux, ccat);
    }
    if(aux[0] != 0)
    {
        pushStringList(slist, aux);
    }
    return slist;
}

int createFile(SuperBlock *sb, FILE *disk, int index, int perm, int size, char path[])
{
    if (path[0] == 0 && size < 0) return 0;
    Inode node = newInode();
    node.perm = perm;
    node.type = 1;
    node.size = size;
    node.uid = currentUser.uid;
    node.gid = currentUser.gid;
    if (path[0] != 0) addFileBlockFromFile(sb, disk, path, &node);
    else addFileBlockFromSize(sb, disk, size, &node);
    writeInInodeBitmap(sb, disk, 1, index);
    writeInode(sb, disk, &node, index);
    return 1;
}

void addFileBlockFromFile(SuperBlock *sb, FILE *disk, char path[], Inode *inode)
{
    FILE *cont = fopen(path, "r");
    if (!cont) return;
    FileBlock fb;
    int size = 0;
    int i = 0;
    int get = 0;
    while ((get = fgetc(cont)) != EOF)
    {
        if (i == 64)
        {
            addFileBlock(sb, disk, &fb, inode);
            i = 0;
        }
        fb.content[i] = (char) get;
        i++;
        size++;
    }
    int b = i != 0;
    for (; i < 64 && b; i++)
    {
        fb.content[i] = 0;
    }
    if (b) addFileBlock(sb, disk, &fb, inode);
    fclose(cont);
    inode->size = size;
}


void addFileBlockFromSize(SuperBlock *sb, FILE *disk, int size, Inode *inode)
{
    char *buffer = (char*) malloc(sizeof(char) * (size + 1));
    int i = 0;
    while (i < size)
    {
        for (int j = 0; j < 10 && i < size; j++)
        {
            buffer[i] = j + '0';
            i++;
        }
    }
    buffer[size] = '\0';
    int a = 0;
    int k = 0;
    FileBlock fb;
    for (; k < size; k++, a++)
    {
        if (a == 64)
        {
            addFileBlock(sb, disk, &fb, inode);
            a = 0;
        }
        fb.content[a] = buffer[k];
    }
    int b = a != 0;
    for (; a < 64 && b; a++)
    {
        fb.content[a] = 0;
    }
    if (b) addFileBlock(sb, disk, &fb, inode);
    free(buffer);
}


void addFileBlock(SuperBlock *sb, FILE *disk, FileBlock *block, Inode *node)
{
    for (int i = 0; i < 15; i++)
    {
        if (node->block[i] == -1 && i < 12)
        {
            int ad = sb->firstBlock;
            node->block[i] = ad;
            writeInBlockBitmap(sb, disk, 1, ad);
            writeBlock(sb, disk, block, ad);
            break;
        }
    }
}


void treeReport(PartitionNode *node)
{
    FILE *graph = fopen("tree.dot", "w");
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    SuperBlock superb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&superb, sizeof(superb), 1, diskFile);
    fprintf(graph, "digraph g {\n");
    fprintf(graph, "rankdir = LR;\n");
    fprintf(graph, "graph [fontname = \"arial\"];\n");
    fprintf(graph, "node [fontname = \"arial\"];\n");
    fprintf(graph, "edge [fontname = \"arial\"];\n");
    treeReportInode(&superb, diskFile, graph, 0);
    fprintf(graph, "}\n");
    fclose(graph);
    fclose(diskFile);
}

void treeReportInode(SuperBlock *sb, FILE *disk, FILE *dot, int index)
{
    Inode inode;
    fseek(disk, getInodeAddressByIndex(sb, index), SEEK_SET);
    fread(&inode, sizeof(Inode), 1, disk);
    char *color = inode.type ? "greenyellow" : "goldenrod1";
    fprintf(dot, "\tin%i [shape=none, margin=0, label =<\n", index);
    fprintf(dot, "\t\t<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" bgcolor=\"%s\">\n", color);
    fprintf(dot, "\t\t\t<tr> <td colspan = \"2\" PORT=\"p\">Inode %i</td> </tr>\n", index);
    fprintf(dot, "\t\t\t<tr> <td>UID</td> <td>%i</td> </tr>\n", inode.uid);
    fprintf(dot, "\t\t\t<tr> <td>GID</td> <td>%i</td> </tr>\n", inode.gid);
    fprintf(dot, "\t\t\t<tr> <td>Size</td> <td>%i</td> </tr>\n", inode.size);
    fprintf(dot, "\t\t\t<tr> <td>aTime</td> <td>%s</td> </tr>\n", inode.aTime);
    fprintf(dot, "\t\t\t<tr> <td>cTime</td> <td>%s</td> </tr>\n", inode.cTime);
    fprintf(dot, "\t\t\t<tr> <td>mTime</td> <td>%s</td> </tr>\n", inode.mTime);
    fprintf(dot, "\t\t\t<tr> <td>Type</td> <td>%i</td> </tr>\n", inode.type);
    fprintf(dot, "\t\t\t<tr> <td>Perm</td> <td>%i</td> </tr>\n", inode.perm);
    for (int i = 0; i < 15; i++)
    {
        fprintf(dot, "\t\t\t<tr> <td>B%i</td> <td PORT = \"f%i\">%i</td> </tr>\n", i + 1, i, inode.block[i]);
    }
    fprintf(dot, "\t</table> >];\n\n");
    for (int i = 0; i < 12; i++)
    {
        if (inode.block[i] != -1 && inode.type == 0)
        {
            treeReportFolderBlock(sb, disk, dot, inode.block[i]);
            fprintf(dot, "\tin%i:f%i -> bl%i:p;\n", index, i, inode.block[i]);
        }
        else if (inode.block[i] != -1 && inode.type == 1)
        {
            treeReportFileBlock(sb, disk, dot, inode.block[i]);
            fprintf(dot, "\tin%i:f%i -> bl%i:p;\n", index, i, inode.block[i]);
        }
    }
}

void treeReportFileBlock(SuperBlock *sb, FILE *disk, FILE *dot, int index)
{
    FileBlock flBlock;
    fseek(disk, getBlockAddressByIndex(sb, index), SEEK_SET);
    fread(&flBlock, sizeof(FileBlock), 1, disk);
    fprintf(dot, "\tbl%i [shape=none, margin=0, label =<\n", index);
    fprintf(dot, "\t\t<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" bgcolor=\"lightblue\">\n");
    fprintf(dot, "\t\t\t<tr> <td colspan = \"2\" PORT=\"p\">Block %i</td> </tr>\n", index);
    fprintf(dot, "\t\t\t<tr> <td colspan = \"2\"> ");
    for(int i = 0; i < 64 && flBlock.content[i] != 0; i++)
    {
        fprintf(dot, "%c", flBlock.content[i]);
    }
    fprintf(dot, " </td> </tr>\n");
    fprintf(dot, "\t</table> >];\n\n");
}

void treeReportFolderBlock(SuperBlock *sb, FILE *disk, FILE *dot, int index)
{
    FolderBlock foBlock;
    fseek(disk, getBlockAddressByIndex(sb, index), SEEK_SET);
    fread(&foBlock, sizeof(FolderBlock), 1, disk);
    fprintf(dot, "\tbl%i [shape=none, margin=0, label =<\n", index);
    fprintf(dot, "\t\t<table border=\"0\" cellborder=\"1\" cellspacing=\"0\" bgcolor=\"sandybrown\">\n");
    fprintf(dot, "\t\t\t<tr> <td colspan = \"2\" PORT=\"p\">Block %i</td> </tr>\n", index);
    for (int i = 0; i < 4; i++)
    {
        fprintf(dot, "\t\t\t<tr> <td>%s</td> <td PORT = \"f%i\">%i</td> </tr>\n", foBlock.content[i].name, i, foBlock.content[i].inode);
    }
    fprintf(dot, "\t</table> >];\n\n");
    for (int i = 0; i < 4; i++)
    {
        char name[16] = {0};
        strcpy(name, foBlock.content[i].name);
        if (foBlock.content[i].inode != -1 && strcmp(name, ".") && strcmp(name, ".."))
        {
            treeReportInode(sb, disk, dot, foBlock.content[i].inode);
            fprintf(dot, "\tbl%i:f%i -> in%i:p;\n", index, i, foBlock.content[i].inode);
        }
    }
}



void bitmapInodeReport(PartitionNode *node, char path[])
{
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    char dir[512] = {0};
    char command[512] = {0};
    strcpy(dir, path);
    getDirectory(dir);
    strcat(command, "mkdir -p -m a=rwx ");
    strcat(command, dir);
    system(command);
    FILE *rep = fopen(path, "w");
    if (!rep)
    {
        //ERROR
        return;
    }
    SuperBlock sb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    for (int i = 0; i < sb.inodesCount; i++)
    {
        if (i != 0 && i % 20 == 0) fprintf(rep, "\n");
        char c;
        fseek(diskFile, sb.bmInodeStart + i, SEEK_SET);
        fread(&c, sizeof(char), 1, diskFile);
        fprintf(rep, "%i ", c);
    }
    fclose(diskFile);
    fclose(rep);
}
void bitmapBlockReport(PartitionNode *node, char path[])
{
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    char dir[512] = {0};
    char command[512] = {0};
    strcpy(dir, path);
    getDirectory(dir);
    strcat(command, "mkdir -p -m a=rwx ");
    strcat(command, dir);
    system(command);
    FILE *rep = fopen(path, "w");
    if (!rep)
    {
        //ERROR
        return;
    }
    SuperBlock sb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    for (int i = 0; i < sb.blocksCount; i++)
    {
        if (i != 0 && i % 20 == 0) fprintf(rep, "\n");
        char c;
        fseek(diskFile, sb.bmBlockStart + i, SEEK_SET);
        fread(&c, sizeof(char), 1, diskFile);
        fprintf(rep, "%i ", c);
    }
    fclose(diskFile);
    fclose(rep);
}

void listInodeReport(PartitionNode *node)
{
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    FILE *dot = fopen("inode_list.dot", "w");
    fprintf(dot, "digraph g {\n");
    fprintf(dot, "rankdir = LR;\n");
    fprintf(dot, "graph [fontname = \"arial\"];\n");
    fprintf(dot, "node [fontname = \"arial\"];\n");
    fprintf(dot, "edge [fontname = \"arial\"];\n");
    SuperBlock sb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    int prev = -1;
    for (int i = 0; i < sb.inodesCount; i++)
    {
        char c;
        fseek(diskFile, sb.bmInodeStart + i, SEEK_SET);
        fread(&c, sizeof(char), 1, diskFile);
        if (!c) continue;
        Inode inode;
        fseek(diskFile, getInodeAddressByIndex(&sb, i), SEEK_SET);
        fread(&inode, sizeof(Inode), 1, diskFile);
        fprintf(dot, "\tin%i [shape=none, margin=0, label =<\n", i);
        fprintf(dot, "\t\t<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n");
        fprintf(dot, "\t\t\t<tr> <td colspan = \"2\">Inode %i</td> </tr>\n", i);
        fprintf(dot, "\t\t\t<tr> <td>UID</td> <td>%i</td> </tr>\n", inode.uid);
        fprintf(dot, "\t\t\t<tr> <td>GID</td> <td>%i</td> </tr>\n", inode.gid);
        fprintf(dot, "\t\t\t<tr> <td>Size</td> <td>%i</td> </tr>\n", inode.size);
        fprintf(dot, "\t\t\t<tr> <td>aTime</td> <td>%s</td> </tr>\n", inode.aTime);
        fprintf(dot, "\t\t\t<tr> <td>cTime</td> <td>%s</td> </tr>\n", inode.cTime);
        fprintf(dot, "\t\t\t<tr> <td>mTime</td> <td>%s</td> </tr>\n", inode.mTime);
        fprintf(dot, "\t\t\t<tr> <td>Type</td> <td>%i</td> </tr>\n", inode.type);
        fprintf(dot, "\t\t\t<tr> <td>Perm</td> <td>%i</td> </tr>\n", inode.perm);
        for (int j = 0; j < 15; j++)
        {
            fprintf(dot, "\t\t\t<tr> <td>B%i</td> <td>%i</td> </tr>\n", j + 1, inode.block[j]);
        }
        fprintf(dot, "\t</table> >];\n\n");
        if (prev != -1)
        {
            fprintf(dot, "\tin%i -> in%i;\n", prev, i);
        }
        prev = i;
    }
    fprintf(dot, "}\n");
    fclose(dot);
    fclose(diskFile);
}


void listBlockReport(PartitionNode *node)
{
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    FILE *dot = fopen("block_list.dot", "w");
    fprintf(dot, "digraph g {\n");
    fprintf(dot, "rankdir = LR;\n");
    fprintf(dot, "graph [fontname = \"arial\"];\n");
    fprintf(dot, "node [fontname = \"arial\"];\n");
    fprintf(dot, "edge [fontname = \"arial\"];\n");
    SuperBlock sb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    int prev = -1;
    for (int i = 0; i < sb.inodesCount; i++)
    {
        char c;
        fseek(diskFile, sb.bmInodeStart + i, SEEK_SET);
        fread(&c, sizeof(char), 1, diskFile);
        if (!c) continue;
        Inode inode;
        fseek(diskFile, getInodeAddressByIndex(&sb, i), SEEK_SET);
        fread(&inode, sizeof(Inode), 1, diskFile);
        for (int j = 0; j < 15; j++)
        {
            if (inode.block[j] == -1) continue;
            int ad = getBlockAddressByIndex(&sb, inode.block[j]);
            if (inode.type == 1)
            {
                FileBlock flBlock;
                fseek(diskFile, ad, SEEK_SET);
                fread(&flBlock, sizeof(FileBlock), 1, diskFile);
                fprintf(dot, "\tbl%i [shape=none, margin=0, label =<\n", inode.block[j]);
                fprintf(dot, "\t\t<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n");
                fprintf(dot, "\t\t\t<tr> <td colspan = \"2\">Block %i</td> </tr>\n", inode.block[j]);
                fprintf(dot, "\t\t\t<tr> <td colspan = \"2\"> ");
                for(int k = 0; k < 64 && flBlock.content[k] != 0; k++)
                {
                    fprintf(dot, "%c", flBlock.content[k]);
                }
                fprintf(dot, " </td> </tr>\n");
                fprintf(dot, "\t</table> >];\n\n");
            }
            else
            {
                FolderBlock foBlock;
                fseek(diskFile, ad, SEEK_SET);
                fread(&foBlock, sizeof(FolderBlock), 1, diskFile);
                fprintf(dot, "\tbl%i [shape=none, margin=0, label =<\n", inode.block[j]);
                fprintf(dot, "\t\t<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n");
                fprintf(dot, "\t\t\t<tr> <td colspan = \"2\" PORT=\"p\">Block %i</td> </tr>\n", inode.block[j]);
                for (int k = 0; k < 4; k++)
                {
                    fprintf(dot, "\t\t\t<tr> <td>%s</td> <td>%i</td> </tr>\n", foBlock.content[k].name, foBlock.content[k].inode);
                }
                fprintf(dot, "\t</table> >];\n\n");
            }
            if (prev != -1)
            {
                fprintf(dot, "\tbl%i -> bl%i;\n", prev, inode.block[j]);
            }
            prev = inode.block[j];
        }
    }
    fprintf(dot, "}\n");
    fclose(dot);
    fclose(diskFile);
}




void superBlockReport(PartitionNode *node)
{
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    FILE *dot = fopen("superblock.dot", "w");
    fprintf(dot, "digraph g {\n");
    fprintf(dot, "rankdir = LR;\n");
    fprintf(dot, "graph [fontname = \"arial\"];\n");
    fprintf(dot, "node [fontname = \"arial\"];\n");
    fprintf(dot, "edge [fontname = \"arial\"];\n");
    SuperBlock sb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    fprintf(dot, "\tblsupinterb [shape=none, margin=0, label =<\n");
    fprintf(dot, "\t\t<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n");
    fprintf(dot, "\t\t\t<tr> <td colspan = \"2\">SuperBloque</td> </tr>\n");
    fprintf(dot, "\t\t\t<tr> <td>Inodes Count</td> <td>%i</td> </tr>\n", sb.inodesCount);
    fprintf(dot, "\t\t\t<tr> <td>Blocks Count</td> <td>%i</td> </tr>\n", sb.blocksCount);
    fprintf(dot, "\t\t\t<tr> <td>Free Blocks</td> <td>%i</td> </tr>\n", sb.freeBlockCount);
    fprintf(dot, "\t\t\t<tr> <td>Free Inodes</td> <td>%i</td> </tr>\n", sb.freeInodesCount);
    fprintf(dot, "\t\t\t<tr> <td>Mount Time</td> <td>%s</td> </tr>\n", sb.mountTime);
    fprintf(dot, "\t\t\t<tr> <td>Unmount Time</td> <td>%s</td> </tr>\n", sb.unmountTime);
    fprintf(dot, "\t\t\t<tr> <td>Mount Count</td> <td>%i</td> </tr>\n", sb.mountCount);
    fprintf(dot, "\t\t\t<tr> <td>Magic</td> <td>%X</td> </tr>\n", sb.magic);
    fprintf(dot, "\t\t\t<tr> <td>Inode Size</td> <td>%i</td> </tr>\n", sb.inodeSize);
    fprintf(dot, "\t\t\t<tr> <td>Block Size</td> <td>%i</td> </tr>\n", sb.blockSize);
    fprintf(dot, "\t\t\t<tr> <td>First Inode</td> <td>%i</td> </tr>\n", sb.firstInode);
    fprintf(dot, "\t\t\t<tr> <td>First Block</td> <td>%i</td> </tr>\n", sb.firstBlock);
    fprintf(dot, "\t\t\t<tr> <td>Inode BM Start</td> <td>%i</td> </tr>\n", sb.bmInodeStart);
    fprintf(dot, "\t\t\t<tr> <td>Block BM Start</td> <td>%i</td> </tr>\n", sb.bmBlockStart);
    fprintf(dot, "\t\t\t<tr> <td>Inode Start</td> <td>%i</td> </tr>\n", sb.inodeStart);
    fprintf(dot, "\t\t\t<tr> <td>Block Start</td> <td>%i</td> </tr>\n", sb.blockStart);
    fprintf(dot, "\t\t\t<tr> <td>Type</td> <td>%i</td> </tr>\n", sb.type);
    fprintf(dot, "\t</table> >];\n\n");
    fprintf(dot, "}\n");
    fclose(dot);
    fclose(diskFile);
}

void fileReport(PartitionNode *partNode, char systemPath[], char destinyPath[])
{
    FILE *diskFile = fopen(partNode->filePtr->path, "rb+");
    char dir[512] = {0};
    char command[512] = {0};
    strcpy(dir, destinyPath);
    getDirectory(dir);
    strcat(command, "mkdir -p -m a=rwx ");
    strcat(command, dir);
    system(command);
    FILE *rep = fopen(destinyPath, "w");
    if (!rep)
    {
        //ERROR
        return;
    }
    SuperBlock sb;
    fseek(diskFile, partNode->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    StringList *slist = makePathToList(systemPath);
    int index = findItemInSystem(&sb, diskFile, slist, 0);
    if(index < 0)
    {
        fclose(diskFile);
        fclose(rep);
        return;
    }
    Inode node;
    fseek(diskFile, getInodeAddressByIndex(&sb, index), SEEK_SET);
    fread(&node, sizeof(Inode), 1, diskFile);
    if (node.type != 1)
    {
        fclose(diskFile);
        fclose(rep);
        return;
    }
    for(int i = 0; i < 12; i++)
    {
        if (node.block[i] == -1) continue;
        FileBlock fb;
        int add = getBlockAddressByIndex(&sb, node.block[i]);
        fseek(diskFile, add, SEEK_SET);
        fread(&fb, sizeof(FileBlock), 1, diskFile);
        for(int j = 0; j < 64 && fb.content[j] != 0; j++)
        {
            fprintf(rep, "%c", fb.content[j]);
        }
    }
    fclose(diskFile);
    fclose(rep);
}

void journalReport(PartitionNode *node){
    FILE *diskFile = fopen(node->filePtr->path, "rb+");
    FILE *dot = fopen("journal.dot", "w");
    fprintf(dot, "digraph g {\n");
    fprintf(dot, "rankdir = LR;\n");
    fprintf(dot, "graph [fontname = \"arial\"];\n");
    fprintf(dot, "node [fontname = \"arial\"];\n");
    fprintf(dot, "edge [fontname = \"arial\"];\n");
    SuperBlock sb;
    fseek(diskFile, node->start, SEEK_SET);
    fread(&sb, sizeof(SuperBlock), 1, diskFile);
    if (sb.type != 3)
    {
        fprintf(dot, "}\n");
        fclose(dot);
        fclose(diskFile);
        return;
    }
    Journal aux;
    fseek(diskFile, node->start + sizeof(SuperBlock), SEEK_SET);
    fread(&aux, sizeof(Journal), 1, diskFile);
    int p = -1;
    while (aux.next != -1) {
        int g = aux.next;
        fseek(diskFile, aux.next, SEEK_SET);
        fread(&aux, sizeof(Journal), 1, diskFile);
        fprintf(dot, "\tjn%i [shape=none, margin=0, label =<\n", g);
        fprintf(dot, "\t\t<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n");
        fprintf(dot, "\t\t\t<tr> <td colspan = \"2\">Journal</td> </tr>\n");
        fprintf(dot, "\t\t\t<tr> <td>Type</td> <td>%i</td> </tr>\n", aux.type);
        fprintf(dot, "\t\t\t<tr> <td>Name</td> <td>%s</td> </tr>\n", aux.name);
        fprintf(dot, "\t\t\t<tr> <td>Content</td> <td>%s</td> </tr>\n", aux.content);
        fprintf(dot, "\t\t\t<tr> <td>Date</td> <td>%s</td> </tr>\n", aux.date);
        fprintf(dot, "\t\t\t<tr> <td>Owner</td> <td>%i</td> </tr>\n", aux.owner);
        fprintf(dot, "\t\t\t<tr> <td>Group</td> <td>%i</td> </tr>\n", aux.group);
        fprintf(dot, "\t\t\t<tr> <td>Perm</td> <td>%i</td> </tr>\n", aux.perm);
        fprintf(dot, "\t\t\t<tr> <td>Father</td> <td>%i</td> </tr>\n", aux.father);
        fprintf(dot, "\t\t\t<tr> <td>Next</td> <td>%i</td> </tr>\n", aux.next);
        fprintf(dot, "\t</table> >];\n\n");
        if (p != -1)
        {
            fprintf(dot, "\tjn%i -> jn%i;\n\n", p, g);
        }
        p = g;
    }
    fprintf(dot, "}\n");
    fclose(dot);
    fclose(diskFile);
}
