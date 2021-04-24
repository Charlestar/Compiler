// 本文件涉及许多指针操作，变量名等实际上是存储在语法树中的

#include "semantic.h"

#include <assert.h>

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
ExtDef 	: Specifier ExtDecList ";"
        | Specifier ";"
        | Specifier FunDec CompSt
        | Specifier FunDec ";"
*/
HashNode* buildExtDef(Node* node) { Type* type = Specifier(node->children[0]); }

/*
Specifier : TYPE
          | StructSpecifier
*/
Type* Specifier(Node* node)
{
    if (strcmp(node->data.str, "Specifier") != 0) assert(0);
    Type* type = (Type*)malloc(sizeof(Type));
    if (node->children[0]->type == TYPE_TYPE) {
        type->kind = BASIC;
        if (strcmp(node->children[0]->data.str, "int") == 0) {
            type->u.basic = INT;
        } else {
            type->u.basic = FLOAT;
        }
    } else {  // 结构体类型
        type->kind = STRUCTURE;
        type->u.structure = StructSpecifier(node->children[0]);
    }
    return type;
}

/*
StructSpecifier : STRUCT OptTag { DefList }
                | STRUCT Tag
*/
FieldList* StructSpecifier(Node* node)
{
    if (strcmp(node->data.str, "StructSpecifier") != 0) assert(0);
    FieldList* structure = initFieldList(NULL, NULL);
    Node* child = node->children[1];
    if (child == NULL) {  // OptTag为空
        structure->name = NULL;
        push();
        structure->next = DefList(node->children[3]);
        pop();
        // TODO check and insert
    } else {
        if (strcmp(child->data.str, "Tag") == 0) {
            structure->name = child->children[0]->data.str;
            structure->next = NULL;
            // 检查结构体是否存在
            HashNode* check = findSymbol(structure->name);
            if (check == NULL) {
                errorHandler(child, UNDEFINED_STRUCT);
            } else {
                if (check->type->kind != STRUCTURE) {
                    // 这里Redefined指结构体名和别的名字冲突了
                    errorHandler(child, REDEFINED_STRUCT);
                }
            }
            check = NULL;
        } else if (strcmp(child->data.str, "OptTag") == 0) {
            structure->name = child->children[0]->data.str;
            push();
            structure->next = DefList(node->children[3]);
            pop();
            // TODO insert and check
        }
    }
    structure->type = NULL;  // type==NULL表示整个结构体，后面的是域
    child = NULL;
    return structure;
}

/*
DefList : Def DefList
        | %empty
*/
FieldList* DefList(Node* node)
{
    if (strcmp(node->data.str, "DefList") != 0) assert(0);
    Node* deflist = node;
    Node* def = NULL;
    while (deflist != NULL) {
        def = deflist->children[0];
        Def(def);
        deflist = deflist->children[1];
    }
    def = NULL;
    deflist = NULL;
    return con2FieldList(depth);
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
    Node* dec = NULL;
    while (declist != NULL) {
        dec = declist->children[0];
        Dec(type, dec);
        declist = declist->children[2];
    }
    dec = NULL;
    declist = NULL;
    return;
}

/*
Dec : VarDec
    | VarDec = Exp
*/
void Dec(Type* type, Node* node)
{
    if (strcmp(node->data.str, "Dec") != 0) assert(0);
    HashNode* hash = (HashNode*)malloc(sizeof(HashNode));
    hash->type = type;
    hash->depth = depth;
    VarDec(hash, node->children[0]);
    // TODO: try to insert to hashtable
    // TODO: check "="
}

