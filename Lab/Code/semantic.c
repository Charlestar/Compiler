// 本文件涉及许多指针操作，变量名等实际上是存储在语法树中的

#include "semantic.h"

#include <assert.h>

// 用于在分析时提供靶子，避免额外开辟空间来表示基本类型
Type Type_int = {.kind = BASIC, .u.basic = INT};
Type Type_float = {.kind = BASIC, .u.basic = FLOAT};

// 指示当前处理的是不是结构体，当为TRUE时，Dec中不允许出现等号
int isStruct = FALSE;

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
void Program(Node* node) { ExtDefList(node); }

/*
ExtDefList : ExtDef ExtDefList
           | %empty
*/
void ExtDefList(Node* node)
{
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
    Type* type = Specifier(node->children[0]);
    if (2 == node->prod_id) {
        HashNode* func = FunDec(type, node->children[1]);
        func->type->u.function.status = DEF;
    } else if (3 == node->prod_id) {
        HashNode* func = FunDec(type, node->children[1]);
        func->type->u.function.status = DEC;
        // 因为函数的参数还存在depth+1的栈上，对于函数声明，要清空这些内容
        delField(depth + 1);
    } else if (1 == node->prod_id) {
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
    if (strcmp(node->data.str, "Specifier") != 0) assert(0);
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
    if (strcmp(node->data.str, "StructSpecifier") != 0) assert(0);
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
                // TODO 这里depth要默认为0，但同时应修改insertSymbol()函数
                HashNode* struct_type = initSymbol(structure->name, type, depth);
                insertSymbol(struct_type);
            }
        }
        return type;
    } else {  // STRUCT Tag
        char* name = node->children[1]->children[0]->data.str;
        HashNode* struct_type = findSymbol(name);
        // ! 如果上面depth设置为0，则这里要进行全局搜索。
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
    if (strcmp(node->data.str, "VarDec") != 0) assert(0);
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
// 交给上层来插入，因为不清楚是定义还是声明
// 函数和结构体不同，函数的params链表第一个就是参数，而不是函数名代表的链表头
/*
FunDec : ID ( VarList )
       | ID ( )
*/
HashNode* FunDec(Type* return_type, Node* fun_dec)
{
    Type* type = (Type*)malloc(sizeof(Type));
    type->kind = FUNCTION;
    type->u.function.return_type = return_type;
    HashNode* hash = initSymbol(fun_dec->children[0]->data.str, type, depth);
    if (fun_dec->prod_id == 1) {
        type->u.function.params = NULL;
    } else {
        depth += 1;
        type->u.function.params = VarList(fun_dec->children[2]);
        depth -= 1;  // 让CompSt的操作更加统一
    }
    return hash;
}

/*
VarList : ParamDec , VarList
        | ParamDec
*/
FieldList* VarList(Node* node)
{
    if (strcmp(node->data.str, "VarList") != 0) assert(0);
    Node* var_list = node;
    while (var_list != NULL) {
        ParamDec(var_list->children[0]);
        var_list = var_list->children[2];
    }
    var_list = NULL;
    return conv2FieldList(depth);
}

/*
ParamDec : Specifier VarDec
*/
void ParamDec(Node* param_dec)
{
    Type* type = Specifier(param_dec->children[0]);
    HashNode* hash = initSymbol(NULL, type, depth);
    VarDec(hash, param_dec->children[1]);
    if (checkAllow(hash, param_dec->line) == TRUE) {
        insertSymbol(hash);
    } else {
        return;
    }
}

/*
CompSt : { DefList StmtList }
*/
void CompSt(Node* node, Type* return_type)
{
    push();
    DefList(node->children[1]);
    StmtList(node->children[2], return_type);
    pop();
}

/*
StmtList : Stmt StmtList
         | %empty
*/
void StmtList(Node* node, Type* return_type)
{
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
    if (0 == node->prod_id) {
        Exp(node->children[0]);
    } else if (1 == node->prod_id) {
        CompSt(node->children[0], return_type);
    } else if (2 == node->prod_id) {
        if (NULL == return_type) {
            printf("This RETURN statement shouldn't be here.\n");
            return;
        }
        Type* type = Exp(node->children[1]);
        if (checkType(type, return_type) == FALSE) {
            errorHandler(RETURN_TYPE_MISS, node->line, NULL);
            return;
        }
    } else if (3 == node->prod_id) {
        Type* type = Exp(node->children[2]);
        if (type->kind != BASIC || type->u.basic != INT) {
            // TODO 逻辑类型报错
            return;
        }
        Stmt(node->children[4], return_type);
    } else if (4 == node->prod_id) {
        Type* type = Exp(node->children[2]);
        if (type->kind != BASIC || type->u.basic != INT) {
            // TODO 逻辑类型报错
            return;
        }
        Stmt(node->children[4], return_type);
        Stmt(node->children[6], return_type);
    } else if (5 == node->prod_id) {
        Type* type = Exp(node->children[2]);
        if (type->kind != BASIC || type->u.basic != INT) {
            // TODO 逻辑类型报错
            return;
        }
        Stmt(node->children[4], return_type);
    }
}

/*
DefList : Def DefList
        | %empty
*/
FieldList* DefList(Node* node)
{
    if (strcmp(node->data.str, "DefList") != 0) assert(0);
    Node* deflist = node;
    while (NULL != deflist) {
        Def(deflist->children[0]);
        deflist = deflist->children[1];
    }
    return conv2FieldList(depth);
}

/*
Def : Specifier DecList ;
*/
void Def(Node* node)
{
    if (strcmp(node->data.str, "Def") != 0) assert(0);
    Type* type = Specifier(node->children[0]);
    DecList(type, node->children[1]);
    return;
}

/*
DecList : Dec
        | Dec , DecList
*/
void DecList(Type* type, Node* node)
{
    if (strcmp(node->data.str, "DecList") != 0) assert(0);
    Node* declist = node;
    while (NULL != declist) {
        Dec(type, declist->children[0]);
        declist = declist->children[2];
    }
    return;
}

/*
Dec : VarDec            0
    | VarDec = Exp      1
*/
void Dec(Type* type, Node* node)
{
    if (strcmp(node->data.str, "Dec") != 0) assert(0);
    HashNode* hash = initSymbol(NULL, type, depth);
    VarDec(hash, node->children[0]);
    if (1 == node->prod_id) {
        if (TRUE == isStruct) {
            errorHandler(STRUCT_FIELD_ERR, node->line, hash->name);
            return;
        } else {
            // check "="
            Type* exp_type = Exp(node->children[2]);
            if (FALSE == checkType(hash->type, exp_type)) {
                errorHandler(ASSIGN_TYPE_MISS, node->line, NULL);
                return;
            }
        }
    }

    // try to insert to hashtable
    if (checkAllow(hash, node->line) == TRUE) {
        insertSymbol(hash);
    } else {
        return;
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
    if (strcmp(node->data.str, "Exp") != 0) assert(0);
    if (16 == node->prod_id) {  // INT
        return &Type_int;
    } else if (17 == node->prod_id) {  // FLOAT
        return &Type_float;
    } else if (node->children[0]->type == TYPE_ID) {
        char* id_name = node->children[0]->data.str;
        HashNode* id = findSymbol(id_name);
        if (15 == node->prod_id) {  // ID
            if (id == NULL) {
                errorHandler(UNDEFINED_VAR, node->line, id_name);
                return NULL;
            }
            // 因为Exp是嵌套的，所以这里类型是arr或struct或basic都可以，但绝不是func
            if (id->type->kind == FUNCTION) {
                errorHandler(REDEFINED_VAR, node->line, id_name);
                return NULL;
            }
            return id->type;
        } else {  // 函数
            if (id == NULL) {
                errorHandler(UNDEFINED_FUNC, node->line, id_name);
                return NULL;
            }
            if (node->prod_id == 11) {  // ID ( Args )
                if (checkFuncCall(id, node->children[2]) == FALSE) {
                    errorHandler(FUNC_PARAM_MISS, node->line, id_name);
                    return NULL;
                }
            }
            return id->type->u.function.return_type;
        }
    } else if (8 == node->prod_id) {  // ( Exp )
        return Exp(node->children[1]);
    } else if (9 == node->prod_id) {  //- Exp
        return Exp(node->children[1]);
    } else if (10 == node->prod_id) {  // "!" Exp
        Type* temp = Exp(node->children[1]);
        if (temp->kind != BASIC || temp->u.basic != INT) {
            // TODO 是否应该有报错，逻辑类型不匹配
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
            snprintf(buf, 64, "%f", node->children[2]->data.f);
            errorHandler(ARR_ACCESS_ERR, node->line, buf);
            return NULL;
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
            return NULL;
        } else {  //! 等于号应该返回什么？
            return &Type_int;
        }
    } else if (1 == node->prod_id || 2 == node->prod_id) {  // Exp && Exp, Exp || Exp
        Type* l = Exp(node->children[0]);
        Type* r = Exp(node->children[2]);
        if (NULL == l || NULL == r) return NULL;
        if (l->kind != BASIC || l->u.basic != INT || r->kind != BASIC || r->u.basic != INT) {
            // TODO 非逻辑类型报错
            return NULL;
        }
        return &Type_int;
    } else if (3 == node->prod_id) {  // Exp RELOP Exp
        Type* l = Exp(node->children[0]);
        Type* r = Exp(node->children[2]);
        if (l->kind != BASIC || r->kind != BASIC) return NULL;
        return &Type_int;
    } else {  // Exp +-*/ Exp
        Type* l = Exp(node->children[0]);
        Type* r = Exp(node->children[2]);
        if (l->kind != BASIC || r->kind != BASIC) return NULL;
        if (l->u.basic != r->u.basic) return NULL;
        return l;
    }
}

void analyseSemantic(Node* node) { Program(node); }

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
        printf("Incomplete definition of function \"%s\".\n", msg);
        break;
    case FUNC_DEC_MISS:
        printf("Function \"%s\" declared but not defined.\n", msg);
        break;
    default:
        printf("Unhandled Error!\n");
        break;
    }
}

