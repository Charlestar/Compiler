#ifndef IR_H
#define IR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "tree.h"

#define DEBUG FALSE
#define MAX_ARR_DIM 1000

extern Node *root;
extern int depth;

enum { EQ = 0, LT, GT, NE, GE, LE };
#define revRel(x) ((x + 3) % 6)

typedef struct Operand_ {
    enum { OP_VARIABLE, OP_INT, OP_FLOAT, OP_ADDRESS, OP_POINTER, OP_TEMP_VAR, OP_LABEL, OP_FUNCTION, OP_RELOP } kind;
    union {
        int id;
        int i;
        float f;
        char *name;
        int rel;
    } u;
} Operand;

typedef struct InterCode_ {
    enum {
        CODE_ASSIGN,
        CODE_ADD,
        CODE_SUB,
        CODE_MUL,
        CODE_DIV,
        CODE_IFGOTO,
        CODE_LABEL,
        CODE_FUNCTION,
        CODE_GOTO,
        CODE_RETURN,
        CODE_DECOP,
        CODE_ARG,
        CODE_PARAM,
        CODE_READ,
        CODE_WRITE
    } kind;
    union {
        // 赋值 = =& =* *= =CALL
        struct {
            struct Operand_ *left, *right;
        } assign;
        // 标记 LABEL, FUNCTION, GOTO, RETURN, ARG, PARAM, READ, WRITE
        struct {
            struct Openrand_ *op;
        } monop;
        // 双目运算符号 + - * /
        struct {
            struct Operand_ *result, *op1, *op2;
        } binop;
        // ifgoto
        struct {
            struct Operand_ *dest, *op1, *op2, *relop;
        } ifgoto;
        // DEC 申请内存空间
        struct {
            struct Operand_ *op, *size;
        } decop;
    } u;
    struct InterCode_ *prev, *next;
} InterCode;

void printInterCode();

#endif