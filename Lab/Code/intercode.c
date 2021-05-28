#include "intercode.h"

#include <assert.h>
#include <stdarg.h>

#define DEBUG FALSE

// 用于统计现在到第几个了
int temp_var_id = 0;
int label_id = 0;
int var_id = 0;

InterCode* now = NULL;
InterCode* arg_list = NULL;

int isStruct[MAX_DEPTH];
int isFuncParam = FALSE;

Operand op_true = {.kind = OP_INT, .u.i = TRUE};
Operand op_false = {.kind = OP_INT, .u.i = FALSE};

FILE* dest_stream = NULL;

static void Program(Node* node);
static void ExtDefList(Node* node);
static void ExtDef(Node* node);
static void ExtDecList(Type* type, Node* node);
static Type* Specifier(Node* node);
static Type* StructSpecifier(Node* node);
static char* VarDec(HashNode* hash, Node* node);
static HashNode* FunDec(Type* return_type, Node* fun_dec);
static FieldList* VarList(Node* node);
static void ParamDec(Node* param_dec);
static void CompSt(Node* node, Type* return_type);
static void StmtList(Node* node, Type* return_type);
static void Stmt(Node* node, Type* return_type);
static FieldList* DefList(Node* node);
static void Def(Node* node);
static void DecList(Type* type, Node* node);
static void Dec(Type* type, Node* node);
static Operand* Exp(Node* node);
static void Args(Node* node);

static void addCode(InterCode* code);
static void printOp(Operand* op);
static void printCode(InterCode* code);
static Operand* initTempVar();
static Operand* initLabel();
static Operand* initOPint(int kind, int val);
static Operand* initOPfloat(int kind, float flt);
static Operand* initOPstr(int kind, char* str);
static Operand* initVar(HashNode* hashnode);
static InterCode* initInterCode(int add, int kind, ...);
static int getRelop(Node* node);
static int getSize(Type* type);
static Operand* getMem(Operand* op);
static int findFieldPos(Type* type, char* name);
static void translate_Cond(Node* node, Operand* L_true, Operand* L_false);
static Type* findFieldID(Type* type, char* name);
static void copyArr(Operand* l, Operand* r);

static void optimize();

void printInterCode(FILE* stream)
{
    if (TRUE == DEBUG) printf("Start IR ...\n");
    InterCode* interhead = (InterCode*)malloc(sizeof(InterCode));
    memset(interhead, 0, sizeof(InterCode));
    now = interhead;

    memset(isStruct, FALSE, sizeof(isStruct));
    initHashTable();

    Program(root);
    optimize();

    if (TRUE == DEBUG) printf("Start Print Code ...\n");
    InterCode* prt = interhead;
    dest_stream = stream;
    while (prt->next != NULL) {
        prt = prt->next;
        printCode(prt);
    }

    delField(0);
}

Operand* initOP(int kind)
{
    Operand* op = (Operand*)malloc(sizeof(Operand));
    op->kind = kind;
    op->isAddress = FALSE;
    op->type = NULL;
    return op;
}

Operand* initOPint(int kind, int val)
{
    Operand* op = initOP(kind);
    op->u.i = (int)val;
    op->type = &Type_int;
    return op;
}

Operand* initOPfloat(int kind, float flt)
{
    Operand* op = initOP(kind);
    op->u.f = (float)flt;
    op->type = &Type_float;
    return op;
}

Operand* initOPstr(int kind, char* str)
{
    Operand* op = initOP(kind);
    op->u.name = str;
    return op;
}

InterCode* initInterCode(int auto_add, int kind, ...)
{
    InterCode* code = (InterCode*)malloc(sizeof(InterCode));
    code->kind = kind;

    va_list op_list;
    va_start(op_list, kind);
    if (CODE_ASSIGN == kind) {
        code->u.assign.left = va_arg(op_list, Operand*);
        code->u.assign.right = va_arg(op_list, Operand*);
    } else if (CODE_LABEL == kind || CODE_FUNCTION == kind || CODE_GOTO == kind || CODE_RETURN == kind ||
               CODE_ARG == kind || CODE_PARAM == kind || CODE_READ == kind || CODE_WRITE == kind) {
        code->u.monop.op = va_arg(op_list, Operand*);
    } else if (CODE_ADD == kind || CODE_SUB == kind || CODE_MUL == kind || CODE_DIV == kind) {
        code->u.binop.result = va_arg(op_list, Operand*);
        code->u.binop.op1 = va_arg(op_list, Operand*);
        code->u.binop.op2 = va_arg(op_list, Operand*);
    } else if (CODE_IFGOTO == kind) {
        code->u.ifgoto.op1 = va_arg(op_list, Operand*);
        code->u.ifgoto.relop = va_arg(op_list, Operand*);
        code->u.ifgoto.op2 = va_arg(op_list, Operand*);
        code->u.ifgoto.dest = va_arg(op_list, Operand*);
    } else if (CODE_DEC == kind) {
        code->u.decop.op = va_arg(op_list, Operand*);
        code->u.decop.size = va_arg(op_list, Operand*);
    } else {
        printf("Wrong Code Type!\n");
    }
    va_end(op_list);
    code->next = NULL;
    code->prev = NULL;
    if (TRUE == auto_add) addCode(code);
    return code;
}

