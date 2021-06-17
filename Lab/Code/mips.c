#include "mips.h"

#include <assert.h>

#define DEBUG FALSE
// WHITE_SPACE
#define WS "  "
#define MAX_LIST 100

static FILE* dest_stream = NULL;
// 避免冲突还是用两个变量表示
static int arg_num = 0;
static int param_num = 0;

static Var var_list[MAX_LIST];
static Var tem_list[MAX_LIST];
static int reg_list[32];
static int lru[32];

static int sp_offset = 0;

static void initMIPS();
static void initCode();
static void translateCode(InterCode* code);
static int getReg(Operand* op);
static void releaseReg(Operand* op);
static int findEmptyReg();
static Var* findVar(Operand* op);
static void moveSP(int dis);

void printMIPS(FILE* stream)
{
    if (TRUE == DEBUG) printf("Printing MIPS ...");
    dest_stream = stream;
    initCode();
    initMIPS();

    InterCode* prt = interhead;
    while (prt->next != NULL) {
        prt = prt->next;
        if (CODE_ARG == prt->kind) {
            InterCode* arg_end = prt;
            while (CODE_ARG == prt->next->kind) prt = prt->next;
            InterCode* arg_start = prt;
            while (arg_start != arg_end) {
                translateCode(arg_start);
                arg_start = arg_start->prev;
            }
            translateCode(arg_end);
        } else {
            translateCode(prt);
        }
    }
}

void initCode()
{
    fprintf(dest_stream, ".data\n");
    fprintf(dest_stream, "_prompt: .asciiz \"Enter an integer:\"\n");
    fprintf(dest_stream, "_ret: .asciiz \"\\n\"\n");
    fprintf(dest_stream, ".globl main\n");
    fprintf(dest_stream, ".text\n");
    // read
    fprintf(dest_stream, "\nread:\n");
    fprintf(dest_stream, "%sli $v0, 4\n", WS);
    fprintf(dest_stream, "%sla $a0, _prompt\n", WS);
    fprintf(dest_stream, "%ssyscall\n", WS);
    fprintf(dest_stream, "%sli $v0, 5\n", WS);
    fprintf(dest_stream, "%ssyscall\n", WS);
    fprintf(dest_stream, "%sjr $ra\n", WS);
    // write
    fprintf(dest_stream, "\nwrite:\n");
    fprintf(dest_stream, "%sli $v0, 1\n", WS);
    fprintf(dest_stream, "%ssyscall\n", WS);
    fprintf(dest_stream, "%sli $v0, 4\n", WS);
    fprintf(dest_stream, "%sla $a0, _ret\n", WS);
    fprintf(dest_stream, "%ssyscall\n", WS);
    fprintf(dest_stream, "%smove $v0, $0\n", WS);
    fprintf(dest_stream, "%sjr $ra\n", WS);
}

void initMIPS()
{
    memset(var_list, 0, sizeof(var_list));
    memset(tem_list, 0, sizeof(tem_list));
    memset(reg_list, FALSE, sizeof(reg_list));
}

