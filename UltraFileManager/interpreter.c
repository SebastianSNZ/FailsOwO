#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "diskmanagement.c"

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
    char fs[512];
    char file[512];
    int p;
    char usr[512];
    char pwd[512];
    char grp[512];
    char cont[512];
    int errorParameter;
};

typedef struct function Function;


struct TOKEN {
    char lex[512];
    char type;
};

typedef struct TOKEN Token;
/*END OF STRUCT SPACE O~O*/


//PROTOTYPES


void analyseString(char instr[1024]);
void deleteParameterList(ParameterList *list);
void deleteParameter(ParameterDuo *par);
void partitonManagerFunction(Function func);
ParameterDuo *newParameter(char name[], char value[]);
ParameterList *newParameterList(char name[]);
int addNewParameter(ParameterList *list, ParameterDuo *parameter);
void deleteWhiteSpaces(char text[]);
void removeStringLiteral(char text[]);
Function getNewFunction(ParameterList *list);
Function getFunction();
void execFunction(Function func);
int getInt(char text[]);
void execFile(char path[]);
int isEmptyLine(char text[]);
Token getNextToken(char text[]);
ParameterList *syntaxViewer(char instr[]);
int currentPosition = 0;
int currentStatus;
void addCharacter(char text[], char c);
//END OF PROTOTYPES

void analyseString(char instr[1024])
{
    ParameterList *l = syntaxViewer(instr);
    Function f = getNewFunction(l);
    deleteParameterList(l);
    execFunction(f);
}

void execFunction(Function func)
{
    if (!strncasecmp(func.title, "mkdisk", 6))
    {
        createNewDisk(getInt(func.size), func.fit[0], func.unit[0], func.path);
        return;
    }
    if (!strncasecmp(func.title, "rmdisk", 6))
    {
        deleteDisk(func.path);
        return;
    }
    if (!strncasecmp(func.title, "fdisk", 6))
    {
        partitonManagerFunction(func);
        return;
    }
    if (!strncasecmp(func.title, "mount", 5))
    {
        mountPartition(func.path, func.name);
        return;
    }
    if (!strncasecmp(func.title, "unmount", 7))
    {
        unMountPartition(func.id);
        return;
    }
    if (!strncasecmp(func.title, "rep", 3))
    {
        report(func.id, func.path, func.name, func.file);
        return;
    }
    if (!strncasecmp(func.title, "exec", 4))
    {
        execFile(func.path);
        return;
    }
    if (!strcasecmp(func.title, "mkfs"))
    {
        makeFileSystem(func.id, func.type, func.fs);
        return;
    }
    if (!strcasecmp(func.title, "pause"))
    {
        pauseThread();
        return;
    }
    if (!strcasecmp(func.title, "login")){
        loginUser(func.usr, func.pwd, func.id);
        return;
    }
    if (!strcasecmp(func.title, "logout")){
        logout();
        return;
    }
    if (!strcasecmp(func.title, "mkusr")) {
        makeUser(func.grp, func.usr, func.pwd, currentUser.pNode);
        return;
    }
    if (!strcasecmp(func.title, "mkgrp")) {
        makeGroup(func.name, currentUser.pNode);
        return;
    }
    if (!strcasecmp(func.title, "mkfile")) {
        makeNewFile(func.path, func.cont, getInt(func.size), func.p, 664, currentUser.pNode);
        return;
    }
    if (!strcasecmp(func.title, "mkdir")) {
        makeNewDirectory(func.path, func.p, 664, currentUser.pNode);
        return;
    }
    if (!strcasecmp(func.title, "loss")) {
        gettingLossed(func.id);
        return;
    }
    if (!strcasecmp(func.title, "recovery")) {
        recoverDisk(func.id);
        return;
    }
    if (!strcasecmp(func.title, "convert")) {
        convertEXT(func.id);
        return;
    }
    if (func.title[0] == '\0')
    {
        return;
    }
}

