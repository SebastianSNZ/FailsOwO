/* Example using strcat by TechOnTheNet.com */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "interpreter.c"

int main()
{
    srand(time(NULL));
    analyseString("mkdisk -size~:~50 -unit~:~M -path~:~/home/archivos/fase1/Disco1.disk");
    return 0;
}