void translateCode(InterCode* code)
{
    if (CODE_LABEL == code->kind)
        fprintf(dest_stream, "L%d:\n", code->u.monop.op->u.i);
    else if (CODE_GOTO == code->kind)
        fprintf(dest_stream, "%sj L%d\n", WS, code->u.monop.op->u.i);
    else if (CODE_FUNCTION == code->kind)
        fprintf(dest_stream, "\n%s:\n", code->u.monop.op->u.name);
    else if (CODE_RETURN == code->kind) {
        Operand* rt = code->u.monop.op;
        int reg_rt = getReg(rt);
        fprintf(dest_stream, "%smove $v0, $%d\n", WS, reg_rt);
        fprintf(dest_stream, "%sjr $ra\n", WS);
        reg_list[2] = TRUE;
        param_num = 0;
    } else if (CODE_ASSIGN == code->kind) {
        Operand* l = code->u.assign.left;
        Operand* r = code->u.assign.right;
        int reg_l = getReg(l);
        if (OP_FUNCTION == r->kind) {
            moveSP(-4);
            fprintf(dest_stream, "%ssw $ra, 0($sp)\n", WS);

            fprintf(dest_stream, "%sjal %s\n", WS, r->u.name);

            fprintf(dest_stream, "%slw $ra, 0($sp)\n", WS);
            moveSP(arg_num < 4 ? 4 : 4 * (arg_num - 3));

            fprintf(dest_stream, "%smove $%d, $v0\n", WS, reg_l);
            arg_num = 0;
            reg_list[2] = FALSE;
        } else {
            if (FALSE == l->isAddress && FALSE == r->isAddress) {
                // 不考虑OP_FLOAT
                if (OP_INT == r->kind) {
                    fprintf(dest_stream, "%sli $%d, %d\n", WS, reg_l, r->u.i);
                } else {
                    int reg_r = getReg(r);
                    fprintf(dest_stream, "%smove $%d, $%d\n", WS, reg_l, reg_r);
                    releaseReg(r);
                }
            } else {
                int reg_r = getReg(r);
                if (TRUE == l->isAddress) {
                    fprintf(dest_stream, "%ssw $%d, 0($%d)\n", WS, reg_r, reg_l);
                } else if (TRUE == r->isAddress) {
                    fprintf(dest_stream, "%slw $%d, 0($%d)\n", WS, reg_l, reg_r);
                }
                releaseReg(r);
            }
        }
        releaseReg(l);
    } else if (CODE_ADD == code->kind || CODE_SUB == code->kind) {
        Operand* op1 = code->u.binop.op1;
        Operand* op2 = code->u.binop.op2;
        Operand* result = code->u.binop.result;
        int reg_op1 = getReg(op1);
        int reg_result = getReg(result);
        // 不考虑OP_FLOAT
        if (OP_INT == op2->kind) {
            int constant = code->kind == CODE_ADD ? op2->u.i : -op2->u.i;
            fprintf(dest_stream, "%saddi $%d, $%d, %d\n", WS, reg_result, reg_op1, constant);
        } else {
            int reg_op2 = getReg(op2);
            if (CODE_ADD == code->kind) {
                fprintf(dest_stream, "%sadd $%d, $%d, $%d\n", WS, reg_result, reg_op1, reg_op2);
            } else if (CODE_SUB == code->kind) {
                fprintf(dest_stream, "%ssub $%d, $%d, $%d\n", WS, reg_result, reg_op1, reg_op2);
            }
            releaseReg(op2);
        }
        releaseReg(op1);
        releaseReg(result);
    } else if (CODE_MUL == code->kind || CODE_DIV == code->kind) {
        Operand* op1 = code->u.binop.op1;
        Operand* op2 = code->u.binop.op2;
        Operand* result = code->u.binop.result;
        int reg_op1 = getReg(op1);
        int reg_op2 = getReg(op2);
        int reg_result = getReg(result);
        if (CODE_MUL == code->kind) {
            fprintf(dest_stream, "%smul $%d, $%d, $%d\n", WS, reg_result, reg_op1, reg_op2);
        } else if (CODE_DIV == code->kind) {
            fprintf(dest_stream, "%sdiv $%d, $%d\n", WS, reg_op1, reg_op2);
            fprintf(dest_stream, "%smflo $%d\n", WS, reg_result);
        }
        releaseReg(op1);
        releaseReg(op2);
        releaseReg(result);
    } else if (CODE_IFGOTO == code->kind) {
        Operand* dest = code->u.ifgoto.dest;
        Operand* op1 = code->u.ifgoto.op1;
        Operand* op2 = code->u.ifgoto.op2;
        Operand* relop = code->u.ifgoto.relop;
        int reg_op1 = getReg(op1);
        int reg_op2 = getReg(op2);
        if (EQ == relop->u.i) {
            fprintf(dest_stream, "%sbeq $%d, $%d, L%d\n", WS, reg_op1, reg_op2, dest->u.i);
        } else if (LT == relop->u.i) {
            fprintf(dest_stream, "%sblt $%d, $%d, L%d\n", WS, reg_op1, reg_op2, dest->u.i);
        } else if (GT == relop->u.i) {
            fprintf(dest_stream, "%sbgt $%d, $%d, L%d\n", WS, reg_op1, reg_op2, dest->u.i);
        } else if (NE == relop->u.i) {
            fprintf(dest_stream, "%sbne $%d, $%d, L%d\n", WS, reg_op1, reg_op2, dest->u.i);
        } else if (GE == relop->u.i) {
            fprintf(dest_stream, "%sbge $%d, $%d, L%d\n", WS, reg_op1, reg_op2, dest->u.i);
        } else if (LE == relop->u.i) {
            fprintf(dest_stream, "%sble $%d, $%d, L%d\n", WS, reg_op1, reg_op2, dest->u.i);
        }
        releaseReg(op1);
        releaseReg(op2);
    } else if (CODE_READ == code->kind) {
        Operand* rd = code->u.monop.op;
        int reg_rd = getReg(rd);
        moveSP(-4);
        fprintf(dest_stream, "%ssw $ra, 0($sp)\n", WS);

        fprintf(dest_stream, "%sjal read\n", WS);

        fprintf(dest_stream, "%slw $ra, 0($sp)\n", WS);
        moveSP(4);

        fprintf(dest_stream, "%smove $%d, $v0\n", WS, reg_rd);
        arg_num = 0;
        releaseReg(rd);
        reg_list[2] = FALSE;
    } else if (CODE_WRITE == code->kind) {
        Operand* wt = code->u.monop.op;
        int reg_wt = getReg(wt);
        fprintf(dest_stream, "%smove $a0, $%d\n", WS, reg_wt);

        moveSP(-4);
        fprintf(dest_stream, "%ssw $ra, 0($sp)\n", WS);

        fprintf(dest_stream, "%sjal write\n", WS);

        fprintf(dest_stream, "%slw $ra, 0($sp)\n", WS);
        moveSP(4);

        arg_num = 0;
        releaseReg(wt);
        reg_list[2] = FALSE;
    } else if (CODE_ARG == code->kind) {
        Operand* arg = code->u.monop.op;
        int reg_arg = getReg(arg);
        if (arg_num < 4) {
            fprintf(dest_stream, "%smove $a%d, $%d\n", WS, arg_num, reg_arg);
        } else {
            moveSP(-4);
            fprintf(dest_stream, "%ssw $%d, 0($sp)\n", WS, reg_arg);
        }
        arg_num++;
        reg_list[reg_arg] = FALSE;
    } else if (CODE_DEC == code->kind) {
        Operand* op = code->u.decop.op;
        Operand* size = code->u.decop.size;
        Var* var = findVar(op);
        var->mem = sp_offset;
        moveSP(-size->u.i);
    } else if (CODE_PARAM == code->kind) {
        Operand* param = code->u.monop.op;
        Var* var = findVar(param);
        if (param_num < 4) {
            var->reg = 4 + param_num;
        } else {
            var->mem = sp_offset + 4 * (param_num - 3);
        }
        param_num += 1;
    }
    memset(reg_list, FALSE, sizeof(reg_list));
}