void partitonManagerFunction(Function func)
{
    if (func.add[0] != '\0')
    {
        addSizePartition(getInt(func.add), func.unit[0], func.path, func.name);
        return;
    }
    if (func.delete[0] != '\0')
    {
        deletePartition(func.path, func.name, func.delete);
        return;
    }
    addPartition(getInt(func.size), func.unit[0], func.type[0], func.fit[0], func.name, func.path);
}

int getInt(char text[]) {
    int negative = 1;
    int val = 0;
    int i = 0;
    if (text[0] == '-')
    {
        i++;
        negative = -1;
    }
    for(; text[i] != '\0'; i++)
    {
        if (text[i] < '0' || text[i] > '9') return -1;
        val = val * 10;
        val = val + (text[i] - 48);
    }
    return val*negative;
}

ParameterList *newParameterList(char name[])
{
    ParameterList *newList = (ParameterList*) malloc(sizeof(ParameterList));
    newList->firstValue = NULL;
    strcpy(newList->commandName, name);
    return newList;
}



ParameterDuo *newParameter(char name[], char value[])
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
        if (!strcasecmp(aux->parameterName, "size")) strcpy(func.size, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "fit")) strcpy(func.fit, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "unit")) strcpy(func.unit, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "path")) strcpy(func.path, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "type")) strcpy(func.type, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "delete")) strcpy(func.delete, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "name")) strcpy(func.name, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "add")) strcpy(func.add, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "id")) strcpy(func.id, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "fs")) strcpy(func.fs, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "p")) func.p = 1;
        else if (!strcasecmp(aux->parameterName, "usr")) strcpy(func.usr, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "pwd")) strcpy(func.pwd, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "grp")) strcpy(func.grp, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "cont")) strcpy(func.cont, aux->parameterValue);
        else if (!strcasecmp(aux->parameterName, "file")) strcpy(func.file, aux->parameterValue);
        else if ((aux->parameterName)[0] == '#');
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
    strcpy(fun.fs, "");
    strcpy(fun.file, "");
    strcpy(fun.usr, "");
    strcpy(fun.pwd, "");
    strcpy(fun.grp, "");
    strcpy(fun.cont, "");
    fun.p = 0;
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
        if (text[i] == '\"' || text[i] == ' ' || text[i] == '\n' || text[i] == '\r' || text[i] == '\t')
            text[i] = '\0';
        else  break;
    }
}

void removeStringLiteral(char text[])
{
    if (text[0] != '\"' && text[0] != ' ' && text[0] != '\n' && text[0] != '\r' && text[0] != '\t') return;
    char newText[512] = {0};
    int i;
    int j;
    for(j = 0; j < 512; j++)
    {
        if (text[j] != '\"' && text[j] != ' ' && text[j] != '\n' && text[j] != '\r' && text[j] != '\t') break;
    }
    for(i = j; i < 512; i++)
    {
        if (text[i] == '\0') break;
        newText[i - j] = text[i];
    }
    newText[i] = '\0';
    strcpy(text, newText);
}

void execFile(char path[])
{
    FILE *readFile = fopen(path, "r");
    if (!readFile)
    {
        printf("Error, imposible abrir archivo.\n");
        return;
    }
    char line[512] = {0};
    while(fgets(line, 512, readFile))
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r' ||line[0] == ' ')
        {
            memset(&line[0], 0, sizeof(line));
            continue;
        }
        printf(">>%s\n", line);
        analyseString(line);
        memset(&line[0], 0, sizeof(line));
    }
    fclose(readFile);
}


int isEmptyLine(char text[])
{
    deleteWhiteSpaces(text);
    if (text[0] == '\0') return 1;
    return 0;
}


