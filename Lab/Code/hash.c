#include "hash.h"
typedef unsigned int uint;

uint pjw_hashfunc(char* name)
{
    uint val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if (i = val & ~HASH_SIZE) val = (val ^ (i >> 12)) & HASH_SIZE;
    }
    return val;
}

HashNode* initSymbol(char* name, Type* type, int depth)
{
    HashNode* hashnode = (HashNode*)malloc(sizeof(HashNode));
    hashnode->name = name;
    hashnode->type = type;
    hashnode->depth = depth;
    hashnode->next_hash = NULL;
    hashnode->next_field_symbol = NULL;
    return;
}

FieldList* initFieldList(char* name, Type* type)
{
    FieldList* field = (FieldList*)malloc(sizeof(FieldList));
    field->name = name;
    field->type = type;
    field->next = NULL;
    return;
}

// TODO: 应该只保留函数定义，将声明删除
void insertSymbol(HashNode* hashnode)
{
    uint key = pjw_hashfunc(hashnode->name);
    hashnode->next_hash = HashTable[key];
    HashTable[key] = hashnode;

    hashnode->next_field_symbol = DepthStack[hashnode->depth];
    DepthStack[hashnode->depth] = hashnode;
}

// 根据名称从哈希表中寻找符号，不论depth，返回找到的第一个
// TODO: 是否有全局变量的概念？
HashNode* findSymbol(char* name)
{
    uint key = pjw_hashfunc(name);
    HashNode* search = HashTable[key];
    while (search != NULL) {
        if (strcmp(name, search->name) == 0)
            return search;
        else
            search = search->next_hash;
    }
    search = NULL;
    return NULL;
}

// 每次删除一层，都是在hash链表中删除头部
void delField(int depth)
{
    HashNode* del = DepthStack[depth];
    while (del != NULL) {
        HashNode* deleted = del;
        uint key = pjw_hashfunc(deleted->name);
        HashTable[key] = deleted->next_hash;
        del = deleted->next_field_symbol;
        free(deleted);
        deleted = NULL;
    }
    del = NULL;
}

void delSymbol(HashNode* del)
{
    uint key = pjw_hashfunc(del->name);
    HashNode* hashnode = HashTable[key];
    if (hashnode == del) {
        HashTable[key] = del->next_hash;
    } else {
        while (hashnode->next_hash != del) {
            hashnode = hashnode->next_hash;
        }
        hashnode->next_hash = del->next_hash;
    }
    hashnode = NULL;

    HashNode* stacknode = DepthStack[del->depth];
    if (stacknode == del) {
        DepthStack[del->depth] = del->next_field_symbol;
    } else {
        while (stacknode->next_field_symbol != del) {
            stacknode = stacknode->next_field_symbol;
        }
        stacknode->next_field_symbol = del->next_field_symbol;
    }
    stacknode = NULL;
    free(del);
    del = NULL;
}

void push() { depth += 1; }

void pop()
{
    delField(depth);
    depth -= 1;
}

// 根据DepthStack中的内容，将HashNode链转化成FieldList链
// 注意这里涉及到链表转置，因为插入DepthStack[d]时是倒着插的
// ! 我感觉这里会出现很多野指针！！！！！！！！
FieldList* conv2FieldList(int d)
{
    HashNode* hash = DepthStack[d];
    if (hash == NULL) return NULL;
    FieldList* field = initFieldList(hash->name, hash->type);
    FieldList* start = NULL;

    hash = hash->next_field_symbol;
    while (hash != NULL) {
        start = initFieldList(hash->name, hash->type);
        start->next = field;
        field = start;

        hash = hash->next_field_symbol;
    }
    field = NULL;
    hash = NULL;
    return start;
}