#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "tree.h"

#define DEBUG FALSE

extern Node* root;
extern int depth;

// TODO: 只要数组的基类型和维数相同就认为类型是匹配的，int a[10][2]和int b[5][3]时同一类型
// 允许类型等价的结构体或数组之间直接赋值->对应的域相应赋值
// 默认0为一切正常
enum {
    UNHANDLED = 0,
    UNDEFINED_VAR = 1,  // 1 变量在使用时未定义
    UNDEFINED_FUNC,     // 2 函数在调用时未定义
    REDEFINED_VAR,      // 3 变量重复定义，或与前面定义过的结构体名重复
    REDEFINED_FUNC,     // 4 函数重复定义
    ASSIGN_TYPE_MISS,   // 5 赋值号两边类型不匹配
    ASSIGN_LEFT_MISS,   // 6 赋值号左边出现只有右值的表达式
    OP_TYPE_MISS,       // 7 操作数类型不匹配，或操作数类型与操作符不匹配
    RETURN_TYPE_MISS,   // 8 return语句返回值与函数定义的返回类型不匹配
    FUNC_PARAM_MISS,    // 9 函数调用时实参与形参不匹配
    NOT_ARR,            // 10 对非数组变量使用[]
    NOT_FUNC,           // 11 对非函数变量使用()
    ARR_ACCESS_ERR,     // 12 数组访问操作符中出现非整数
    NOT_STRUCT,         // 13 对非结构体变量使用.
    STRUCT_FIELD_MISS,  // 14 访问结构体中未定义的域
    STRUCT_FIELD_ERR,   // 15 同一结构体域名重复定义，或定义时对域初始化
    REDEFINED_STRUCT,   // 16 结构体名与前面定义的变量重复
    UNDEFINED_STRUCT,   // 17 使用未定义过的结构体
    FUNC_DEC_NO_DEF,    // 18 函数进行了声明但没有被定义
    FUNC_DEC_MISS,      // 19 函数的多次声明相互冲突或声明与定义相互冲突
} semantic_error_code;

typedef struct FuncRecord_ {
    char* name;
    int line;
    int defined;
    struct FuncRecord_* next;
} FuncRecord;

extern void analyseSemantic(Node* node);

void Program(Node* node);
void ExtDefList(Node* node);
void ExtDef(Node* node);
void ExtDecList(Type* type, Node* node);
Type* Specifier(Node* node);
Type* StructSpecifier(Node* node);
void VarDec(HashNode* hash, Node* node);
HashNode* FunDec(Type* return_type, Node* fun_dec);
FieldList* VarList(Node* node);
void ParamDec(Node* param_dec);
void CompSt(Node* node, Type* return_type);
void StmtList(Node* node, Type* return_type);
void Stmt(Node* node, Type* return_type);
FieldList* DefList(Node* node);
void Def(Node* node);
void DecList(Type* type, Node* node);
void Dec(Type* type, Node* node);
Type* Exp(Node* node);
void errorHandler(int error_code, int line, char* msg);
Type* findFieldID(Type* type, char* name);
int checkFuncCall(HashNode* func, Node* args);
int checkType(Type* l, Type* r);
int checkField(FieldList* field1, FieldList* field2);
int checkFuncDEF();
int checkAllow(HashNode* node, int line);

FuncRecord* initFuncRecord(char* name, int line, int defined);
void addFuncRecord(char* name, int line, int defined);

#endif