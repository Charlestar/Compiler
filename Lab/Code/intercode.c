#include "intercode.h"

#include <assert.h>
#include <stdarg.h>

#define DEBUG FALSE

int temp_var_id = 0;
int label_id = 0;
int var_id = 0;

InterCode* now = NULL;

int isStruct[MAX_DEPTH];
int isFuncParam = FALSE;
int arrDim[MAX_ARR_DIM];
int arr_id_ptr = 0;

Operand op_true = {.kind = OP_INT, .u.i = TRUE};
Operand op_false = {.kind = OP_INT, .u.i = FALSE};

FILE* destStream = NULL;

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

static void insertCode(InterCode* brk, InterCode* start);
static void addCode(InterCode* code);
static void printOp(Operand* op);
static void printCode(InterCode* code);
static Operand* initTempVar();
static Operand* initLabel();
static Operand* initOPint(int kind, int val);
static Operand* initOPfloat(int kind, float flt);
static Operand* initOPstr(int kind, char* str);
static InterCode* initInterCode(int kind, ...);
static int getRelop(Node* node);
static int getSize(Type* type);
static int findFieldPos(Type* type, char* name);
static Operand* doRelop(Operand* l, int relop, Operand* r);

static void optimize();

void printInterCode(FILE* stream)
{
    if (TRUE == DEBUG) printf("Start IR ...\n");
    InterCode* interhead = (InterCode*)malloc(sizeof(InterCode));
    memset(interhead, 0, sizeof(InterCode));
    now = interhead;

    memset(isStruct, FALSE, sizeof(isStruct));
    memset(arrDim, 0, sizeof(arrDim));
    initHashTable();

    Program(root);
    if (TRUE == DEBUG) printf("Start Print Code ...\n");
    InterCode* prt = interhead;
    if (NULL == prt) assert(0);
    destStream = stream;
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
    op->isPointer = FALSE;
    return op;
}

Operand* initOPint(int kind, int val)
{
    Operand* op = initOP(kind);
    op->u.i = (int)val;
    return op;
}

Operand* initOPfloat(int kind, float flt)
{
    Operand* op = initOP(kind);
    op->u.f = (float)flt;
    return op;
}

Operand* initOPstr(int kind, char* str)
{
    Operand* op = initOP(kind);
    op->u.name = str;
    return op;
}

