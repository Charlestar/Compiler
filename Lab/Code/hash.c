#include "hash.h"
typedef unsigned int uint;
int depth = 0;
// 用于在分析时提供靶子，避免额外开辟空间来表示基本类型
Type Type_int = {.kind = BASIC, .u.basic = INT};
Type Type_float = {.kind = BASIC, .u.basic = FLOAT};

uint pjw_hashfunc(char* name)
{
    uint val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if (i = val & ~HASH_SIZE) val = (val ^ (i >> 12)) & HASH_SIZE;
    }
    return val;
}

HashNode* initSymbol(char* name, Type* type, int d)
{
    HashNode* hashnode = (HashNode*)malloc(sizeof(HashNode));
    hashnode->name = name;
    hashnode->type = type;
    hashnode->depth = d;
    hashnode->next_hash = NULL;
    hashnode->next_field_symbol = NULL;
    return hashnode;
}

FieldList* initFieldList(char* name, Type* type)
{
    FieldList* field = (FieldList*)malloc(sizeof(FieldList));
    field->name = name;
    field->type = type;
    field->next = NULL;
    return field;
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
    write_type->u.function.params = initFieldList(write_name, &Type_int);
    write_type->u.function.status = DEF;
    HashNode* read_node = initSymbol(read_name, read_type, 0);
    HashNode* write_node = initSymbol(write_name, write_type, 0);
    insertSymbol(read_node);
    insertSymbol(write_node);
}

// 是按深度插入哈希表的，总保证哈希表前面的是高深度
void insertSymbol(HashNode* hashnode)
{
    uint key = pjw_hashfunc(hashnode->name);

    if (NULL == HashTable[key] || HashTable[key]->depth <= hashnode->depth) {
        hashnode->next_hash = HashTable[key];
        HashTable[key] = hashnode;
    } else {
        HashNode* insert = HashTable[key];
        while (insert->next_hash != NULL) {
            if (insert->next_hash->depth <= hashnode->depth) break;
            insert = insert->next_hash;
        }
        hashnode->next_hash = insert->next_hash;
        insert->next_hash = hashnode;
    }

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
        // TODO 这里会有name为NULL的情况
        uint key = pjw_hashfunc(deleted->name);
        HashTable[key] = deleted->next_hash;
        del = deleted->next_field_symbol;
        free(deleted);
        deleted = NULL;
    }
    del = NULL;
    // 在删除一层后需要置为NULL，否则后面访问时会出错
    DepthStack[depth] = NULL;
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
FieldList* conv2FieldList(int d)
{
    HashNode* hash = DepthStack[d];
    if (hash == NULL) return NULL;
    FieldList* field = initFieldList(hash->name, hash->type);
    // 不这样的话，当域中只有一个元素时会直接返回NULL
    FieldList* start = field;

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