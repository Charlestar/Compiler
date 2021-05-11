#ifndef IR_H
#define IR_H

struct Operand_ {
    enum { VARIABLE, CONSTANT, ADDRESS } kind;
    union {
        int var_no;
        int value;
    } u;
};
typedef struct Operand_* Operand;

typedef struct InterCode_ {
    enum { ASSIGN, ADD, SUB, MUL } kind;
    union {
        struct {
            Operand right, left;
        } assign;
        struct {
            Operand result, op1, op2;
        } binop;
    } u;
} InterCode;

struct InterCodes {
    InterCode code;
    struct InterCode_ *prev, *next;
};

#endif