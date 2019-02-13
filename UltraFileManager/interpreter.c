#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*STRUCT SPACE OwO*/

struct parameterDuo
{
    char parameterName[64];
    char parameterValue[512];
    struct parameterDuo *nextValue;
};

typedef struct parameterDuo ParameterDuo;

struct parameterList
{
    char commandName[64];
    ParameterDuo *firstValue;
};

typedef struct parameterList ParameterList;

/*END OF STRUCT SPACE O~O*/


//PROTOTYPES


void analyseString(char instr[1024]);
void deleteParameterList(ParameterList *list);
void deleteParameter(ParameterDuo *par);
ParameterList *getList(char instr[1024]);
ParameterDuo *newParameter(char name[64], char value[512]);
ParameterList *newParameterList(char name[64]);
int addNewParameter(ParameterList *list, ParameterDuo *parameter);

//END OF PROTOTYPES

void analyseString(char instr[1024])
{
    ParameterList *l = getList(instr);
    printf("YESSSSSSSS\n");
    deleteParameterList(l);
    printf("NOOOOOOOOOO\n");
}


ParameterList *getList(char instr[1024])
{
    char instruction[1024] = "";
    char instructionName[64] = "";
    strcat(instruction, instr);
    int stPointer = 0;
    if (instruction[0] == '#') return newParameterList(instructionName);
    while (instruction[stPointer] != ' ' && instruction[stPointer] != '\0')
    {
        char auxiliarCharacter[2] = {instruction[stPointer], '\0'};
        strcat(instructionName, auxiliarCharacter);
        stPointer++;
    }
    ParameterList *list = newParameterList(instructionName);
    while (instruction[stPointer] != '\0')
    {
        char paramName[64] = "";
        char paramValue[512] = "";
        while(instruction[stPointer] != '-' && instruction [stPointer] != '\0')
            stPointer++;
        stPointer++;
        while (instruction[stPointer] != '~' && instruction [stPointer] != '\0')
        {
            char auxiliarCharacter[2] = {instruction[stPointer], '\0'};
            strcat(paramName, auxiliarCharacter);
            stPointer++;
        }
        stPointer++;
        while (instruction[stPointer] != '~' && instruction [stPointer] != '\0')
            stPointer++;
        stPointer++;
        while (instruction[stPointer] != '-' && instruction [stPointer] != '\0')
        {
            char auxiliarCharacter[2] = {instruction[stPointer], '\0'};
            strcat(paramValue, auxiliarCharacter);
            stPointer++;
        }
        addNewParameter(list, newParameter(paramName, paramValue));
    }
    return list;
}

ParameterList *newParameterList(char name[64])
{
    ParameterList *newList = (ParameterList*) malloc(sizeof(ParameterList));
    strcpy(newList->commandName, name);
    return newList;
}



ParameterDuo *newParameter(char name[64], char value[512])
{
    ParameterDuo *newPar = (ParameterDuo*) malloc(sizeof(ParameterDuo));
    strcpy(newPar->parameterName, name);
    strcpy(newPar->parameterValue, value);
    newPar->nextValue = NULL;
    return newPar;
}

int addNewParameter(ParameterList *list, ParameterDuo *parameter)
{
    if (list->firstValue == NULL)
    {
        list->firstValue = parameter;
        return 0;
    }
    ParameterDuo *aux = list->firstValue;
    while(aux->nextValue != NULL) aux = aux->nextValue;
    aux->nextValue = parameter;
    return 0;
}

void deleteParameterList(ParameterList *list)
{
    deleteParameter(list->firstValue);
    free(list);
}

void deleteParameter(ParameterDuo *par)
{
    if (par == NULL) return;
    deleteParameter(par->nextValue);
    free(par);
}