/*
Program : ExtDefList
*/
void Program(Node* node) { ExtDefList(node->children[0]); }

/*
ExtDefList : ExtDef ExtDefList
           | %empty
*/
void ExtDefList(Node* node)
{
    if (NULL == node) return;
    if (TRUE == DEBUG) printf("ExtDefList\n");
    Node* ext_def_list = node;
    while (ext_def_list != NULL) {
        ExtDef(ext_def_list->children[0]);
        ext_def_list = ext_def_list->children[1];
    }
}

/*
ExtDef 	: Specifier ExtDecList ";"  0
        | Specifier ";"             1
        | Specifier FunDec CompSt   2
        | Specifier FunDec ";"      3
*/
void ExtDef(Node* node)
{
    if (TRUE == DEBUG) printf("ExtDef\n");
    Type* type = Specifier(node->children[0]);
    if (2 == node->prod_id) {  // Specifier FunDec CompSt
        HashNode* func = FunDec(type, node->children[1]);
        func->type->u.function.status = DEF;
        insertSymbol(func);
        CompSt(node->children[2], func->type->u.function.return_type);
    } else if (3 == node->prod_id) {  // Specifier FunDec ";"
        HashNode* func = FunDec(type, node->children[1]);
        // 因为函数的参数还存在depth + 1的栈上，对于函数声明，要清空这些内容
        delField(depth + 1);
        func->type->u.function.status = DEC;
        insertSymbol(func);
    } else if (1 == node->prod_id) {
        // 插入结构体的操作在StructSpecifier中完成
        return;
    } else if (0 == node->prod_id) {
        ExtDecList(type, node->children[1]);
    } else {
        printf("Wrong ExtDef prod_id!\n");
    }
}

/*
ExtDecList : VarDec
           | VarDec , ExtDecList
*/
void ExtDecList(Type* type, Node* node)
{
    if (TRUE == DEBUG) printf("ExtDecList\n");
    Node* ext_dec_list = node;
    while (ext_dec_list != NULL) {
        HashNode* hash = initSymbol(NULL, type, depth);
        VarDec(hash, ext_dec_list->children[0]);
        insertSymbol(hash);
        ext_dec_list = ext_dec_list->children[2];
    }
}

/*
Specifier : TYPE                0
          | StructSpecifier     1
*/
Type* Specifier(Node* node)
{
    if (TRUE == DEBUG) printf("Specifier\n");
    if (0 == node->prod_id) {
        if (0 == strcmp(node->children[0]->data.str, "int")) {
            return &Type_int;
        } else {
            return &Type_float;
        }
    } else {  // 结构体类型
        return StructSpecifier(node->children[0]);
    }
}

// 这里我将结构体名也当成变量插入符号表，因为错误类型16是这样保证的。
/*
StructSpecifier : STRUCT OptTag { DefList }     0
                | STRUCT Tag                    1
*/
Type* StructSpecifier(Node* node)
{
    if (TRUE == DEBUG) printf("StructSpecifier\n");
    // structure是结构体域的头结点，存储的是结构体的名字
    if (0 == node->prod_id) {  // STRUCT OptTag { DefList }
        Type* type = (Type*)malloc(sizeof(Type));
        type->kind = STRUCTURE;
        type->u.structure = initFieldList(NULL, NULL);

        FieldList* structure = type->u.structure;
        push();
        isStruct[depth] = TRUE;
        structure->next = DefList(node->children[3]);
        isStruct[depth] = FALSE;
        pop();
        if (NULL != node->children[1]) {
            structure->name = node->children[1]->children[0]->data.str;
            // 这里depth为0，即默认结构体名都是全局的
            HashNode* struct_type = initSymbol(structure->name, type, 0);
            insertSymbol(struct_type);
        }
        // 对于匿名结构体，因为之后不可能再使用了，所以不需要插入符号表
        return type;
    } else {  // STRUCT Tag
        char* name = node->children[1]->children[0]->data.str;
        HashNode* struct_type = findSymbol(name);
        return struct_type->type;
    }
}

