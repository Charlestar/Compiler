// 本文件涉及许多指针操作，变量名等实际上是存储在语法树中的
// 逻辑类型报错视为操作符不匹配

#include "semantic.h"

#include <assert.h>

// 用于在分析时提供靶子，避免额外开辟空间来表示基本类型
Type Type_int = {.kind = BASIC, .u.basic = INT};
Type Type_float = {.kind = BASIC, .u.basic = FLOAT};

// 指示当前处理的是不是结构体，当为TRUE时，Dec中不允许出现等号
// TODO isStruct全局的使用存在问题，如果在VarList中，仅在isStruct情况下进行域转换，会导致嵌套在内层的结构体无法被定义。
int isStruct = FALSE;

FuncRecord* func_head = NULL;

/*
enum {
    UNDEFINED_VAR = 1,  // 1变量在使用时未定义
    UNDEFINED_FUNC,     // 2函数在调用时未定义
    REDEFINED_VAR,      // 3变量重复定义，或与前面定义过的结构体名重复
    REDEFINED_FUNC,     // 4函数重复定义
    ASSIGN_TYPE_MISS,   // 5赋值号两边类型不匹配
    ASSIGN_LEFT_MISS,   // 6赋值号左边出现只有右值的表达式
    OP_TYPE_MISS,       // 7操作数类型不匹配，或操作数类型与操作符不匹配
    RETURN_TYPE_MISS,   // 8return语句返回值与函数定义的返回类型不匹配
    FUNC_PARAM_MISS,    // 9函数调用时实参与形参不匹配
    NOT_ARR,            // 10对非数组变量使用[]
    NOT_FUNC,           // 11对非函数变量使用()
    ARR_ACCESS_ERR,     // 12数组访问操作符中出现非整数
    NOT_STRUCT,         // 13对非结构体变量使用.
    STRUCT_FIELD_MISS,  // 14访问结构体中未定义的域
    STRUCT_FIELD_ERR,   // 15同一结构体域名重复定义，或定义时对域初始化
    REDEFINED_STRUCT,   // 16结构体名域前面定义的变量重复
    UNDEFINED_STRUCT,   // 17使用未定义过的结构体
    FUNC_DEC_NO_DEF,    // 18函数进行了声明但没有被定义
    FUNC_DEC_MISS       // 函数的多次声明相互冲突或与定义相互冲突
} semantic_error_code;
*/

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
        addFuncRecord(func->name, node->line, TRUE);
        if (TRUE == checkAllow(func, node->line)) {
            insertSymbol(func);
        }
        CompSt(node->children[2], func->type->u.function.return_type);
    } else if (3 == node->prod_id) {  // Specifier FunDec ";"
        HashNode* func = FunDec(type, node->children[1]);
        // 因为函数的参数还存在depth+1的栈上，对于函数声明，要清空这些内容
        delField(depth + 1);
        if (NULL == func) return;
        func->type->u.function.status = DEC;
        addFuncRecord(func->name, node->line, FALSE);
        if (TRUE == checkAllow(func, node->line)) {
            insertSymbol(func);
        }
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
        VarDec(hash, ext_dec_list->children[0]);
        if (TRUE == checkAllow(hash, node->line)) {
            insertSymbol(hash);
        } else {
            free(hash);
        }
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

// TODO 所有结构体都是全局的
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
        isStruct = TRUE;
        structure->next = DefList(node->children[3]);
        isStruct = FALSE;
        pop();
        if (NULL != node->children[1]) {
            structure->name = node->children[1]->children[0]->data.str;
            if (NULL != findSymbol(structure->name)) {
                errorHandler(REDEFINED_STRUCT, node->line, structure->name);
                return NULL;
            } else {
                // 这里depth为0，即默认结构体名都是全局的
                HashNode* struct_type = initSymbol(structure->name, type, 0);
                if (TRUE == DEBUG) printf("%s\n", struct_type->name);
                insertSymbol(struct_type);
            }
        }
        // 对于匿名结构体，因为之后不可能再使用了，所以不需要插入符号表
        return type;
    } else {  // STRUCT Tag
        char* name = node->children[1]->children[0]->data.str;
        HashNode* struct_type = findSymbol(name);
        if (NULL == struct_type || STRUCTURE != struct_type->type->kind) {
            errorHandler(UNDEFINED_STRUCT, node->line, name);
            return NULL;
        }
        return struct_type->type;
    }
}