/*
0       : zero, 常数0
1       : at, 汇编器保留
2-3     : v0-v1, 表达式求值或函数结果
4-7     : a0-a3, 函数的首四个参数
8-15    : t0-t7, 调用者保存寄存器
16-23   : s0-s7, 被调用者保存寄存器
24-25   : t8-t9, 调用者保存寄存器
26-27   : k0-k1, 中断处理保留
28      : gp, 指向静态数据段64K内存空间的中部
29      : sp, 栈顶指针
30      : s8/fp, 普通的s8或者帧指针
31      : ra, 返回地址
*/
int getReg(Operand* op)
{
    if (OP_INT == op->kind) {
        int reg_int = findEmptyReg();
        fprintf(dest_stream, "%sli $%d, %d\n", WS, reg_int, op->u.i);
        return reg_int;
    } else if (OP_TEMP_VAR == op->kind || OP_VARIABLE == op->kind) {
        Var* var = findVar(op);
        if (var->reg != 0) {
            return var->reg;
        } else if (var->mem != 0) {
            int reg_var = findEmptyReg();
            fprintf(dest_stream, "%slw $%d, %d($sp)\n", WS, reg_var, (var->mem - sp_offset));
            var->reg = reg_var;
            return var->reg;
        } else {
            var->reg = findEmptyReg();
            return var->reg;
        }
    } else {
        printf("Can't store in reg!\n");
        assert(0);
        return -1;
    }
}

void releaseReg(Operand* op)
{
    Var* var = findVar(op);
    if (NULL == var || 0 == var->reg)
        return;
    else if (0 == var->mem) {
        moveSP(-4);
        fprintf(dest_stream, "%ssw $%d, 0($sp)\n", WS, var->reg);
        reg_list[var->reg] = FALSE;
        var->reg = 0;
        var->mem = sp_offset;
    } else {
        fprintf(dest_stream, "%ssw $%d, %d($sp)\n", WS, var->reg, (var->mem - sp_offset));
        reg_list[var->reg] = FALSE;
        var->reg = 0;
    }
}

int findEmptyReg()
{
    int id = 8;
    while (reg_list[id] != FALSE) id += 1;
    reg_list[id] = TRUE;
    return id;
}

Var* findVar(Operand* op)
{
    if (OP_VARIABLE == op->kind) {
        return &var_list[op->u.i % MAX_LIST];
    } else if (OP_TEMP_VAR == op->kind) {
        return &tem_list[op->u.i % MAX_LIST];
    } else {
        if (TRUE == DEBUG) printf("Not a Variable!!\n");
        return NULL;
    }
}

void moveSP(int dis)
{
    sp_offset += dis;
    fprintf(dest_stream, "%saddi $sp, $sp, %d\n", WS, dis);
}