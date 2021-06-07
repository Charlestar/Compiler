#ifndef MIPS_H
#define MIPS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intercode.h"

extern InterCode* interhead;

void printMIPS(FILE* stream);

#endif