/*
VarDec : ID                 0
       | VarDec [ INT ]     1
*/
char* VarDec(HashNode* hash, Node* node)
{
    if (TRUE == DEBUG) printf("VarDec\n");
    if (node->prod_id == 0) {
        hash->name = node->children[0]->data.str;
        if (ARRAY == hash->type->kind && !isFuncParam && !isStruct[depth]) {
            Operand* size = initOPint(OP_INT, getSize(hash->type));
            Operand* arr = initVar(hash);
            arr->isAddress = FALSE;
            InterCode* dec_arr = initInterCode(TRUE, CODE_DEC, arr, size);
        } else if (STRUCTURE == hash->type->kind && !isFuncParam && !isStruct[depth]) {
            Operand* size = initOPint(OP_INT, getSize(hash->type));
            Operand* st = initVar(hash);
            st->isAddress = FALSE;
            InterCode* dec_st = initInterCode(TRUE, CODE_DEC, st, size);
        }
        return hash->name;
    } else {
        Type* arr = (Type*)malloc(sizeof(Type));
        arr->kind = ARRAY;
        arr->u.array.size = node->children[2]->data.i;
        arr->u.array.elem = hash->type;

        arr->u.array.space = getSize(arr->u.array.elem);
        hash->type = arr;
        return VarDec(hash, node->children[0]);
    }
}

// 函数的参数列表应该入栈，在函数体内不可再定义
// 交给上层来将函数插入符号表，因为不清楚是定义还是声明
// 函数和结构体不同，函数的params链表第一个就是参数，而不是函数名代表的链表头
/*
FunDec : ID ( VarList )     0
       | ID ( )             1
*/
HashNode* FunDec(Type* return_type, Node* node)
{
    if (TRUE == DEBUG) printf("FunDec\n");

    Type* type = (Type*)malloc(sizeof(Type));
    type->kind = FUNCTION;
    type->u.function.return_type = return_type;
    HashNode* hash = initSymbol(node->children[0]->data.str, type, depth);
    type = hash->type;

    Operand* func = initOPstr(OP_FUNCTION, hash->name);
    InterCode* code = initInterCode(TRUE, CODE_FUNCTION, func);

    if (1 == node->prod_id) {  // ID ( )
        type->u.function.params = NULL;
    } else {  // ID ( VarList )
        depth += 1;
        isFuncParam = TRUE;
        type->u.function.params = VarList(node->children[2]);
        isFuncParam = FALSE;
        depth -= 1;
        // 让CompSt的操作更加统一
    }
    return hash;
}

/*
VarList : ParamDec , VarList
        | ParamDec
*/
FieldList* VarList(Node* node)
{
    if (TRUE == DEBUG) printf("VarList\n");
    Node* var_list = node;
    while (var_list != NULL) {
        ParamDec(var_list->children[0]);
        var_list = var_list->children[2];
    }
    return conv2FieldList(depth);
}

/*
ParamDec : Specifier VarDec
*/
void ParamDec(Node* node)
{
    if (TRUE == DEBUG) printf("ParamDec\n");
    Type* type = Specifier(node->children[0]);
    HashNode* hash = initSymbol(NULL, type, depth);
    VarDec(hash, node->children[1]);
    hash->is_param = TRUE;
    insertSymbol(hash);

    Operand* param = initVar(hash);
    InterCode* code = initInterCode(TRUE, CODE_PARAM, param);
}

/*
CompSt : { DefList StmtList }
*/
void CompSt(Node* node, Type* return_type)
{
    if (TRUE == DEBUG) printf("CompSt\n");
    push();
    DefList(node->children[1]);
    StmtList(node->children[2], return_type);
    pop();
    if (TRUE == DEBUG) printf("exit CompSt\n");
}

/*
StmtList : Stmt StmtList
         | %empty
*/
void StmtList(Node* node, Type* return_type)
{
    if (NULL == node) return;
    if (TRUE == DEBUG) printf("StmtList\n");
    Node* stmt_list = node;
    while (stmt_list != NULL) {
        Stmt(stmt_list->children[0], return_type);
        stmt_list = stmt_list->children[1];
    }
}

