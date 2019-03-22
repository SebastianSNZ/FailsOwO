#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "interpreter.c"

int main()
{
    printf("Ultra File Manager - Sebastian Sanchez - 201603014\n");
    //int exit = 1;
    //char val[512] = {0};
    currentUser.nullUser = 1;
    currentUser.pNode = NULL;
    char val[512] = "exec -path~:~ /home/sebastian/Documentos/Cal.sh";
    analyseString(val);
    /*while (exit)
    {
        gets(val);
        if (!strncasecmp(val, "exit", 4))
        {
            exit = 0;
            continue;
        }
        analyseString(val);
    }*/

}

