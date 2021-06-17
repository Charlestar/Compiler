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
} Var;

void printMIPS(FILE* stream);

#endif