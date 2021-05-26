#include "intercode.h"

#include <stdarg.h>

int temp_var_id = 0;
int label_id = 0;

InterCode* now = NULL;

int isStruct[MAX_DEPTH];
int isFuncParam = FALSE;
int arrDim[MAX_ARR_DIM];
int arr_id_ptr = 0;

Operand op_true = {.kind = OP_INT, .u.i = TRUE};
Operand op_false = {.kind = OP_INT, .u.i = FALSE};

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

void initHashTable();
void insertCode(InterCode* brk, InterCode* start);
void printOp(Operand* op);
void printCode(InterCode* code);
Operand* initTempVar();
Operand* initLabel();
int getRelop(Node* node);
InterCode* prepareParam(Node* node);
int getSize(Type* type);
int findFieldPos(Type* type, char* name);
Operand* doRelop(Operand* l, int relop, Operand* r);

void printInterCode()
{
    InterCode* interhead = (InterCode*)malloc(sizeof(InterCode));
    memset(interhead, 0, sizeof(InterCode));
    now = interhead;
    memset(isStruct, FALSE, sizeof(isStruct));
    initHashTable();
    Program(root);
    InterCode* prt = interhead;
    while (prt->next != NULL) {
        printCode(prt);
        prt = prt->next;
    }
    delField(0);
}

