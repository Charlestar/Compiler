#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "tree.h"

extern Node* root;
extern int depth;

// 用于在分析时提供靶子，避免额外开辟空间来表示基本类型
Type const Type_int = {.kind = BASIC, .u.basic = INT};
Type const Type_float = {.kind = BASIC, .u.basic = FLOAT};

// TODO: 只要数组的基类型和维数相同就认为类型是匹配的，int a[10][2]和int b[5][3]时同一类型
// 允许类型等价的结构体或数组之间直接赋值->对应的域相应赋值
// 默认0为一切正常
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

extern void analyseSemantic(Node* node);
extern void errorHandler(Node* node, int errorCode);

#endif