/*
Stmt : Exp ;                        0
     | CompSt                       1
     | RETURN Exp ;                 2
     | IF ( Exp ) Stmt              3
     | IF ( Exp ) Stmt ELSE Stmt    4
     | WHILE ( Exp ) Stmt           5
*/
void Stmt(Node* node, Type* return_type)
{
    if (TRUE == DEBUG) printf("Stmt\n");
    if (0 == node->prod_id) {  // Exp ;
        Exp(node->children[0]);
    } else if (1 == node->prod_id) {  // CompSt
        CompSt(node->children[0], return_type);
    } else if (2 == node->prod_id) {  // RETURN Exp ;
        Operand* op = Exp(node->children[1]);
        InterCode* code = initInterCode(TRUE, CODE_RETURN, op);
    } else if (3 == node->prod_id) {  // IF ( Exp ) Stmt
        Operand* L1 = initLabel();
        Operand* L2 = initLabel();
        translate_Cond(node->children[2], L1, L2);
        InterCode* label1 = initInterCode(TRUE, CODE_LABEL, L1);
        Stmt(node->children[4], return_type);
        InterCode* label2 = initInterCode(TRUE, CODE_LABEL, L2);
    }

    else if (4 == node->prod_id) {  // IF ( Exp ) Stmt ELSE Stmt
        Operand* L1 = initLabel();
        Operand* L2 = initLabel();
        Operand* L3 = initLabel();
        translate_Cond(node->children[2], L1, L2);
        InterCode* label1 = initInterCode(TRUE, CODE_LABEL, L1);
        Stmt(node->children[4], return_type);
        InterCode* goto3 = initInterCode(TRUE, CODE_GOTO, L3);
        InterCode* label2 = initInterCode(TRUE, CODE_LABEL, L2);
        Stmt(node->children[6], return_type);
        InterCode* label3 = initInterCode(TRUE, CODE_LABEL, L3);

    } else if (5 == node->prod_id) {  // WHILE ( Exp ) Stmt
        Operand* L1 = initLabel();
        Operand* L2 = initLabel();
        Operand* L3 = initLabel();
        InterCode* label1 = initInterCode(TRUE, CODE_LABEL, L1);
        translate_Cond(node->children[2], L2, L3);
        InterCode* label2 = initInterCode(TRUE, CODE_LABEL, L2);
        Stmt(node->children[4], return_type);
        InterCode* goto1 = initInterCode(TRUE, CODE_GOTO, L1);
        InterCode* label3 = initInterCode(TRUE, CODE_LABEL, L3);
    } else {
        printf("Wrong Stmt prod_id!\n");
    }
}

/*
DefList : Def DefList
        | %empty
*/
FieldList* DefList(Node* node)
{
    if (NULL == node) return NULL;
    if (TRUE == DEBUG) printf("DefList\n");
    Node* def_list = node;
    while (NULL != def_list) {
        Def(def_list->children[0]);
        def_list = def_list->children[1];
    }
    if (TRUE == isStruct[depth]) {
        return conv2FieldList(depth);
    } else {
        return NULL;
    }
}

/*
Def : Specifier DecList ;
*/
void Def(Node* node)
{
    if (TRUE == DEBUG) printf("Def\n");
    Type* type = Specifier(node->children[0]);
    DecList(type, node->children[1]);
}

/*
DecList : Dec
        | Dec , DecList
*/
void DecList(Type* type, Node* node)
{
    if (TRUE == DEBUG) printf("DecList\n");
    Node* dec_list = node;
    while (NULL != dec_list) {
        Dec(type, dec_list->children[0]);
        dec_list = dec_list->children[2];
    }
}

/*
Dec : VarDec            0
    | VarDec = Exp      1
*/
void Dec(Type* type, Node* node)
{
    if (TRUE == DEBUG) printf("Dec\n");
    HashNode* hash = initSymbol(NULL, type, depth);
    char* var_name = VarDec(hash, node->children[0]);

    insertSymbol(hash);

    // TODO: 多种赋值情况
    if (1 == node->prod_id) {
        Operand* l = initVar(hash);
        Operand* r = Exp(node->children[2]);
        if (ARRAY == l->type->kind && ARRAY == r->type->kind) {
            copyArr(l, r);
        } else {
            InterCode* code = initInterCode(TRUE, CODE_ASSIGN, l, r);
        }
    }
}

/*
Exp : Exp "=" Exp       0
    | Exp "&&" Exp      1
    | Exp "||" Exp      2
    | Exp RELOP Exp     3
    | Exp "+" Exp       4
    | Exp "-" Exp       5
    | Exp "*" Exp       6
    | Exp "/" Exp       7
    | "(" Exp ")"       8
    | "-" Exp           9
    | "!" Exp           10
    | ID "(" Args ")"   11
    | ID "(" ")"        12
    | Exp "[" Exp "]"   13
    | Exp "." ID        14
    | ID                15
    | INT               16
    | FLOAT             17
*/

