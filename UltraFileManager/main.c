/* Example using strcat by TechOnTheNet.com */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "interpreter.c"

int main()
{
    srand(time(NULL));
    analyseString("mkdisk -size~:~1 -unit~:~M -path~:~/home/archivos/fase1/Disco1.disk -fit~:~WF");
    analyseString("fdisk -type~:~P -unit~:~K -name~:~Part1 -size~:~300 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    analyseString("fdisk -type~:~E -unit~:~K -name~:~Part2 -size~:~200 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    analyseString("fdisk -type~:~P -unit~:~K -name~:~Part3 -size~:~400 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    analyseString("fdisk -type~:~L -unit~:~K -name~:~LogicalPart4 -size~:~30 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    analyseString("fdisk -type~:~L -unit~:~K -name~:~LogicalPart5 -size~:~35 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    analyseString("fdisk -type~:~L -unit~:~K -name~:~LogicalPart6 -size~:~40 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    //analyseString("fdisk -delete~:~full -name~:~LogicalPart5 -path~:~/home/archivos/fase1/Disco1.disk");
    analyseString("fdisk -add~:~-20 -unit~:~K -path~:~/home/archivos/fase1/Disco1.disk -name~:~LogicalPart4");
    analyseString("fdisk -type~:~L -unit~:~K -name~:~LogicalPart7 -size~:~15 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    return 0;

}