InterCode* initInterCode(int kind, ...)
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
            // TODO: 变量改名
            Operand* arr = initOP(OP_VARIABLE);
            arr->u.name = hash->name;

            InterCode* dec_arr = initInterCode(CODE_DEC, arr, size);
            addCode(dec_arr);
        }
        return hash->name;
    } else {
        Type* arr = (Type*)malloc(sizeof(Type));
        arr->kind = ARRAY;
        arr->u.array.size = node->children[2]->data.i;
        arr->u.array.elem = hash->type;

        if (arr->u.array.elem->kind != ARRAY) {
            arr->u.array.dim = 1;
        } else {
            arr->u.array.dim = arr->u.array.elem->u.array.dim + 1;
        }

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

    Operand* func = initOPstr(OP_FUNCTION, node->children[0]->data.str);
    InterCode* code = initInterCode(CODE_FUNCTION, func);
    addCode(code);

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
    char* var_name = VarDec(hash, node->children[1]);

    // TODO: 改变变量名
    Operand* param = initOPstr(OP_VARIABLE, var_name);
    InterCode* code = initInterCode(CODE_PARAM, param);
    addCode(code);

    insertSymbol(hash);
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
        InterCode* code = initInterCode(CODE_RETURN, op);
        addCode(code);
    } else if (3 == node->prod_id || 4 == node->prod_id) {  // IF ( Exp ) Stmt [ELSE Stmt]
        Operand* L1 = initLabel();
        InterCode* ifgoto = NULL;
        if (3 == node->children[2]->prod_id) {  // Exp := Exp RELOP Exp
            int rel = getRelop(node->children[2]->children[1]);
            Operand* op1 = Exp(node->children[2]->children[0]);
            Operand* op2 = Exp(node->children[2]->children[2]);
            Operand* relop = initOPint(OP_RELOP, revRel(rel));
            ifgoto = initInterCode(CODE_IFGOTO, op1, relop, op2, L1);
        } else {
            Operand* op1 = Exp(node->children[2]);
            // 如果是FALSE就进行跳转
            Operand* relop = initOPint(OP_RELOP, EQ);
            ifgoto = initInterCode(CODE_IFGOTO, op1, relop, &op_false, L1);
        }
        addCode(ifgoto);
        Stmt(node->children[4], return_type);
        Operand* L2 = NULL;
        if (4 == node->prod_id) {
            L2 = initLabel();
            InterCode* jump_else = initInterCode(CODE_GOTO, L2);
            addCode(jump_else);
        }
        InterCode* L1_code = initInterCode(CODE_LABEL, L1);
        addCode(L1_code);
        if (4 == node->prod_id) {
            Stmt(node->children[6], return_type);
            InterCode* L2_code = initInterCode(CODE_LABEL, L2);
            addCode(L2_code);
        }
    } else if (5 == node->prod_id) {  // WHILE ( Exp ) Stmt
        Operand* L_start = initLabel();
        InterCode* start = initInterCode(CODE_LABEL, L_start);
        addCode(start);
        Operand* L_end = initLabel();
        InterCode* ifgoto = NULL;
        if (3 == node->children[2]->prod_id) {
            int rel = getRelop(node);
            Operand* op1 = Exp(node->children[2]->children[0]);
            Operand* op2 = Exp(node->children[2]->children[2]);
            Operand* relop = initOPint(OP_RELOP, revRel(rel));
            ifgoto = initInterCode(CODE_IFGOTO, op1, relop, op2, L_end);
        } else {
            Operand* op1 = Exp(node->children[2]);
            Operand* relop = initOPint(OP_RELOP, EQ);
            ifgoto = initInterCode(CODE_IFGOTO, op1, relop, &op_false, L_end);
        }
        addCode(ifgoto);

        Stmt(node->children[4], return_type);
        InterCode* jump2start = initInterCode(CODE_GOTO, L_start);
        addCode(jump2start);
        InterCode* end = initInterCode(CODE_LABEL, L_end);
        addCode(end);
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
        Operand* op2 = Exp(node->children[2]);
        // TODO: 修改变量名
        Operand* op1 = initOPstr(OP_VARIABLE, var_name);
        InterCode* dec_code = initInterCode(CODE_ASSIGN, op1, op2);
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
        // TODO :涉及到将所有变量全变为临时变量
        Operand* op = initOPstr(OP_VARIABLE, id->name);
        return op;
    } else if (11 == node->prod_id || 12 == node->prod_id) {  // 函数
        char* id_name = node->children[0]->data.str;
        if (11 == node->prod_id && strcmp(id_name, "write") == 0) {
            Operand* op = Exp(node->children[2]->children[0]);
            InterCode* write = initInterCode(CODE_WRITE, op);
            addCode(write);
            return &op_true;
        } else {
            HashNode* id = findSymbol(id_name);
            Operand* op = initOPstr(OP_FUNCTION, id->name);
            if (11 == node->prod_id) {  // ID ( Args )
                Node* args = node->children[2];
                InterCode* params = NULL;
                while (args != NULL) {
                    Operand* arg = Exp(args->children[0]);
                    InterCode* param = initInterCode(CODE_ARG, arg);
                    if (params != NULL) {
                        param->next = params;
                        params->prev = param;
                    }
                    params = param;
                    args = args->children[2];
                }
                addCode(params);
            }
            return op;
        }
    } else if (8 == node->prod_id) {  // ( Exp )
        return Exp(node->children[1]);
    } else if (9 == node->prod_id) {  //- Exp
        Operand* op = Exp(node->children[1]);
        if (OP_INT == op->kind)
            op->u.i = -op->u.i;
        else if (OP_FLOAT == op->kind)
            op->u.f = -op->u.f;
        else
            printf("Wrong -Exp!\n");
        return op;
    } else if (10 == node->prod_id) {  // "!" Exp
        Operand* op = Exp(node->children[1]);
        if (op->kind != OP_INT) {
            printf("Error Logical Type!\n");
            return NULL;
        }
        // !认为所有非0的整数为true
        if (FALSE == op->u.i)
            op->u.i = TRUE;
        else
            op->u.i = FALSE;
        return op;
    } else if (13 == node->prod_id) {  // Exp [ Exp ]
        Node* exp = node;
        Operand* bias = initOPint(OP_INT, 0);

        while (13 == exp->prod_id) {  // 嵌套数组
            Operand* index = Exp(exp->children[2]);
            arrDim[arr_id_ptr++] = index->u.i;
            exp = exp->children[0];
        }

        char* arr_name = exp->children[0]->data.str;
        HashNode* hash = findSymbol(arr_name);
        Type* arr_dim = hash->type;
        while (arr_id_ptr != 0) {
            bias->u.i += arrDim[--arr_id_ptr] * arr_dim->u.array.space;
            arr_dim = arr_dim->u.array.elem;
        }
        Operand* temp = initTempVar();
        temp->isPointer = TRUE;
        // TODO: 变量名问题
        Operand* arr = initOPstr(OP_VARIABLE, arr_name);
        InterCode* code = initInterCode(CODE_ADD, temp, arr, bias);
        addCode(code);
        return temp;
    } else if (14 == node->prod_id) {  // Exp . ID
        Node* exp = node->children[0];
        char* id_name = node->children[2]->data.str;
        Operand* bias = initOPint(OP_INT, 0);

        while (14 == exp->prod_id) {  // 嵌套数组
            char* st_name = exp->children[2]->data.str;
            HashNode* st = findSymbol(st_name);
            bias->u.i += findFieldPos(st->type, id_name);
            exp = exp->children[0];
            id_name = st_name;
        }

        char* st_name = exp->children[0]->data.str;
        HashNode* st = findSymbol(st_name);
        bias->u.i += findFieldPos(st->type, id_name);

        Operand* temp = initTempVar();
        temp->isPointer = TRUE;
        Operand* st_op = Exp(exp);
        InterCode* code = initInterCode(CODE_ADD, temp, st_op, bias);
        addCode(code);
        return temp;
    } else if (0 == node->prod_id) {  // Exp = Exp
        Operand* l = Exp(node->children[0]);
        // 对read函数单独处理
        if (12 == node->children[2]->prod_id && strcmp(node->children[2]->children[0]->data.str, "read") == 0) {
            InterCode* read = initInterCode(CODE_READ, l);
            addCode(read);
        } else {
            Operand* r = Exp(node->children[2]);
            InterCode* code = initInterCode(CODE_ASSIGN, l, r);
            addCode(code);
        }
        return l;

    } else if (1 == node->prod_id) {  // Exp && Exp
        Operand* l = Exp(node->children[0]);
        Operand* r = Exp(node->children[2]);
        if (l->kind != OP_INT || r->kind != OP_INT) {
            printf("Not Logical Valure!\n");
            return NULL;
        }
        if (l->u.i != FALSE && r->u.i != FALSE)
            return &op_true;
        else
            return &op_false;
    } else if (2 == node->prod_id) {  // Exp || Exp
        Operand* l = Exp(node->children[0]);
        Operand* r = Exp(node->children[2]);
        if (l->kind != OP_INT || r->kind != OP_INT) {
            printf("Not Logical Valure!\n");
            return NULL;
        }
        if (l->u.i != FALSE || r->u.i != FALSE)
            return &op_true;
        else
            return &op_false;
    } else if (3 == node->prod_id) {  // Exp RELOP Exp
        Operand* l = Exp(node->children[0]);
        Operand* r = Exp(node->children[2]);
        int relop = getRelop(node->children[1]);
        Operand* res = doRelop(l, relop, r);
        return res;
    } else {  // Exp +-*/ Exp
        // 如果运算符不匹配，只进行报错，并返回左边表达式的类型
        Operand* l = Exp(node->children[0]);
        Operand* r = Exp(node->children[2]);
        Operand* t = initTempVar();
        InterCode* math = NULL;
        if (4 == node->prod_id) {  // Exp + Exp
            math = initInterCode(CODE_ADD, t, l, r);
        } else if (5 == node->prod_id) {  // Exp - Exp
            math = initInterCode(CODE_SUB, t, l, r);
        } else if (6 == node->prod_id) {  // Exp * Exp
            math = initInterCode(CODE_MUL, t, l, r);
        } else if (7 == node->prod_id) {  // Exp / Exp
            math = initInterCode(CODE_DIV, t, l, r);
        }
        addCode(math);
    }
}