Operand* Exp(Node* node)
{
    if (TRUE == DEBUG) printf("Exp\n");
    if (NULL == node) return NULL;
    if (16 == node->prod_id) {  // INT
        Operand* op = initOPint(OP_INT, node->children[0]->data.i);
        return op;
    } else if (17 == node->prod_id) {  // FLOAT
        Operand* op = initOPfloat(OP_FLOAT, node->children[0]->data.f);
        return op;
    } else if (15 == node->prod_id) {  // ID
        char* id_name = node->children[0]->data.str;
        HashNode* id = findSymbol(id_name);
        Operand* op = initVar(id);
        return op;
    } else if (12 == node->prod_id) {  // ID ( )
        char* id_name = node->children[0]->data.str;
        if (strcmp(id_name, "read") == 0) {
            Operand* t = initTempVar();
            InterCode* read = initInterCode(TRUE, CODE_READ, t);
            return t;
        } else {
            HashNode* id = findSymbol(id_name);
            Operand* t = initTempVar();
            Operand* func = initOPstr(OP_FUNCTION, id_name);
            InterCode* funcall = initInterCode(TRUE, CODE_ASSIGN, t, func);
            t->type = id->type->u.function.return_type;
            return t;
        }
    } else if (11 == node->prod_id) {  // ID ( Args )
        char* id_name = node->children[0]->data.str;
        if (strcmp(id_name, "write") == 0) {
            Operand* op = Exp(node->children[2]->children[0]);
            Operand* temp = getMem(op);
            InterCode* write = initInterCode(TRUE, CODE_WRITE, temp);
            Operand* zero = initOPint(OP_INT, 0);
            return zero;
        } else {
            HashNode* id = findSymbol(id_name);

            arg_list = NULL;
            Args(node->children[2]);
            // 这里addCode是将所有参数都加进去了
            addCode(arg_list);
            arg_list = NULL;

            Operand* t = initTempVar();
            Operand* func = initOPstr(OP_FUNCTION, id->name);
            InterCode* funcall = initInterCode(TRUE, CODE_ASSIGN, t, func);
            t->type = id->type->u.function.return_type;
            return t;
        }
    } else if (8 == node->prod_id) {  // ( Exp )
        return Exp(node->children[1]);
    } else if (9 == node->prod_id) {  // - Exp
        Node* exp = node->children[1];
        Operand* op = Exp(exp);
        if (16 == exp->prod_id) {
            op->u.i = -op->u.i;
            return op;
        } else if (17 == exp->prod_id) {
            op->u.f = -op->u.f;
            return op;
        } else {
            Operand* temp = initTempVar();
            Operand* zero = initOPint(OP_INT, 0);
            InterCode* minus = initInterCode(TRUE, CODE_SUB, temp, zero, op);
            temp->type = op->type;
            return temp;
        }
    } else if (13 == node->prod_id) {  // Exp [ Exp ]
        Operand* arr = Exp(node->children[0]);
        if (OP_VARIABLE != arr->kind) arr->isAddress = FALSE;
        Operand* index = Exp(node->children[2]);

        Operand* bias = initTempVar();
        Operand* mul_op1 = initOPint(OP_INT, arr->type->u.array.space);
        InterCode* count_bias = initInterCode(TRUE, CODE_MUL, bias, index, mul_op1);

        Operand* res = initTempVar();
        InterCode* add_bias = initInterCode(TRUE, CODE_ADD, res, arr, bias);

        Operand* return_res = initOPint(OP_TEMP_VAR, res->u.i);
        return_res->isAddress = TRUE;
        return_res->type = arr->type->u.array.elem;
        return return_res;
    } else if (14 == node->prod_id) {  // Exp . ID
        Operand* st = Exp(node->children[0]);
        if (OP_VARIABLE != st->kind) st->isAddress = FALSE;
        char* id_name = node->children[2]->data.str;

        Operand* bias = initOPint(OP_INT, 0);
        bias->u.i = findFieldPos(st->type, id_name);

        Operand* res = initTempVar();
        InterCode* code = initInterCode(TRUE, CODE_ADD, res, st, bias);

        Operand* return_res = initOPint(OP_TEMP_VAR, res->u.i);
        return_res->isAddress = TRUE;
        return_res->type = findFieldID(st->type, id_name);
        return return_res;
    } else if (0 == node->prod_id) {  // Exp = Exp
        Operand* l = Exp(node->children[0]);
        Operand* r = Exp(node->children[2]);
        if (ARRAY == l->type->kind && ARRAY == r->type->kind) {
            copyArr(l, r);
        } else {
            InterCode* code = initInterCode(TRUE, CODE_ASSIGN, l, r);
        }
        return l;
    } else if (4 == node->prod_id || 5 == node->prod_id || 6 == node->prod_id || 7 == node->prod_id) {  // Exp +-*/ Exp
        // 如果运算符不匹配，只进行报错，并返回左边表达式的类型
        Operand* l = Exp(node->children[0]);
        Operand* r = Exp(node->children[2]);
        Operand* t = initTempVar();
        InterCode* math = NULL;
        if (4 == node->prod_id) {  // Exp + Exp
            math = initInterCode(FALSE, CODE_ADD, t, l, r);
        } else if (5 == node->prod_id) {  // Exp - Exp
            math = initInterCode(FALSE, CODE_SUB, t, l, r);
        } else if (6 == node->prod_id) {  // Exp * Exp
            math = initInterCode(FALSE, CODE_MUL, t, l, r);
        } else if (7 == node->prod_id) {  // Exp / Exp
            math = initInterCode(FALSE, CODE_DIV, t, l, r);
        }
        addCode(math);
        t->type = l->type;
        return t;
    } else {  // Exp RELOP Exp, NOT Exp, Exp AND Exp, Exp OR Exp
        Operand* L1 = initLabel();
        Operand* L2 = initLabel();
        Operand* result = initTempVar();
        InterCode* assign_false = initInterCode(TRUE, CODE_ASSIGN, result, &op_false);
        translate_Cond(node, L1, L2);
        InterCode* label1 = initInterCode(TRUE, CODE_LABEL, L1);
        InterCode* assign_true = initInterCode(TRUE, CODE_ASSIGN, result, &op_true);
        InterCode* label2 = initInterCode(TRUE, CODE_LABEL, L2);
        result->type = &Type_int;
        return result;
    }
}