Operand* initOP(int kind)
{
    Operand* op = (Operand*)malloc(sizeof(Operand));
    op->kind = kind;
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
    } else if (CODE_DECOP == kind) {
        code->u.decop.op = va_arg(op_list, Operand*);
        code->u.decop.size = va_arg(op_list, Operand*);
    } else {
        printf("Wrong Code Type!");
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
    if (NULL == type) return;
    if (2 == node->prod_id) {  // Specifier FunDec CompSt
        HashNode* func = FunDec(type, node->children[1]);
        if (NULL == func) {
            delField(depth + 1);
            return;
        }
        func->type->u.function.status = DEF;
        insertSymbol(func);
        CompSt(node->children[2], func->type->u.function.return_type);
    } else if (3 == node->prod_id) {  // Specifier FunDec ";"
        HashNode* func = FunDec(type, node->children[1]);
        // 因为函数的参数还存在depth+1的栈上，对于函数声明，要清空这些内容
        delField(depth + 1);
        if (NULL == func) return;
        func->type->u.function.status = DEC;
        insertSymbol(func);
    } else if (1 == node->prod_id) {
        // 插入结构体的操作在StructSpecifier中完成
        return;
    } else if (0 == node->prod_id) {
        ExtDecList(type, node->children[1]);
    } else {
        printf("There must be something wrong when build ExtDef node!");
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
    while (NULL != ext_dec_list) {
        HashNode* hash = initSymbol(NULL, type, depth);
        char* var_name = VarDec(hash, ext_dec_list->children[0]);
        insertSymbol(hash);
        hash = NULL;
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
    // 为了达到错误类型16，将结构体类型放到一个同名变量中，并将该变量插入符号表
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
            if (TRUE == DEBUG) printf("%s\n", struct_type->name);
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
        if (isFuncParam || isStruct[depth]) return hash->name;
        if (ARRAY == hash->type->kind) {
            Operand* size = initOP(OP_INT);
            size->u.i = getSize(hash->type);
            Operand* arr = initOP(OP_VARIABLE);
            arr->u.name = hash->name;
            InterCode* dec_arr = initInterCode(CODE_DECOP, arr, size);
            insertCode(now, dec_arr);
        }
        return hash->name;
    } else {
        Type* arr = (Type*)malloc(sizeof(Type));
        arr->kind = ARRAY;
        arr->u.array.size = node->children[2]->data.i;
        arr->u.array.elem = hash->type;
        if (hash->type->kind != ARRAY) {
            arr->u.array.dim = 1;
            arr->u.array.space = getSize(arr->u.array.elem);
        } else {
            arr->u.array.dim = hash->type->u.array.dim + 1;
            arr->u.array.space = hash->type->u.array.size * hash->type->u.array.space;
        }
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

    Operand* func = initOP(OP_FUNCTION);
    func->u.name = node->children[0]->data.str;
    InterCode* code = initInterCode(CODE_FUNCTION, func);
    insertCode(now, code);

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

    Operand* param = initOP(OP_VARIABLE);
    param->u.name = var_name;
    InterCode* code = initInterCode(CODE_PARAM, param);
    insertCode(now, code);

    insertSymbol(hash);
    hash = NULL;
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
        insertCode(now, code);
    } else if (3 == node->prod_id || 4 == node->prod_id) {  // IF ( Exp ) Stmt [ELSE Stmt]
        Operand* L1 = initLabel();
        InterCode* ifgoto = NULL;
        if (3 == node->children[2]->prod_id) {
            int rel = getRelop(node->children[2]->children[1]);
            Operand* op1 = Exp(node->children[2]->children[0]);
            Operand* op2 = Exp(node->children[2]->children[2]);
            Operand* relop = initOP(OP_RELOP);
            relop->u.rel = revRel(rel);
            ifgoto = initInterCode(CODE_IFGOTO, op1, relop, op2, L1);
        } else {
            Operand* op1 = Exp(node->children[2]);
            // FALSE
            Operand* op2 = initOP(OP_INT);
            op2->u.i = 0;
            Operand* relop = initOP(OP_RELOP);
            relop->u.rel = EQ;
            ifgoto = initInterCode(CODE_IFGOTO, op1, relop, op2, L1);
        }
        insertCode(now, ifgoto);
        Stmt(node->children[4], return_type);
        Operand* L2 = initLabel();
        if (4 == node->prod_id) {
            InterCode* jump_else = initInterCode(CODE_GOTO, L2);
            insertCode(now, jump_else);
        }
        InterCode* L1_code = initInterCode(CODE_LABEL, L1);
        insertCode(now, L1_code);
        if (4 == node->prod_id) {
            Stmt(node->children[6], return_type);
            InterCode* L2_code = initInterCode(CODE_LABEL, L2);
            insertCode(now, L2_code);
        }
    } else if (5 == node->prod_id) {  // WHILE ( Exp ) Stmt
        Operand* L_start = initLabel();
        InterCode* start = initInterCode(CODE_LABEL, L_start);
        insertCode(now, start);
        Operand* L_end = initLabel();
        InterCode* ifgoto = NULL;
        if (3 == node->children[2]->prod_id) {
            int rel = getRelop(node);
            Operand* op1 = Exp(node->children[2]->children[0]);
            Operand* op2 = Exp(node->children[2]->children[2]);
            Operand* relop = initOP(OP_RELOP);
            relop->u.rel = revRel(rel);
            ifgoto = initInterCode(CODE_IFGOTO, op1, relop, op2, L_end);
        } else {
            Operand* op1 = Exp(node->children[2]);
            // FALSE
            Operand* op2 = initOP(OP_INT);
            op2->u.i = 0;
            Operand* relop = initOP(OP_RELOP);
            relop->u.rel = EQ;
            ifgoto = initInterCode(CODE_IFGOTO, op1, relop, op2, L_end);
        }
        insertCode(now, ifgoto);
        Stmt(node->children[4], return_type);
        InterCode* jump_start = initInterCode(CODE_GOTO, L_start);
        insertCode(now, jump_start);
        InterCode* end = initInterCode(CODE_LABEL, L_end);
        insertCode(now, end);
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
    if (NULL == type) return;
    DecList(type, node->children[1]);
    return;
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
    return;
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
        Operand* op1 = initOP(OP_VARIABLE);
        op1->u.name = var_name;
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
        Operand* op = initOP(OP_INT);
        op->u.i = node->data.i;
        return op;
    } else if (17 == node->prod_id) {  // FLOAT
        Operand* op = initOP(OP_FLOAT);
        op->u.f = node->data.f;
        return op;
    } else if (15 == node->prod_id) {  // ID
        char* id_name = node->children[0]->data.str;
        HashNode* id = findSymbol(id_name);
        // TODO :涉及到将所有变量全变为临时变量
        Operand* op = initOP(OP_VARIABLE);
        op->u.name = id->name;
        return op;
    } else if (11 == node->prod_id || 12 == node->prod_id) {  // 函数
        char* id_name = node->children[0]->data.str;
        HashNode* id = findSymbol(id_name);
        Operand* op = initOP(OP_FUNCTION);
        op->u.name = id->name;
        if (11 == node->prod_id) {  // ID ( Args )
            InterCode* param = prepareParam(node->children[2]);
            insertCode(now, param);
        }
        return op;
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
                                       // TODO: 数组如何处理！
        Operand* arr = Exp(node->children[0]);
        Operand* index = Exp(node->children[2]);
        if (arr->kind != OP_VARIABLE) {
            arrDim[arr_id_ptr++] = index->u.i;
            Exp(node->children[0]);
            return index;
        } else {
            char* arr_name = arr->u.name;
            HashNode* hash = findSymbol(arr_name);
            Type* arr_dim = hash->type;
            int pos = 0;
            while (arr_id_ptr != 0) {
                --arr_id_ptr;
                pos += arrDim[arr_id_ptr] * arr_dim->u.array.space;
                arr_dim = arr_dim->u.array.elem;
            }
            Operand* temp = initTempVar();
            Operand* bias = initOP(OP_INT);
            bias->u.i = pos;
            InterCode* code = initInterCode(CODE_ADD, temp, arr, bias);
            insertCode(now, code);
        }
    } else if (14 == node->prod_id) {  // Exp . ID
        Type* st = Exp(node->children[0]);
        char* id_name = node->children[2]->data.str;
        int bias = findFieldPos(st, id_name);
        Operand* temp = initTempVar();
        Operand* struct_name = Exp(node->children[0]);
        Operand* bias_op = initOP(OP_INT);
        bias_op->u.i = bias;
        InterCode* code = initInterCode(CODE_ADD, temp, struct_name, bias_op);
        insertCode(now, code);
        return temp;
    } else if (0 == node->prod_id) {  // Exp = Exp

        Operand* l = Exp(node->children[0]);
        Operand* r = Exp(node->children[2]);
        InterCode* code = initInterCode(CODE_ASSIGN, l, r);
        insertCode(now, code);
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
        insertCode(now, math);
    }
}

// 将Args倒着输出
/*
Args : Exp "," Args
     : Exp
*/

InterCode* prepareParam(Node* node) { return NULL; }

void printCode(InterCode* code)
{
    int k = code->kind;
    if (CODE_ASSIGN == k) {
        printOp(code->u.assign.left);
        printf(" := ");
        if (OP_FUNCTION == code->u.assign.right->kind) printf("CALL ");
        printOp(code->u.assign.right);
    } else if (CODE_ADD == k || CODE_SUB == k || CODE_MUL == k || CODE_DIV == k) {
        printOp(code->u.binop.result);
        printf(" := ");
        printOp(code->u.binop.op1);
        if (CODE_ADD == k)
            printf(" + ");
        else if (CODE_SUB == k)
            printf(" - ");
        else if (CODE_MUL == k)
            printf(" * ");
        else if (CODE_DIV == k)
            printf(" / ");
        else
            printf("Wrong BinOP!");
        printOp(code->u.binop.op2);
    } else if (CODE_LABEL == k || CODE_FUNCTION == k) {
        if (CODE_LABEL == k)
            printf("LABEL ");
        else if (CODE_FUNCTION == k)
            printf("FUNCTION ");
        printOp(code->u.monop.op);
        printf(" :");
    } else if (CODE_GOTO == k || CODE_RETURN == k || CODE_ARG == k || CODE_PARAM == k || CODE_READ == k ||
               CODE_WRITE == k) {
        if (CODE_GOTO == k)
            printf("GOTO ");
        else if (CODE_RETURN == k)
            printf("RETURN ");
        else if (CODE_ARG == k)
            printf("ARG ");
        else if (CODE_PARAM == k)
            printf("PARAM ");
        else if (CODE_READ == k)
            printf("READ ");
        else if (CODE_WRITE == k)
            printf("WRITE ");
        else
            printf("Wrong MonOP!");
        printOp(code->u.monop.op);
    } else if (CODE_IFGOTO == k) {
        printf("IF ");
        printOp(code->u.ifgoto.op1);
        printf(" ");
        printOp(code->u.ifgoto.relop);
        printf(" ");
        printOp(code->u.ifgoto.op2);
        printf(" GOTO ");
        printOp(code->u.ifgoto.dest);
    } else if (CODE_DECOP == k) {
        printf("DEC ");
        printOp(code->u.decop.op);
        printf(" ");
        printf("%d", code->u.decop.size->u.i);
    }
    printf("\n");
}

void printOp(Operand* op)
{
    switch (op->kind) {
    case OP_VARIABLE:
    case OP_FUNCTION:
        printf("%s", op->u.name);
        break;
    case OP_INT:
        printf("#%d", op->u.i);
        break;
    case OP_FLOAT:
        printf("#%f", op->u.f);
        break;
    case OP_ADDRESS:
        printf("&%s", op->u.name);
        break;
    case OP_POINTER:
        printf("*%s", op->u.name);
        break;
    case OP_TEMP_VAR:
        printf("t%d", op->u.id);
        break;
    case OP_LABEL:
        printf("L%d", op->u.id);
        break;
    case OP_RELOP:
        switch (op->u.rel) {
        case EQ:
            printf("==");
            break;
        case LT:
            printf("<");
            break;
        case GT:
            printf(">");
            break;
        case NE:
            printf("!=");
            break;
        case GE:
            printf(">=");
            break;
        case LE:
            printf("<=");
            break;
        default:
            printf("Wrong Relop!");
            break;
        }
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
    if (brk == now) now = end;
}

void initHashTable()
{
    static char read_name[10] = "read";
    static char write_name[10] = "write";
    Type* read_type = (Type*)malloc(sizeof(Type));
    Type* write_type = (Type*)malloc(sizeof(Type));
    read_type->kind = FUNCTION;
    read_type->u.function.return_type = &Type_int;
    read_type->u.function.params = NULL;
    read_type->u.function.status = DEF;
    write_type->kind = FUNCTION;
    write_type->u.function.return_type = &Type_int;
    write_type->u.function.params = &Type_int;
    write_type->u.function.status = DEF;
    HashNode* read_node = initSymbol(read_name, read_type, 0);
    HashNode* write_node = initSymbol(write_name, write_type, 0);
    insertSymbol(read_node);
    insertSymbol(write_name);
}

int getSize(Type* type)
{
    if (BASIC == type->kind)
        return 4;
    else if (ARRAY == type->kind)
        return type->u.array.space;
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
        printf("Wrong RELOP!");
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
        printf("Wrong RELOP!\n");
        break;
    }
    return &op_false;
}

Operand* initLabel()
{
    Operand* label = initOP(OP_LABEL);
    label->u.id = label_id++;
    return label;
}

Operand* initTempVar()
{
    Operand* temp_var = initOP(OP_TEMP_VAR);
    temp_var->u.id = temp_var_id++;
    return temp_var;
}