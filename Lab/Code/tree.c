#include "tree.h"

#define WHITE_SPACE "  "
#define TRUE 1
#define FALSE 0

// TODO: 据说使用sscanf的效率较低，可以考虑改成之前installINT的实现
int Conv2Dec(char* str, int base)
{
    int temp = 0;
    switch (base) {
    case 8:
        sscanf(str, "%o", &temp);
        break;
    case 16:
        sscanf(str, "%x", &temp);
        break;
    default:
        printf("This is a base conversion error!");
        break;
    }
    return temp;
}

Node* createNode(int type, char data[], int line, int column)
{
    // creation and assignment
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = type;
    if (data != NULL)
        strcpy(node->data, data);
    else
        strcpy(node->data, "");
    node->line = line;
    node->column = column;

    // initialization
    node->level = 0;
    memset(node->children, 0, sizeof(node->children));  //理论上等价于全赋值为NULL
    node->child_ptr = 0;
    return node;
}

void addChild(Node* parent, Node* child)
{
    if (parent == NULL) return;
    parent->children[parent->child_ptr] = child;
    parent->child_ptr += 1;
}

int printNode(Node* node)
{
    if (node == NULL) return -1;
    int has_child = FALSE;
    for (int i = 0; i < node->level; i++) {
        printf(WHITE_SPACE);
    }
    switch (node->type) {
    case TYPE_NONTERMINAL:
        printf("%s (%d)\n", node->data, node->line);
        has_child = TRUE;
        break;
    case TYPE_TERMINAL:
        printf("%s\n", node->data);
        break;
    case TYPE_TYPE:
        printf("TYPE: %s\n", node->data);
        break;
    case TYPE_ID:
        printf("ID: %s\n", node->data);
        break;
    case TYPE_DEC:
        printf("INT: %d\n", atoi(node->data));
        break;
    case TYPE_OCT:
        printf("INT: %d\n", Conv2Dec(node->data, 8));
        break;
    case TYPE_HEX:
        printf("INT: %d\n", Conv2Dec(node->data, 16));
        break;
    case TYPE_FLOAT:
        printf("FLOAT: %f\n", atof(node->data));
        break;
    case TYPE_RELOP:
        printf("RELOP: %s\n", node->data);
        break;
    default:
        printf("Wrrong TYPE when print a node!\n");
        break;
    }
    return has_child;
}

void printTree(Node* root, int level)
{
    if (root == NULL) return;
    root->level = level;
    if (printNode(root) == TRUE) {
        for (int i = 0; i < root->child_ptr; i++) {
            printTree(root->children[i], level + 1);
        }
    } else {
        return;
    }
}