/*
Args : Exp "," Args
       Exp
*/

void Args(Node* node)
{
    if (TRUE == DEBUG) printf("Args\n");
    Operand* op = Exp(node->children[0]);
    if (OP_TEMP_VAR == op->kind && (ARRAY == op->type->kind || STRUCTURE == op->type->kind)) op->isAddress = FALSE;
    InterCode* arg = initInterCode(FALSE, CODE_ARG, op);
    if (NULL == arg_list) {
        arg_list = arg;
    } else {
        arg->next = arg_list;
        arg_list->prev = arg;
        arg_list = arg;
    }
    if (0 == node->prod_id) {  // Exp "," Args
        Args(node->children[2]);
    }
}

Operand* getMem(Operand* op)
{
    if (TRUE == op->isAddress) {
        Operand* temp = initTempVar();
        InterCode* get_mem = initInterCode(TRUE, CODE_ASSIGN, temp, op);
        return temp;
    } else {
        return op;
    }
}

void translate_Cond(Node* node, Operand* L_true, Operand* L_false)
{
    if (3 == node->prod_id) {  // Exp := Exp RELOP Exp
        Operand* l = Exp(node->children[0]);
        Operand* r = Exp(node->children[2]);
        Operand* relop = initOPint(OP_RELOP, getRelop(node->children[1]));
        InterCode* ifgoto_true = initInterCode(TRUE, CODE_IFGOTO, l, relop, r, L_true);
        InterCode* goto_false = initInterCode(TRUE, CODE_GOTO, L_false);
    } else if (10 == node->prod_id) {  // Exp := ! Exp
        return translate_Cond(node->children[1], L_false, L_true);
    } else if (1 == node->prod_id) {  // Exp := Exp && Exp
        Operand* L1 = initLabel();
        translate_Cond(node->children[0], L1, L_false);
        InterCode* label1 = initInterCode(TRUE, CODE_LABEL, L1);
        translate_Cond(node->children[2], L_true, L_false);
    } else if (2 == node->prod_id) {  // Exp := Exp || Exp
        Operand* L1 = initLabel();
        translate_Cond(node->children[0], L_true, L1);
        InterCode* code = initInterCode(TRUE, CODE_LABEL, L1);
        translate_Cond(node->children[2], L_true, L_false);
    } else {
        Operand* op = Exp(node);
        Operand* zero = initOPint(OP_INT, 0);
        InterCode* ifgoto_true = initInterCode(TRUE, CODE_IFGOTO, op, initOPint(OP_RELOP, NE), zero, L_true);
        InterCode* code2 = initInterCode(TRUE, CODE_GOTO, L_false);
    }
}

