#include "tree.h"

#define WHITE_SPACE "  "

Node* CreateNode(int type, char data[], int line, int column)
{
    // creation and assignment
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = type;
    if (data == NULL)
        strcpy(node->data.str, "");
    else {
        if (type == TYPE_INT) {
            node->data.i = atoi(data);
        } else if (type == TYPE_FLOAT) {
            node->data.f = atof(data);
        } else {
            strcpy(node->data.str, data);
        }
    }

    node->line = line;
    node->column = column;

    // initialization
    node->level = 0;
    memset(node->children, 0, sizeof(node->children));  //理论上等价于全赋值为NULL
    node->child_ptr = 0;
    node->prod_id = -1;
    return node;
}

void AddChild(Node* parent, int prod_id, Node* child)
{
    if (parent == NULL) return;
    parent->children[parent->child_ptr++] = child;
    parent->prod_id = prod_id;
}

void AddChildren(Node* parent, int prod_id, int childnum, ...)
{
    if (parent == NULL) return;
    parent->prod_id = prod_id;
    va_list child_list;
    va_start(child_list, childnum);
    for (int i = 0; i < childnum; i++) {
        parent->children[parent->child_ptr++] = va_arg(child_list, Node*);
    }
    va_end(child_list);
}

int PrintNode(Node* node)
{
    if (node == NULL) return -1;
    int has_child = FALSE;
    for (int i = 0; i < node->level; i++) {
        printf(WHITE_SPACE);
    }
    switch (node->type) {
    case TYPE_NONTERMINAL:
        printf("%s (%d)\n", node->data.str, node->line);
        has_child = TRUE;
        break;
    case TYPE_TERMINAL:
        printf("%s\n", node->data.str);
        break;
    case TYPE_TYPE:
        printf("TYPE: %s\n", node->data.str);
        break;
    case TYPE_ID:
        printf("ID: %s\n", node->data.str);
        break;
    case TYPE_INT:
        // 对于2147483648这个数，实际是超出了int范围的，但因为前面有个负号被单独处理了，实际是合法的
        // 这里int我使用long long进行存储，在把负号加回来时应当注意。
        printf("INT: %lld\n", node->data.i);
        break;
    case TYPE_FLOAT:
        // 此处强转float可能会造成精度损失，如果不转将以double输出。
        printf("FLOAT: %f\n", (float)node->data.f);
        break;
    case TYPE_RELOP:
        printf("RELOP\n");
        break;
    default:
        printf("Wrrong TYPE when print a node!\n");
        break;
    }
    return has_child;
}

void PrintTree(Node* root, int level)
{
    if (root == NULL) return;
    root->level = level;
    if (PrintNode(root) == TRUE) {
        for (int i = 0; i < root->child_ptr; i++) {
            PrintTree(root->children[i], level + 1);
        }
    } else {
        return;
    }
}