Token getNextToken(char text[])
{
    char lex[512] = {0};
    Token tok;
    strcpy(tok.lex, "");
    tok.type = 'N';

    while (text[currentPosition] != '\0' || currentPosition > 512)
    {
        switch (currentStatus)
        {
        case 0:
            if(isalpha(text[currentPosition]) || isdigit(text[currentPosition])
                    || text[currentPosition] == '/' || text[currentPosition] == '.'
                    || text[currentPosition] == '_')
            {
                currentStatus = 2;
                addCharacter(lex, text[currentPosition]);
                currentPosition++;
            }
            else if (text[currentPosition] == '-')
            {
                currentStatus = 1;
                currentPosition++;
            }
            else if (text[currentPosition] == '~')
            {
                currentStatus = 4;
                currentPosition++;
            }
            else if (text[currentPosition] == '\"')
            {
                currentStatus = 3;
                currentPosition++;
            }
            else if (text[currentPosition] == '#')
            {
                return tok;
            }
            else
            {
                currentPosition++;
            }
            break;
        case 1:
            if (isalpha(text[currentPosition]))
            {
                currentStatus = 52;
                addCharacter(lex, text[currentPosition]);
                currentPosition++;
            }
            else if (isdigit(text[currentPosition]))
            {
                currentStatus = 50;
                addCharacter(lex, '-');
                addCharacter(lex, text[currentPosition]);
                currentPosition++;
            }
            else
            {
                currentStatus = 0;
                tok.type = 'N';
                return tok;
            }
            break;
        case 2:
            if (isalpha(text[currentPosition]) || isdigit(text[currentPosition])
                    || text[currentPosition] == '/' || text[currentPosition] == '.'
                    || text[currentPosition] == '_')
            {
                currentStatus = 2;
                addCharacter(lex, text[currentPosition]);
                currentPosition++;
            }
            else
            {
                currentStatus = 0;
                tok.type = 'V';
                strcpy(tok.lex, lex);
                return tok;
            }
            break;
        case 3:
            if (text[currentPosition] == '\"')
            {
                currentStatus = 0;
                currentPosition++;
                tok.type = 'V';
                strcpy(tok.lex, lex);
                return tok;
            }
            else
            {
                currentStatus = 3;
                addCharacter(lex, text[currentPosition]);
                currentPosition++;
            }
            break;
        case 4:
            if (text[currentPosition] == '~' || text[currentPosition] == ':')
            {
                currentStatus = 4;
                currentPosition++;
            }
            else
            {
                currentStatus = 0;
                tok.type = 'S';
                strcpy(tok.lex, lex);
                return tok;
            }
            break;
        case 50:
            if (isdigit(text[currentPosition]))
            {
                currentStatus = 50;
                addCharacter(lex, text[currentPosition]);
                currentPosition++;
            }
            else
            {
                currentStatus = 0;
                tok.type = 'V';
                strcpy(tok.lex, lex);
                return tok;
            }
            break;
        case 52:
            if (isalpha(text[currentPosition]) || isdigit(text[currentPosition]))
            {
                currentStatus = 52;
                addCharacter(lex, text[currentPosition]);
                currentPosition++;
            }
            else
            {
                currentStatus = 0;
                tok.type = 'P';
                strcpy(tok.lex, lex);
                return tok;
            }
            break;
        }
    }
    return tok;
}

void addCharacter(char text[], char c)
{
    char aux[2] = {c, '\0'};
    strcat(text, aux);
}

ParameterList *syntaxViewer(char instr[])
{
    addCharacter(instr, '#');
    currentStatus = 0;
    currentPosition = 0;
    Token next = getNextToken(instr);
    ParameterList *param = newParameterList(next.lex);
    next = getNextToken(instr);
    while (next.type != 'N')
    {
        char par[64] = {0};
        strcpy(par, next.lex);
        next = getNextToken(instr);
        if (next.type == 'S')
        {
            next = getNextToken(instr);
            addNewParameter(param, newParameter(par, next.lex));
            next = getNextToken(instr);
        }
        else
        {
            addNewParameter(param, newParameter(par, "1"));
        }
    }
    return param;
}