/*
VarDec : ID
       | VarDec [ INT ]
*/
void VarDec(HashNode* hash, Node* node)
{
    if (strcmp(node->data.str, "VarDec") != 0) assert(0);
    if (node->children[0]->type == TYPE_ID) {
        hash->name = node->data.str;
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

/*
Exp : Exp "=" Exp
    | Exp "&&" Exp
    | Exp "||" Exp
    | Exp RELOP Exp
    | Exp "+" Exp
    | Exp "-" Exp
    | Exp "*" Exp
    | Exp "/" Exp
    | "(" Exp ")"           [x]
    | "-" Exp               [x]
    | "!" Exp               [x]
    | ID "(" Args ")"       [x]
    | ID "(" ")"            [x]
    | Exp "[" Exp "]"
    | Exp "." ID
    | INT                   [x]
    | FLOAT                 [x]
    | ID                    [x]
*/

Type* Exp(Node* node)
{
    if (strcmp(node->data.str, "Exp") != 0) assert(0);
    if (node->children[0]->type == TYPE_INT) {  // INT
        return &Type_int;
    } else if (node->children[0]->type == TYPE_FLOAT) {  // FLOAT
        return &Type_float;
    } else if (node->children[0]->type == TYPE_ID) {
        char* id_name = node->children[0]->data.str;
        HashNode* id = findSymbol(id_name);
        if (node->children[1] = NULL) {  // ID
            if (id == NULL) {
                errorHandler(node->children[0], UNDEFINED_VAR);
                return NULL;
            }
            // 因为Exp是嵌套的，所以这里类型是arr或struct或basic都可以，但绝不是func
            if (id->type->kind == FUNCTION) {
                errorHandler(node->children[0], REDEFINED_VAR);
                return NULL;
            }
            return id->type;
        } else {  // 函数
            if (id == NULL) {
                errorHandler(node->children[0], UNDEFINED_FUNC);
                return NULL;
            }
            if (node->children[3] != NULL) {  // ID ( Args )
                if (checkFuncCall(id, node->children[2]) == FALSE) {
                    errorHandler(node->children[0], FUNC_PARAM_MISS);
                    return NULL;
                }
            }
            return id->type->u.function.return_type;
        }
    } else if (node->children[0]->type == TYPE_TERMINAL) {
        if (strcmp(node->children[0]->data.str, "LP") == 0) {  // ( Exp )
            return Exp(node->children[1]);
        } else if (strcmp(node->children[0]->data.str, "MINUS") == 0) {  //- Exp
            return Exp(node->children[1]);
        } else if (strcmp(node->children[0]->data.str, "NOT") == 0) {  // "!" Exp
            Type* temp = Exp(node->children[1]);
            if (temp->kind != BASIC || temp->u.basic != INT) {
                // TODO 是否应该有报错，逻辑类型不匹配
                temp = NULL;
            }
            return temp;
        }
    } else {
        if (strcmp(node->children[1]->data.str, "LB") == 0) {  // Exp [ Exp ]
            Type* temp1 = Exp(node->children[0]);
            Type* temp2 = Exp(node->children[2]);
            if (NULL == temp1 || NULL == temp2) return NULL;
            if (temp1->kind != ARRAY) {
                errorHandler(node->children[0], NOT_ARR);
                return NULL;
            }
            if (temp2->kind != BASIC || temp2->u.basic != INT) {
                errorHandler(node->children[2], ARR_ACCESS_ERR);
                return NULL;
            }
            // TODO 数组越界检查
            return temp1->u.array.elem;
        } else if (strcmp(node->children[1]->data.str, "DOT") == 0) {  // Exp . ID
            Type* temp = Exp(node->children[0]);
            if (temp == NULL) {
                return NULL;
            }
            if (temp->kind != STRUCTURE) {
                errorHandler(node->children[0], NOT_STRUCT);
                temp = NULL;
            } else {
                char* id_name = node->children[2]->data.str;
                temp = findFieldID(temp, id_name);
                if (temp == NULL) {
                    errorHandler(node->children[0], STRUCT_FIELD_MISS);
                }
            }
            return temp;
        } else if (strcmp(node->children[1]->data.str, "ASSIGNOP") == 0) {  // Exp = Exp
            if (checkAssignOP(Exp(node->children[0]), Exp(node->children[2])) == FALSE) {
                return NULL;
            } else {  //! 等于号应该返回什么？
                return &Type_int;
            }
        } else if (strcmp(node->children[1]->data.str, "AND") == 0 ||  // Exp && Exp
                   strcmp(node->children[1]->data.str, "OR") == 0) {   // Exp || Exp
            Type* temp1 = Exp(node->children[0]);
            Type* temp2 = Exp(node->children[2]);
            if (NULL == temp1 || NULL == temp2) return NULL;
            if (temp1->kind != BASIC || temp1->u.basic != INT || temp2->kind != BASIC || temp2->u.basic != INT) {
                // TODO 非逻辑类型报错
                return NULL;
            }
        } else if (strcmp(node->children[1]->data.str, "ADD") == 0 ||    // Exp + Exp
                   strcmp(node->children[1]->data.str, "MINUS") == 0 ||  // Exp - Exp
                   strcmp(node->children[1]->data.str, "STAR") == 0 ||   // Exp * Exp
                   strcmp(node->children[1]->data.str, "DIV") == 0) {    // Exp / Exp
            Type* temp1 = Exp(node->children[0]);
            Type* temp2 = Exp(node->children[2]);
            if (temp1->kind != BASIC || temp2->kind != BASIC) return NULL;
            if (temp1->u.basic != temp2->u.basic) return NULL;
            return temp1;
        } else {  // Exp RELOP Exp
            Type* temp1 = Exp(node->children[0]);
            Type* temp2 = Exp(node->children[2]);
            if (temp1->kind != BASIC || temp2->kind != BASIC) return NULL;
            return &Type_int;
        }
    }
}

HashNode* buildDef(Node* node) {}

void analyseSemantic(Node* node)
{
    if (strcmp(node->data.str, "LC") == 0) {
        push();
    } else if (strcmp(node->data.str, "RC") == 0) {
        pop();
    } else if (strcmp(node->data.str, "ExtDef") == 0) {
        HashNode* hash_node = buildExtDef(node);
        // TODO check and insert
    } else if (strcmp(node->data.str, "Def") == 0) {
        HashNode* hash_node = buildDef(node);
        // TODO check and insert
    } else if (strcmp(node->data.str, "Dec") == 0) {
        // TODO check dec
    } else if (strcmp(node->data.str, "Exp") == 0) {
        // TODO check exp
    } else {
        for (int i = 0; i < node->child_ptr; i++) {
            analyseSemantic(node->children[i]);
        }
    }
}

void errorHandler(Node* node, int error_code)
{
    printf("Error type %d at Line %d: ", error_code, node->line);

    switch (error_code) {
    case UNDEFINED_VAR:
        printf("Undefined variable \"%s\".\n", node->data.str);
        break;
    case UNDEFINED_FUNC:
        printf("Undefined function \"%s\".\n", node->data.str);
        break;
    case REDEFINED_VAR:
        printf("Redefined variable \"%s\".\n", node->data.str);
        break;
    case REDEFINED_FUNC:
        printf("Redefined function \"%s\".\n", node->data.str);
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
        printf("Function \"%s\" is not applicable for arguments \n");
        break;
    case NOT_ARR:
        printf("\"%s\" is not an array.\n", node->data.str);
        break;
    case NOT_FUNC:
        printf("\"%s\" is not a function.\n", node->data.str);
        break;
    case ARR_ACCESS_ERR:
        printf("\"%f\" is not an integer.\n");
        break;
    case NOT_STRUCT:
        printf("Illegal use of \".\".\n");
        break;
    case STRUCT_FIELD_MISS:
        printf("Non-existent field \"%s\".\n", node->data.str);
        break;
    case STRUCT_FIELD_ERR:
        printf("Incorrectly defined field \"%s\".\n", node->data.str);
        break;
    case REDEFINED_STRUCT:
        printf("Duplicated name \"%s\".\n", node->data.str);
        break;
    case UNDEFINED_STRUCT:  // 传入的node是Tag
        printf("Undefined structure \"%s\".\n", node->children[0]->data.str);
        break;
    case FUNC_DEC_NO_DEF:
        printf("Incomplete definition of function \"%s\".\n", node->data.str);
        break;
    case FUNC_DEC_MISS:
        printf("Function \"%s\" declared but not defined.\n", node->data.str);
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

// 逐个比较参数情况，如果匹配返回true，否则返回false
int checkFuncCall(HashNode* func, Node* args) {}

// 检查赋值操作是否成立
int checkAssignOP(Type* l, Type* r) {}

// 检查类型是否相同，不会管名字
int checkType(Type* type1, Type* type2)
{
    if (type1->kind != type2->kind) return FALSE;
    if (type1->kind == BASIC && type1->u.basic != type2->u.basic) return FALSE;
    if (type1->kind == ARRAY) {
        if (type1->u.array.size != type2->u.array.size) return FALSE;
        if (checkType(type1->u.array.elem, type2->u.array.elem) == FALSE) return FALSE;
    }
    if (type1->kind == STRUCTURE) {
        if (checkField(type1->u.structure, type2->u.structure) == FALSE) return FALSE;
    }
    if (type1->kind == FUNCTION) return FALSE;
    return TRUE;
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

int checkFunc(HashNode* node1, HashNode* node2)
{
    if (node1->type->u.function.status == DEF && node2->type->u.function.status == DEF) return FALSE;
    if (node1->type->u.function.return_type != node2->type->u.function.return_type) return FALSE;
    if (node1->type->u.function.param_num != node2->type->u.function.param_num) return FALSE;
    if (checkField(node1->type->u.function.params, node2->type->u.function.params) == FALSE) return FALSE;
    return TRUE;
}

// 检查两个节点是否合法，包括同名检查和函数合法性检查
int checkAllow(HashNode* node1, HashNode* node2)
{
    if (node1->depth != node2->depth) return TRUE;
    if (strcmp(node1->name, node2->name) == 0) {
        if ((node1->type->kind == FUNCTION) && (node2->type->kind == FUNCTION)) {
            return checkFunc(node1, node2);
        } else {
            return FALSE;
        }
    } else {
        return TRUE;
    }
}