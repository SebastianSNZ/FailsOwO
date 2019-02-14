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

struct function
{
    char title[512];
    char size[512];
    char unit[512];
    char fit[512];
    char path[512];
    char type[512];
    char delete[512];
    char name[512];
    char add[512];
    char id[512];
    int errorParameter;
};

typedef struct function Function;

/*END OF STRUCT SPACE O~O*/


//PROTOTYPES


void analyseString(char instr[1024]);
void deleteParameterList(ParameterList *list);
void deleteParameter(ParameterDuo *par);
ParameterList *getList(char instr[1024]);
ParameterDuo *newParameter(char name[64], char value[512]);
ParameterList *newParameterList(char name[64]);
int addNewParameter(ParameterList *list, ParameterDuo *parameter);
void deleteWhiteSpaces(char text[]);
void removeStringLiteral(char text[]);
Function getNewFunction(ParameterList *list);
Function getFunction();

//END OF PROTOTYPES

void analyseString(char instr[1024])
{
    ParameterList *l = getList(instr);
    printf("YESSSSSSSS\n");
    Function f = getNewFunction(l);
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
    deleteWhiteSpaces(newPar->parameterValue);
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

Function getNewFunction(ParameterList *list)
{
    Function func = getFunction();
    strcpy(func.title, list->commandName);
    ParameterDuo *aux = list->firstValue;
    while(aux != NULL)
    {
        if (!strncasecmp(aux->parameterName, "size", 4)) strcpy(func.size, aux->parameterValue);
        else if (!strncasecmp(aux->parameterName, "fit", 3)) strcpy(func.fit, aux->parameterValue);
        else if (!strncasecmp(aux->parameterName, "unit", 4)) strcpy(func.unit, aux->parameterValue);
        else if (!strncasecmp(aux->parameterName, "path", 4)) strcpy(func.path, aux->parameterValue);
        else if (!strncasecmp(aux->parameterName, "type", 4)) strcpy(func.type, aux->parameterValue);
        else if (!strncasecmp(aux->parameterName, "delete", 6)) strcpy(func.delete, aux->parameterValue);
        else if (!strncasecmp(aux->parameterName, "name", 4)) strcpy(func.name, aux->parameterValue);
        else if (!strncasecmp(aux->parameterName, "add", 3)) strcpy(func.add, aux->parameterValue);
        else if (!strncasecmp(aux->parameterName, "id", 2)) strcpy(func.id, aux->parameterValue);
        else
        {
            func.errorParameter = 1;
            printf("El parametro %s no existe.\n", aux->parameterName);
        }
        aux = aux->nextValue;
    }
    return func;
}

Function getFunction()
{
    Function fun;
    strcpy(fun.title, "");
    strcpy(fun.size, "");
    strcpy(fun.fit, "");
    strcpy(fun.unit, "");
    strcpy(fun.path, "");
    strcpy(fun.type, "");
    strcpy(fun.delete, "");
    strcpy(fun.name, "");
    strcpy(fun.add, "");
    strcpy(fun.id, "");
    fun.errorParameter = 0;
    return fun;
}


void deleteWhiteSpaces(char text[])
{
    removeStringLiteral(text);
    int i;
    for(i = 0; i < 512; i++)
    {
        if (text[i] == '\0') break;
    }
    i--;
    for (; i > 0; i--)
    {
        if (text[i] == ' ' || text[i] == '\"' ) text[i] = '\0';
        else  break;
    }
}

void removeStringLiteral(char text[])
{
    if (text[0] != '\"') return;
    char newText[512];
    int i;
    for(i = 1; i < 512; i++)
    {
        if (text[i] == '\0') break;
        newText[i - 1] = text[i];
    }
    newText[i] = '\0';
    strcpy(text, newText);
}
