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
    char name[];
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



//FIN STRUCTS


//PROTOTIPOS

void createNewDisk(int size, char fit, char unit, char path[]);
int paramatersDiskCreation(int size, char fit, char unit);
MasterBootRecord getMBR(int size, char fit, char unit);

//FIN PROTOTIPOS




void createNewDisk(int size, char fit, char unit, char path[])
{
    if (!paramatersDiskCreation(size, fit, unit)) return;
    if (fit == '\0') fit = 'F';
    if (unit == '0') unit = 'M';
    MasterBootRecord mbr = getMBR(size, fit, unit);
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


MasterBootRecord getMBR(int size, char fit, char unit)
{
    int currentSize = 0;
    if (unit == 'k' || unit == 'K') currentSize = size * 1024;
    else currentSize = size * 1024 * 1024;
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

