#ifndef TREE_H
#define TREE_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHILD 64
#define TRUE 1
#define FALSE 0

enum { TYPE_TERMINAL = 1, TYPE_NONTERMINAL, TYPE_INT, TYPE_FLOAT, TYPE_TYPE, TYPE_ID, TYPE_RELOP } TYPEs;

typedef struct TreeNode {
    int type;  // 从TYPE中取值
    union {
        char str[64];
        long long i;
        double f;
    } data;                                // 节点属性值
    int line, column;                      // 词法单元出现的位置
    int level;                             // 在树中的层次，以便打印
    struct TreeNode* children[MAX_CHILD];  // 子节点列表
    int child_ptr;                         // 子节点指针，以便插入
    int prod_id;                           // 产生式编号，表示使用哪条产生式展开，从0开始编号
} Node;

extern Node* CreateNode(int type, char data[], int line, int column);
extern void AddChild(Node* parent, int prod_id, Node* child);
extern void AddChildren(Node* parent, int prod_id, int childnum, ...);
extern int PrintNode(Node* node);
extern void PrintTree(Node* root, int level);

#endif