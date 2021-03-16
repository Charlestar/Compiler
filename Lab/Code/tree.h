#ifndef TREE_H
#define TREE_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHILD 64
enum {
    TYPE_TERMINAL = 1,
    TYPE_NONTERMINAL,
    TYPE_DEC,
    TYPE_OCT,
    TYPE_HEX,
    TYPE_FLOAT,
    TYPE_TYPE,
    TYPE_ID,
    TYPE_RELOP
} TYPEs;

typedef struct TreeNode {
    int type;                              // 从TYPE中取值
    char data[32];                         // 节点属性值，只在type为INT,FLOAT,TYPE,ID时启用
    int line, column;                      // 词法单元出现的位置
    int level;                             // 在树中的层次，以便打印
    struct TreeNode* children[MAX_CHILD];  // 子节点列表
    int child_ptr;                         // 子节点指针，以便插入
} Node;

extern Node* CreateNode(int type, char data[], int line, int column);
extern void AddChild(Node* parent, Node* child);
extern void AddChildren(Node* parent, ...);
extern int PrintNode(Node* node);
extern void PrintTree(Node* root, int level);

#endif