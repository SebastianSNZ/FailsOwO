#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
    struct PARTITION_NODE *next;
};

typedef struct PARTITION_NODE PartitionNode;

//FIN STRUCTS

//VARIABLES GLOBALES

FileDiskNode *diskList = NULL;
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
PartitionNode *newPartitionNode(char name[], char id[], FileDiskNode *ptr);
void addNewPartitionNode(PartitionNode **aux, char name[], char id[], FileDiskNode *ptr);
PartitionNode *getPartitionNode(PartitionNode *aux, char id[]);
void mountPartition(char path[], char name[]);
void deletePartitionNode(PartitionNode **aux, char id[]);
void unMountPartition(char id[]);
void getDirectory(char text[]);
void report(char id[], char path[], char name[]);
void reportDisk(char filePath[], char destiny[]);
void reportMbr(char filePath[], char destiny[]);

//FIN PROTOTIPOS




void createNewDisk(int size, char fit, char unit, char path[512])
{
    if (!paramatersDiskCreation(size, fit, unit)) return;
    if (fit == '\0') fit = 'F';
    if (unit == '0') unit = 'M';
    char dir[512] = {0};
    char command[512];
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


PartitionNode *newPartitionNode(char name[], char id[], FileDiskNode *ptr)
{
    PartitionNode *pNode = (PartitionNode *) malloc(sizeof(PartitionNode));
    strcpy(pNode->id, id);
    strcpy(pNode->name, name);
    pNode->filePtr = ptr;
    pNode->next = NULL;
    return pNode;
}


void addNewPartitionNode(PartitionNode **aux, char name[], char id[], FileDiskNode *ptr)
{
    if ((*aux) == NULL)
    {
        (*aux) = newPartitionNode(name, id, ptr);
        return;
    }
    PartitionNode *piv = *aux;
    while (piv->next != NULL) piv = piv->next;
    piv->next = newPartitionNode(name, id, ptr);
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
    if (address < 0 && logicalAddress < 0)
    {
        printf("Error, %s no posee una particion %s\n", path, name);
        fclose(diskFile);
        return;
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
    addNewPartitionNode(&partList, name, id, dNode);
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

void report(char id[], char path[], char name[])
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
    char diskPath[512];
    strcpy(diskPath, result->filePtr->path);
    if (!strncasecmp(name, "disk", 4))
    {
        reportDisk(diskPath, path);
    }
    else if (!strncasecmp(name, "mbr", 3))
    {
        reportMbr(diskPath, path);
    }
}

void reportDisk(char filePath[], char destiny[])
{
    char dir[512] = {0};
    char command[512];
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