void printCode(InterCode* code)
{
    // ! 不该有这种情况的
    if (NULL == code) return;
    if (TRUE == DEBUG) printf("printCode\n");
    int k = code->kind;
    if (CODE_ASSIGN == k) {
        printOp(code->u.assign.left);
        fprintf(destStream, " := ");
        if (OP_FUNCTION == code->u.assign.right->kind) fprintf(destStream, "CALL ");
        printOp(code->u.assign.right);
    } else if (CODE_ADD == k || CODE_SUB == k || CODE_MUL == k || CODE_DIV == k) {
        printOp(code->u.binop.result);
        fprintf(destStream, " := ");
        printOp(code->u.binop.op1);
        if (CODE_ADD == k)
            fprintf(destStream, " + ");
        else if (CODE_SUB == k)
            fprintf(destStream, " - ");
        else if (CODE_MUL == k)
            fprintf(destStream, " * ");
        else if (CODE_DIV == k)
            fprintf(destStream, " / ");
        else
            printf("Wrong BinOP!\n");
        printOp(code->u.binop.op2);
    } else if (CODE_LABEL == k || CODE_FUNCTION == k) {
        if (CODE_LABEL == k)
            fprintf(destStream, "LABEL ");
        else if (CODE_FUNCTION == k)
            fprintf(destStream, "FUNCTION ");
        printOp(code->u.monop.op);
        fprintf(destStream, " :");
    } else if (CODE_GOTO == k || CODE_RETURN == k || CODE_ARG == k || CODE_PARAM == k || CODE_READ == k ||
               CODE_WRITE == k) {
        if (CODE_GOTO == k)
            fprintf(destStream, "GOTO ");
        else if (CODE_RETURN == k)
            fprintf(destStream, "RETURN ");
        else if (CODE_ARG == k)
            fprintf(destStream, "ARG ");
        else if (CODE_PARAM == k)
            fprintf(destStream, "PARAM ");
        else if (CODE_READ == k)
            fprintf(destStream, "READ ");
        else if (CODE_WRITE == k)
            fprintf(destStream, "WRITE ");
        else
            printf("Wrong MonOP!\n");
        printOp(code->u.monop.op);
    } else if (CODE_IFGOTO == k) {
        fprintf(destStream, "IF ");
        printOp(code->u.ifgoto.op1);
        fprintf(destStream, " ");
        printOp(code->u.ifgoto.relop);
        fprintf(destStream, " ");
        printOp(code->u.ifgoto.op2);
        fprintf(destStream, " GOTO ");
        printOp(code->u.ifgoto.dest);
    } else if (CODE_DEC == k) {
        fprintf(destStream, "DEC ");
        printOp(code->u.decop.op);
        fprintf(destStream, " %d", code->u.decop.size->u.i);
    } else {
        printf("Wrong Code Type!\n");
    }
    fprintf(destStream, "\n");
}