void copyArr(Operand* l, Operand* r)
{
    if (OP_TEMP_VAR == l->kind) l->isAddress = FALSE;
    if (OP_TEMP_VAR == r->kind) r->isAddress = FALSE;
    int l_size = getSize(l->type);
    int r_size = getSize(r->type);
    int cp_size = l_size > r_size ? r_size : l_size;
    Operand* size = initOPint(OP_INT, cp_size);
    Operand* pos = initTempVar();
    InterCode* init_pos = initInterCode(TRUE, CODE_ASSIGN, pos, initOPint(OP_INT, 0));

    Operand* L1 = initLabel();
    Operand* L2 = initLabel();
    Operand* L3 = initLabel();
    InterCode* label1 = initInterCode(TRUE, CODE_LABEL, L1);
    // Exp
    InterCode* ifgoto_true = initInterCode(TRUE, CODE_IFGOTO, pos, initOPint(OP_RELOP, LT), size, L2);
    InterCode* code2 = initInterCode(TRUE, CODE_GOTO, L3);

    InterCode* label2 = initInterCode(TRUE, CODE_LABEL, L2);
    // Stmt
    Operand* temp1 = initTempVar();
    Operand* temp2 = initTempVar();

    Operand* assign_t1 = initOP(OP_TEMP_VAR);
    assign_t1->u.i = temp1->u.i;
    assign_t1->isAddress = TRUE;
    Operand* assign_t2 = initOP(OP_TEMP_VAR);
    assign_t2->u.i = temp2->u.i;
    assign_t2->isAddress = TRUE;

    InterCode* addt1 = initInterCode(TRUE, CODE_ADD, temp1, l, pos);
    InterCode* addt2 = initInterCode(TRUE, CODE_ADD, temp2, r, pos);
    InterCode* assign = initInterCode(TRUE, CODE_ASSIGN, assign_t1, assign_t2);
    InterCode* add_pos = initInterCode(TRUE, CODE_ADD, pos, pos, initOPint(OP_INT, 4));

    InterCode* goto1 = initInterCode(TRUE, CODE_GOTO, L1);
    InterCode* label3 = initInterCode(TRUE, CODE_LABEL, L3);
}

void printCode(InterCode* code)
{
    if (TRUE == DEBUG) printf("printCode, kind = %d\n", code->kind);
    int k = code->kind;
    if (CODE_ASSIGN == k) {
        printOp(code->u.assign.left);
        fprintf(dest_stream, " := ");
        if (OP_FUNCTION == code->u.assign.right->kind) fprintf(dest_stream, "CALL ");
        printOp(code->u.assign.right);
    } else if (CODE_ADD == k || CODE_SUB == k || CODE_MUL == k || CODE_DIV == k) {
        printOp(code->u.binop.result);
        fprintf(dest_stream, " := ");
        printOp(code->u.binop.op1);
        if (CODE_ADD == k)
            fprintf(dest_stream, " + ");
        else if (CODE_SUB == k)
            fprintf(dest_stream, " - ");
        else if (CODE_MUL == k)
            fprintf(dest_stream, " * ");
        else if (CODE_DIV == k)
            fprintf(dest_stream, " / ");
        else
            printf("Wrong BinOP!\n");
        printOp(code->u.binop.op2);
    } else if (CODE_LABEL == k || CODE_FUNCTION == k) {
        if (CODE_LABEL == k)
            fprintf(dest_stream, "LABEL ");
        else if (CODE_FUNCTION == k)
            fprintf(dest_stream, "FUNCTION ");
        printOp(code->u.monop.op);
        fprintf(dest_stream, " :");
    } else if (CODE_GOTO == k || CODE_RETURN == k || CODE_ARG == k || CODE_PARAM == k || CODE_READ == k ||
               CODE_WRITE == k) {
        if (CODE_GOTO == k)
            fprintf(dest_stream, "GOTO ");
        else if (CODE_RETURN == k)
            fprintf(dest_stream, "RETURN ");
        else if (CODE_ARG == k)
            fprintf(dest_stream, "ARG ");
        else if (CODE_PARAM == k)
            fprintf(dest_stream, "PARAM ");
        else if (CODE_READ == k) {
            fprintf(dest_stream, "READ ");
        } else if (CODE_WRITE == k) {
            fprintf(dest_stream, "WRITE ");
        } else
            printf("Wrong MonOP!\n");
        printOp(code->u.monop.op);
    } else if (CODE_IFGOTO == k) {
        fprintf(dest_stream, "IF ");
        printOp(code->u.ifgoto.op1);
        fprintf(dest_stream, " ");
        printOp(code->u.ifgoto.relop);
        fprintf(dest_stream, " ");
        printOp(code->u.ifgoto.op2);
        fprintf(dest_stream, " GOTO ");
        printOp(code->u.ifgoto.dest);
    } else if (CODE_DEC == k) {
        fprintf(dest_stream, "DEC ");
        printOp(code->u.decop.op);
        fprintf(dest_stream, " %d", code->u.decop.size->u.i);
    } else {
        printf("Wrong Code Type!\n");
    }
    fprintf(dest_stream, "\n");
}

