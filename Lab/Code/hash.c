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

// TODO: 是否应该只保留函数定义，将声明删除
int insertSymbol(char* name, Type* type, int depth)
{
    HashNode* node = (HashNode*)malloc(sizeof(HashNode));
    node->name = name;
    node->type = type;
    node->depth = depth;

    uint key = pjw_hashfunc(name);
    if (HashTable[key] == NULL) {
        HashTable[key] = node;
        node->next_hash = NULL;
    } else {
        HashNode* peer = HashTable[key];
        while (peer != NULL && peer->depth == node->depth) {
            if (checkAllow(node, peer) == FALSE) {
                free(node);
                peer = NULL;
                node = NULL;
                return FALSE;
            }
            peer = peer->next_hash;
        }
        peer = NULL;
        node->next_hash = HashTable[key];
        HashTable[key] = node;
    }

    node->next_field_symbol = DepthStack[depth];
    DepthStack[depth] = node;
    return TRUE;
}

// 根据名称从哈希表中寻找符号，不论depth，返回找到的第一个
// TODO: 是否有全局变量的概念？
Type* findSymbol(char* name)
{
    uint key = pjw_hashfunc(name);
    if (HashTable[key] == NULL) return NULL;
    HashNode* search = HashTable[key];
    while (search != NULL) {
        if (strcmp(name, search->name) == 0)
            return search->type;
        else
            search = search->next_hash;
    }
    return NULL;
}

// 每次删除一层，都是在hash链表中删除头部
void delField(int depth)
{
    HashNode* delete = DepthStack[depth];
    while (delete != NULL) {
        uint key = pjw_hashfunc(delete->name);
        free(HashTable[key]);
        HashTable[key] = delete->next_hash;
        delete = delete->next_field_symbol;
    }
    delete = NULL;
}