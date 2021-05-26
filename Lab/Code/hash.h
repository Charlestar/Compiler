#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 0x3fff
#define MAX_DEPTH 1000
#define TRUE 1
#define FALSE 0

enum { INT, FLOAT } basic_type;
// DEF定义只能有一次，DEC声明可以多次
enum { DEC, DEF } func_status;

typedef struct Type_ {
    enum { BASIC, ARRAY, STRUCTURE, FUNCTION } kind;
    union {
        // 基本类型
        int basic;
        // 数组类型信息包括元素类型与数组大小构成
        struct {
            struct Type_* elem;
            int size;
            int dim;
            // 一个维度占据的空间
            int space;
        } array;
        // 结构体类型信息是一个链表
        struct FieldList_* structure;
        // 函数类型
        struct {
            struct Type_* return_type;
            struct FieldList_* params;
            int status;
        } function;
    } u;
} Type;

typedef struct FieldList_ {
    char* name;               // 域的名字
    struct Type_* type;       // 域的类型
    struct FieldList_* next;  // 下一个域
} FieldList;

typedef struct HashNode_ {
    char* name;
    struct Type_* type;
    int depth;
    int id;  // 用于在中间代码中给变量重命名
    struct HashNode_* next_hash;
    struct HashNode_* next_field_symbol;
} HashNode;

HashNode* HashTable[HASH_SIZE];
// 域中元素是倒着插入到DepthStack的链表的。
HashNode* DepthStack[MAX_DEPTH];
extern int depth;
extern Type Type_int;
extern Type Type_float;

extern HashNode* initSymbol(char* name, Type* type, int depth);
extern FieldList* initFieldList(char* name, Type* type);
extern void insertSymbol(HashNode* hashnode);
extern HashNode* findSymbol(char* name);
extern void delField(int depth);
extern void delSymbol(HashNode* del);
extern void push();
extern void pop();
extern FieldList* conv2FieldList(int d);
extern void initHashTable();

#endif