/*
VarDec : ID                 0
       | VarDec [ INT ]     1
*/
void VarDec(HashNode* hash, Node* node)
{
    if (TRUE == DEBUG) printf("VarDec\n");
    if (node->prod_id == 0) {
        hash->name = node->children[0]->data.str;
    } else {
        Type* arr = (Type*)malloc(sizeof(Type));
        arr->kind = ARRAY;
        arr->u.array.size = node->children[2]->data.i;
        arr->u.array.elem = hash->type;
        if (hash->type->kind != ARRAY) {
            arr->u.array.dim = 1;
        } else {
            arr->u.array.dim = hash->type->u.array.dim + 1;
        }
        hash->type = arr;
        VarDec(hash, node->children[0]);
    }
    return;
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
    if (1 == node->prod_id) {  // ID ( )
        type->u.function.params = NULL;
    } else {  // ID ( VarList )
        depth += 1;
        type->u.function.params = VarList(node->children[2]);
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
    if (checkAllow(hash, node->line) == TRUE) {
        insertSymbol(hash);
    } else {
        free(hash);
    }
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
    if (TRUE == DEBUG) printf("exit StmtList\n");
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
        if (NULL == return_type) {
            printf("This RETURN statement shouldn't be here.\n");
            return;
        }
        Type* type = Exp(node->children[1]);
        if (checkType(type, return_type) == FALSE) {
            errorHandler(RETURN_TYPE_MISS, node->line, NULL);
            return;
        }
    } else if (3 == node->prod_id) {  // IF ( Exp ) Stmt
        Type* type = Exp(node->children[2]);
        if (NULL == type || type->kind != BASIC || type->u.basic != INT)
            // 逻辑类型报错
            errorHandler(OP_TYPE_MISS, node->children[2]->line, NULL);
        Stmt(node->children[4], return_type);
    } else if (4 == node->prod_id) {  // IF ( Exp ) Stmt ELSE Stmt
        Type* type = Exp(node->children[2]);
        if (NULL == type || type->kind != BASIC || type->u.basic != INT)
            //  逻辑类型报错
            errorHandler(OP_TYPE_MISS, node->children[2]->line, NULL);
        Stmt(node->children[4], return_type);
        Stmt(node->children[6], return_type);
    } else if (5 == node->prod_id) {  // WHILE ( Exp ) Stmt
        Type* type = Exp(node->children[2]);
        if (NULL == type || type->kind != BASIC || type->u.basic != INT)
            //  逻辑类型报错
            errorHandler(OP_TYPE_MISS, node->children[2]->line, NULL);
        Stmt(node->children[4], return_type);
    } else {
        errorHandler(UNHANDLED, node->line, "Wrong Stmt prod_id");
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
    return conv2FieldList(depth);
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
    VarDec(hash, node->children[0]);

    // try to insert to hashtable
    if (checkAllow(hash, node->line) == TRUE) {
        insertSymbol(hash);
        if (1 == node->prod_id) {
            if (TRUE == isStruct) {
                errorHandler(STRUCT_FIELD_ERR, node->line, hash->name);
            } else {
                // check "="
                Type* exp_type = Exp(node->children[2]);
                if (FALSE == checkType(hash->type, exp_type)) {
                    errorHandler(ASSIGN_TYPE_MISS, node->line, NULL);
                }
            }
        }
    } else {
        free(hash);
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

Type* Exp(Node* node)
{
    if (TRUE == DEBUG) printf("Exp\n");
    if (NULL == node) return NULL;
    if (16 == node->prod_id) {  // INT
        return &Type_int;
    } else if (17 == node->prod_id) {  // FLOAT
        return &Type_float;
    } else if (15 == node->prod_id) {  // ID
        char* id_name = node->children[0]->data.str;
        HashNode* id = findSymbol(id_name);
        if (NULL == id) {
            errorHandler(UNDEFINED_VAR, node->line, id_name);
            return NULL;
        }
        // 因为Exp是嵌套的，所以这里类型是arr或struct或basic都可以，但绝不是func
        if (id->type->kind == FUNCTION) {
            errorHandler(UNDEFINED_VAR, node->line, id_name);
            return NULL;
        }
        return id->type;
    } else if (11 == node->prod_id || 12 == node->prod_id) {  // 函数
        char* id_name = node->children[0]->data.str;
        HashNode* id = findSymbol(id_name);
        if (NULL == id) {
            errorHandler(UNDEFINED_FUNC, node->line, id_name);
            return NULL;
        }
        if (FUNCTION != id->type->kind) {
            errorHandler(NOT_FUNC, node->line, id_name);
            return NULL;
        }
        if (11 == node->prod_id) {  // ID ( Args )
            if (checkFuncCall(id, node->children[2]) == FALSE) {
                errorHandler(FUNC_PARAM_MISS, node->line, id_name);
                return NULL;
            }
        }
        return id->type->u.function.return_type;
    } else if (8 == node->prod_id) {  // ( Exp )
        return Exp(node->children[1]);
    } else if (9 == node->prod_id) {  //- Exp
        return Exp(node->children[1]);
    } else if (10 == node->prod_id) {  // "!" Exp
        Type* temp = Exp(node->children[1]);
        if (NULL == temp || temp->kind != BASIC || temp->u.basic != INT) {
            // 逻辑类型不匹配
            errorHandler(OP_TYPE_MISS, node->children[1]->line, NULL);
            temp = NULL;
        }
        return temp;
    } else if (13 == node->prod_id) {  // Exp [ Exp ]
        Type* arr = Exp(node->children[0]);
        Type* index = Exp(node->children[2]);
        if (NULL == arr || NULL == index) return NULL;
        if (arr->kind != ARRAY) {
            Node* find_id = node->children[0];
            while (find_id->type != TYPE_ID) find_id = find_id->children[0];
            errorHandler(NOT_ARR, node->line, find_id->data.str);
            return NULL;
        }
        if (index->kind != BASIC || index->u.basic != INT) {
            char buf[64];
            // TODO 这里的处理只针对中括号中出现单独的FLOAT时报错
            snprintf(buf, 64, "%f", node->children[2]->children[0]->data.f);
            errorHandler(ARR_ACCESS_ERR, node->line, buf);
            // ! 这里让它正常返回，防止多报错误
            // return NULL;
        }
        // TODO 数组越界检查
        return arr->u.array.elem;
    } else if (14 == node->prod_id) {  // Exp . ID
        Type* st = Exp(node->children[0]);
        if (st == NULL) {
            return NULL;
        }
        if (st->kind != STRUCTURE) {
            errorHandler(NOT_STRUCT, node->line, NULL);
            st = NULL;
        } else {
            char* id_name = node->children[2]->data.str;
            st = findFieldID(st, id_name);
            if (st == NULL) {
                errorHandler(STRUCT_FIELD_MISS, node->line, id_name);
            }
        }
        return st;
    } else if (0 == node->prod_id) {  // Exp = Exp
        // 左值检查，只有ID, Exp [ Exp ], Exp . ID可作为左值
        Node* left = node->children[0];
        if (15 != left->prod_id && 13 != left->prod_id && 14 != left->prod_id) {
            errorHandler(ASSIGN_LEFT_MISS, node->line, NULL);
            return NULL;
        }
        // 等于号类型检查
        if (checkType(Exp(node->children[0]), Exp(node->children[2])) == FALSE) {
            errorHandler(ASSIGN_TYPE_MISS, node->line, NULL);
            return NULL;
        } else {  //! 等于号应该返回什么？
            return &Type_int;
        }
    } else if (1 == node->prod_id || 2 == node->prod_id) {  // Exp && Exp, Exp || Exp
        Type* l = Exp(node->children[0]);
        Type* r = Exp(node->children[2]);
        if (NULL == l || NULL == r) return NULL;
        if (l->kind != BASIC || l->u.basic != INT || r->kind != BASIC || r->u.basic != INT) {
            // 非逻辑类型报错
            errorHandler(OP_TYPE_MISS, node->children[2]->line, NULL);
            return NULL;
        }
        return &Type_int;
    } else if (3 == node->prod_id) {  // Exp RELOP Exp
        Type* l = Exp(node->children[0]);
        Type* r = Exp(node->children[2]);
        if (NULL == l || NULL == r) return NULL;
        if (l->kind != BASIC || r->kind != BASIC) return NULL;
        return &Type_int;
    } else {  // Exp +-*/ Exp
        // 如果运算符不匹配，只进行报错，并返回左边表达式的类型
        Type* l = Exp(node->children[0]);
        Type* r = Exp(node->children[2]);
        if (NULL == l && NULL == r) {
            errorHandler(OP_TYPE_MISS, node->line, NULL);
            return NULL;
        } else if (NULL == l) {
            errorHandler(OP_TYPE_MISS, node->line, NULL);
            return r;
        } else if (NULL == r) {
            errorHandler(OP_TYPE_MISS, node->line, NULL);
            return l;
        } else {
            if (l->kind != BASIC || r->kind != BASIC || l->u.basic != r->u.basic) {
                errorHandler(OP_TYPE_MISS, node->line, NULL);
            }
            return l;
        }
    }
}

void analyseSemantic(Node* node)
{
    func_head = initFuncRecord(NULL, 0, FALSE);
    Program(node);
    checkFuncDEF();
}

void errorHandler(int error_code, int line, char* msg)
{
    printf("Error type %d at Line %d: ", error_code, line);

    switch (error_code) {
    case UNDEFINED_VAR:
        printf("Undefined variable \"%s\".\n", msg);
        break;
    case UNDEFINED_FUNC:
        printf("Undefined function \"%s\".\n", msg);
        break;
    case REDEFINED_VAR:
        printf("Redefined variable \"%s\".\n", msg);
        break;
    case REDEFINED_FUNC:
        printf("Redefined function \"%s\".\n", msg);
        break;
    case ASSIGN_TYPE_MISS:
        printf("Type mismatched for assignment.\n");
        break;
    case ASSIGN_LEFT_MISS:
        printf("The left-hand side of an assignment must be a variable.\n");
        break;
    case OP_TYPE_MISS:
        printf("Type mismatched for operands.\n");
        break;
    case RETURN_TYPE_MISS:
        printf("Type mismatched for return.\n");
        break;
    case FUNC_PARAM_MISS:
        // TODO 和样例输出仍有差距
        printf("Function \"%s\" is not applicable for arguments you gave.\n", msg);
        break;
    case NOT_ARR:
        printf("\"%s\" is not an array.\n", msg);
        break;
    case NOT_FUNC:
        printf("\"%s\" is not a function.\n", msg);
        break;
    case ARR_ACCESS_ERR:
        printf("\"%s\" is not an integer.\n", msg);
        break;
    case NOT_STRUCT:
        printf("Illegal use of \".\".\n");
        break;
    case STRUCT_FIELD_MISS:
        printf("Non-existent field \"%s\".\n", msg);
        break;
    case STRUCT_FIELD_ERR:
        printf("Incorrectly defined field \"%s\".\n", msg);
        break;
    case REDEFINED_STRUCT:
        printf("Duplicated name \"%s\".\n", msg);
        break;
    case UNDEFINED_STRUCT:
        printf("Undefined structure \"%s\".\n", msg);
        break;
    case FUNC_DEC_NO_DEF:
        printf("Function \"%s\" declared but not defined.\n", msg);
        break;
    case FUNC_DEC_MISS:
        printf("Inconsistent declaration of function \"%s\".\n", msg);
        break;
    default:
        printf("Unhandled Error \"%s\".\n", msg);
        break;
    }
}

// 查看结构体的域，输出名字为name的域的类型
// 输入为结构体的首个FieldList，相当于链表头
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

/*
Args : Exp "," Args
     | Exp
*/
// 逐个比较参数情况，如果匹配返回true，否则返回false
int checkFuncCall(HashNode* hashnode, Node* args)
{
    if (TRUE == DEBUG) printf("checking func call ...\n");
    if (NULL == hashnode) return FALSE;
    if (hashnode->type->kind != FUNCTION) return FALSE;
    FieldList* field = hashnode->type->u.function.params;
    Type* arg_type = NULL;
    Node* arg = args;
    while (arg != NULL) {
        if (NULL == field) return FALSE;
        arg_type = Exp(arg->children[0]);
        if (arg_type->kind != field->type->kind) return FALSE;
        if (arg_type->kind == BASIC && arg_type->u.basic != field->type->u.basic) return FALSE;
        field = field->next;
        arg = arg->children[2];
    }
    if (NULL != field) return FALSE;
    return TRUE;
}

// 检查类型是否匹配，用于等号或域匹配等
int checkType(Type* l, Type* r)
{
    if (DEBUG == TRUE) printf("checking type ...\n");
    if (NULL == l || NULL == r) return FALSE;
    // 加快效率的大杀器！
    if (l == r) return TRUE;
    if (l->kind != r->kind) return FALSE;
    if (BASIC == l->kind) {
        return l->u.basic == r->u.basic ? TRUE : FALSE;
    } else if (ARRAY == l->kind) {
        if (l->u.array.dim != r->u.array.dim) return FALSE;
        Type *templ = l, *tempr = r;
        while (templ->kind == ARRAY) templ = templ->u.array.elem;
        while (tempr->kind == ARRAY) tempr = tempr->u.array.elem;
        // 数组可以有多种元素选择
        return checkType(templ, tempr);
    } else if (STRUCTURE == l->kind) {
        return checkField(l->u.structure->next, r->u.structure->next);
    } else {  // FUNCTION
        return FALSE;
    }
}

// 检查域是否等价，用于函数的参数列表和结构体域
// 域中不允许含有函数类型
// 域中内容是有序的，这里我用的是按序逐个比较
int checkField(FieldList* field1, FieldList* field2)
{
    if (TRUE == DEBUG) printf("checking field ...\n");
    FieldList* temp1 = field1;
    FieldList* temp2 = field2;
    while (temp1 != NULL && temp2 != NULL) {
        if (checkType(temp1->type, temp2->type) == FALSE) {
            break;
        }
        temp1 = temp1->next;
        temp2 = temp2->next;
    }
    if (temp1 != NULL || temp2 != NULL) return FALSE;
    return TRUE;
}

FuncRecord* initFuncRecord(char* name, int line, int defined)
{
    FuncRecord* record = (FuncRecord*)malloc(sizeof(FuncRecord));
    record->name = name;
    record->line = line;
    record->defined = defined;
    record->next = NULL;
    return record;
}

void addFuncRecord(char* name, int line, int defined)
{
    FuncRecord* add = func_head;
    while (add->next != NULL) {
        add = add->next;
        if (strcmp(add->name, name) == 0 && add->defined == FALSE) {
            // 不更新行数，多次声明以第一次为准
            add->defined = defined;
            return;
        }
    }
    add->next = initFuncRecord(name, line, defined);
    return;
}

// 检查错误类型18：函数进行了声明但没有被定义
int checkFuncDEF()
{
    FuncRecord* check = func_head;
    int OK = TRUE;
    while (check->next != NULL) {
        check = check->next;
        if (check->defined == FALSE) {
            errorHandler(FUNC_DEC_NO_DEF, check->line, check->name);
            OK = FALSE;
        }
    }
    return OK;
}

// 检查两个节点是否合法，包括同名检查和函数合法性检查
int checkAllow(HashNode* hashnode, int line)
{
    if (TRUE == DEBUG) printf("checking allow ...\n");
    HashNode* exist = findSymbol(hashnode->name);
    // 这个符号尚未插入到表中
    if (NULL == exist) return TRUE;
    // 表中有但并没有在同一深度
    if (hashnode->depth != exist->depth) {
        if (hashnode->type->kind == FUNCTION) {
            errorHandler(REDEFINED_FUNC, line, hashnode->name);
            return FALSE;
        } else if (hashnode->type->kind == STRUCTURE &&
                   strcmp(hashnode->name, hashnode->type->u.structure->name) ==
                       0) {  // 只在结构体定义上进行判断，而结构体定义指的是哈希表名和结构体名相同。
            errorHandler(REDEFINED_STRUCT, line, hashnode->name);
            return FALSE;
        } else {
            return TRUE;
        }
    } else {  // 如果深度相同
        if ((hashnode->type->kind == FUNCTION) && (exist->type->kind == FUNCTION)) {
            if (hashnode->type->u.function.status == DEF && exist->type->u.function.status == DEF) {
                errorHandler(REDEFINED_FUNC, line, hashnode->name);
                return FALSE;
            }
            if (hashnode->type->u.function.return_type != exist->type->u.function.return_type) {
                errorHandler(FUNC_DEC_MISS, line, hashnode->name);
                return FALSE;
            }
            if (checkField(hashnode->type->u.function.params, exist->type->u.function.params) == FALSE) {
                errorHandler(FUNC_DEC_MISS, line, hashnode->name);
                return FALSE;
            }
            // 里面已经有DEC或DEF，这是不必再插入符号表
            if (hashnode->type->u.function.status == DEC) {
                return FALSE;
            } else {
                delSymbol(exist);
                return TRUE;
            }
        } else {
            int err_code = 0;
            if (hashnode->type->kind == STRUCTURE) {
                err_code = isStruct == TRUE ? STRUCT_FIELD_ERR : REDEFINED_STRUCT;
            } else if (hashnode->type->kind == FUNCTION) {
                err_code = REDEFINED_FUNC;
            } else {
                err_code = isStruct == TRUE ? STRUCT_FIELD_ERR : REDEFINED_VAR;
            }
            errorHandler(err_code, line, hashnode->name);
            return FALSE;
        }
    }
}