// 查看结构体的域，输出名字为name的域的类型
// 输入为结构体的首个FieldList，相当于链表头
Type* findFieldID(Type* type, char* name)
{
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
int checkFuncCall(HashNode* func, Node* args)
{
    if (func->type->kind != FUNCTION) return FALSE;
    FieldList* field = func->type->u.function.params;
    Type* arg_type = NULL;
    while (args != NULL) {
        if (NULL == field) return FALSE;
        arg_type = Exp(args->children[0]);
        if (arg_type->kind != field->type->kind) return FALSE;
        if (arg_type->kind == BASIC && arg_type->u.basic != field->type->u.basic) return FALSE;
        field = field->next;
        args = args->children[2];
    }
    if (NULL != field) return FALSE;
    return TRUE;
}

// 检查类型是否匹配，用于等号或域匹配等
int checkType(Type* l, Type* r)
{
    if (l->kind != r->kind) return FALSE;
    if (BASIC == l->kind) {
        return l->u.basic == r->u.basic ? TRUE : FALSE;
    } else if (ARRAY == l->kind) {
        if (l->u.array.dim != r->u.array.dim) return FALSE;
        Type *templ = l, *tempr = r;
        while (templ->kind != BASIC) templ = templ->u.array.elem;
        while (tempr->kind != BASIC) tempr = tempr->u.array.elem;
        if (templ->u.basic != tempr->u.basic) return FALSE;
        return TRUE;
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
    FieldList* temp1 = field1;
    FieldList* temp2 = field2;
    int allow = TRUE;
    while (temp1 != NULL && temp2 != NULL) {
        if (checkType(temp1->type, temp2->type) == FALSE) {
            temp1 = NULL;
            temp2 = NULL;
            return FALSE;
        }
        temp1 = temp1->next;
        temp2 = temp2->next;
    }
    if (temp1 != NULL || temp2 != NULL) allow = FALSE;
    temp1 = NULL;
    temp2 = NULL;
    return allow;
}

// TODO 检查错误类型18：函数进行了声明但没有被定义
int checkFuncDEF() {}

// 检查两个节点是否合法，包括同名检查和函数合法性检查
int checkAllow(HashNode* node, int line)
{
    HashNode* exist = findSymbol(node->name);
    // 这个符号尚未插入到表中
    if (NULL == exist) return TRUE;
    // 表中有但并没有在同一深度
    if (node->depth != exist->depth) {
        if (node->type->kind == FUNCTION) {
            errorHandler(REDEFINED_FUNC, line, node->name);
            return FALSE;
        } else if (node->type->kind == STRUCTURE) {
            errorHandler(REDEFINED_STRUCT, line, node->name);
            return FALSE;
        } else {
            return TRUE;
        }
    } else {  // 如果深度相同
        if ((node->type->kind == FUNCTION) && (exist->type->kind == FUNCTION)) {
            if (node->type->u.function.status == DEF && exist->type->u.function.status == DEF) {
                errorHandler(REDEFINED_FUNC, line, node->name);
                return FALSE;
            }
            if (node->type->u.function.return_type != exist->type->u.function.return_type) {
                errorHandler(FUNC_DEC_MISS, line, node->name);
                return FALSE;
            }
            if (checkField(node->type->u.function.params, exist->type->u.function.params) == FALSE) {
                errorHandler(FUNC_DEC_MISS, line, node->name);
                return FALSE;
            }
            // 里面已经有DEC或DEF，这是不必再插入符号表
            if (node->type->u.function.status == DEC) {
                return FALSE;
            } else {
                delSymbol(exist);
                return TRUE;
            }
        } else {
            if (node->type->kind == STRUCTURE) {
                errorHandler(REDEFINED_STRUCT, line, node->name);
            } else if (node->type->kind == FUNCTION) {
                errorHandler(REDEFINED_FUNC, line, node->name);
            } else {
                errorHandler(REDEFINED_VAR, line, node->name);
            }
            return FALSE;
        }
    }
}