void printOp(Operand* op)
{
    if (TRUE == DEBUG) printf("printOP\n");
    switch (op->kind) {
    case OP_VARIABLE:
        if (TRUE == op->isAddress) fprintf(dest_stream, "&");
        fprintf(dest_stream, "v%d", op->u.i);
        break;
    case OP_FUNCTION:
        fprintf(dest_stream, "%s", op->u.name);
        break;
    case OP_INT:
        fprintf(dest_stream, "#%d", op->u.i);
        break;
    case OP_FLOAT:
        fprintf(dest_stream, "#%f", op->u.f);
        break;
    case OP_TEMP_VAR:
        if (TRUE == op->isAddress) fprintf(dest_stream, "*");
        fprintf(dest_stream, "t%d", op->u.i);
        break;
    case OP_LABEL:
        fprintf(dest_stream, "L%d", op->u.i);
        break;
    case OP_RELOP:
        switch (op->u.i) {
        case EQ:
            fprintf(dest_stream, "==");
            break;
        case LT:
            fprintf(dest_stream, "<");
            break;
        case GT:
            fprintf(dest_stream, ">");
            break;
        case NE:
            fprintf(dest_stream, "!=");
            break;
        case GE:
            fprintf(dest_stream, ">=");
            break;
        case LE:
            fprintf(dest_stream, "<=");
            break;
        default:
            printf("Wrong Relop!");
            break;
        }
        break;
    default:
        printf("Wrong OP TYPE!\n");
        break;
    }
}

void addCode(InterCode* code)
{
    now->next = code;
    code->prev = now;
    if (code->next != NULL) {
        InterCode* end = code;
        while (end->next != NULL) end = end->next;
        now = end;
        end = NULL;
    } else {
        now = code;
    }
}

int getSize(Type* type)
{
    if (BASIC == type->kind)
        return 4;
    else if (ARRAY == type->kind)
        return type->u.array.space * type->u.array.size;
    else if (STRUCTURE == type->kind) {
        int size = 0;
        FieldList* field = type->u.structure;
        while (field->next != NULL) {
            field = field->next;
            size += getSize(field->type);
        }
        return size;
    } else {
        printf("Wrong Type to Count Size!\n");
        return -1;
    }
}

int findFieldPos(Type* type, char* name)
{
    if (NULL == type || NULL == name) {
        if (TRUE == DEBUG) printf("Input NULL!\n");
        return 0;
    }
    if (type->kind != STRUCTURE) {
        if (TRUE == DEBUG) printf("Not Struct!\n");
        return 0;
    }

    FieldList* field = type->u.structure;
    int pos = 0;

    while (field->next != NULL) {
        field = field->next;
        if (strcmp(field->name, name) == 0) {
            return pos;
        } else {
            pos += getSize(field->type);
        }
    }

    if (TRUE == DEBUG) printf("Not Find Field in Struct!\n");
    return 0;
}

Type* findFieldID(Type* type, char* name)
{
    if (NULL == type || NULL == name) return NULL;
    if (type->kind != STRUCTURE) return NULL;
    // 找到真正的域
    FieldList* field = type->u.structure->next;
    while (field != NULL) {
        if (strcmp(field->name, name) == 0) {
            return field->type;
        } else {
            field = field->next;
        }
    }
    return NULL;
}

// 输入为RELOP节点
int getRelop(Node* node)
{
    char* rel_s = node->data.str;
    if (strcmp(rel_s, "EQ") == 0)
        return EQ;
    else if (strcmp(rel_s, "LT") == 0)
        return LT;
    else if (strcmp(rel_s, "GT") == 0)
        return GT;
    else if (strcmp(rel_s, "NE") == 0)
        return NE;
    else if (strcmp(rel_s, "GE") == 0)
        return GE;
    else if (strcmp(rel_s, "LE") == 0)
        return LE;
    else {
        printf("Wrong RELOP to get!\n");
        return -1;
    }
}

Operand* initLabel()
{
    Operand* label = initOP(OP_LABEL);
    label->u.i = label_id++;
    return label;
}

Operand* initTempVar()
{
    Operand* temp_var = initOP(OP_TEMP_VAR);
    temp_var->u.i = temp_var_id++;
    return temp_var;
}

Operand* initVar(HashNode* hashnode)
{
    Operand* var = initOP(OP_VARIABLE);
    if (hashnode->id < 0) {
        var->u.i = var_id++;
        hashnode->id = var->u.i;
    } else {
        var->u.i = hashnode->id;
    }
    var->type = hashnode->type;

    // 因为函数参数传的就是地址，所以不需要再输出时标记了
    if ((ARRAY == hashnode->type->kind || STRUCTURE == hashnode->type->kind) && FALSE == hashnode->is_param)
        var->isAddress = TRUE;
    else
        var->isAddress = FALSE;

    return var;
}

void optimize()
{
    // TODO: 删除紧邻的标签
}