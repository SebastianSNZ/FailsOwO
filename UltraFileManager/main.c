/* Example using strcat by TechOnTheNet.com */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "interpreter.c"

int main()
{
    srand(time(NULL));
    analyseString("mkdisk -size~:~1 -unit~:~M -path~:~/home/archivos/fase1/Disco1.disk");
    analyseString("fdisk -type~:~P -unit~:~K -name~:~Part1 -size~:~300 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    analyseString("fdisk -type~:~P -unit~:~K -name~:~Part2 -size~:~200 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    analyseString("fdisk -type~:~P -unit~:~K -name~:~Part3 -size~:~400 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    analyseString("fdisk -type~:~P -unit~:~K -name~:~Part4 -size~:~50 -path~:~/home/archivos/fase1/Disco1.disk -fit~:~BF");
    return 0;

}

