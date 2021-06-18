#ifndef MIPS_H
#define MIPS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intercode.h"

extern InterCode* interhead;

typedef struct Var_ {
    int mem;
    int reg;
    enum { VAR, TEMP } kind;
    struct Var_* next;
} Var;

typedef struct Reg_ {
    int used;
    struct Var_* var;
    struct Var_* stored;
} Reg;

typedef struct Log_ {
    int offset;
    struct Log_* next;
} Log;

void printMIPS(FILE* stream);

#endif