void printOp(Operand* op)
{
    // !为什么会有为NULL的情况
    if (NULL == op) return;
    if (TRUE == DEBUG) printf("printOP, kind = %d\n", op->kind);
    if (TRUE == op->isAddress) fprintf(destStream, "&");
    if (TRUE == op->isPointer) fprintf(destStream, "*");
    switch (op->kind) {
    case OP_VARIABLE:
        fprintf(destStream, "%s", op->u.name);
        break;
    case OP_FUNCTION:
        fprintf(destStream, "%s", op->u.name);
        break;
    case OP_INT:
        fprintf(destStream, "#%d", op->u.i);
        break;
    case OP_FLOAT:
        fprintf(destStream, "#%f", op->u.f);
        break;
    case OP_TEMP_VAR:
        fprintf(destStream, "t%d", op->u.i);
        break;
    case OP_LABEL:
        fprintf(destStream, "L%d", op->u.i);
        break;
    case OP_RELOP:
        switch (op->u.i) {
        case EQ:
            fprintf(destStream, "==");
            break;
        case LT:
            fprintf(destStream, "<");
            break;
        case GT:
            fprintf(destStream, ">");
            break;
        case NE:
            fprintf(destStream, "!=");
            break;
        case GE:
            fprintf(destStream, ">=");
            break;
        case LE:
            fprintf(destStream, "<=");
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

void insertCode(InterCode* brk, InterCode* start)
{
    InterCode* end = start;
    while (end->next != NULL) end = end->next;
    end->next = brk->next;
    brk->next = start;
    start->prev = brk;
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
    if (NULL == type || NULL == name) return -1;
    if (type->kind != STRUCTURE) return -1;

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

    printf("Not Find Field in Struct!\n");
    return -1;
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

Operand* doRelop(Operand* l, int relop, Operand* r)
{
    float lval = l->kind == OP_FLOAT ? l->u.f : (float)l->u.i;
    float rval = r->kind == OP_FLOAT ? r->u.f : (float)r->u.i;
    switch (relop) {
    case EQ:
        if (lval == rval) return &op_true;
        break;
    case LT:
        if (lval < rval) return &op_true;
        break;
    case GT:
        if (lval > rval) return &op_true;
        break;
    case NE:
        if (lval != rval) return &op_true;
        break;
    case GE:
        if (lval >= rval) return &op_true;
        break;
    case LE:
        if (lval <= rval) return &op_true;
        break;
    default:
        printf("Wrong RELOP to do!\n");
        break;
    }
    return &op_false;
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

Operand* initVar()
{
    Operand* var = initOP(OP_VARIABLE);
    var->u.i = var_id++;
    return var;
}

void optimize()
{
    // TODO: 删除紧邻的标签
}