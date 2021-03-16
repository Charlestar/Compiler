#include <stdio.h>

#include "Tree.h"

extern FILE* yyin;
struct TreeNode* root;

int main(int argc, char** argv)
{
    if (argc <= 1) return 1;
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(f);
    yyparse();
    return 0;

    // YY_BUFFER_STATE bp;
    // FILE* f;
    // f = fopen("", "r");
    // bp = yy_create_buffer(f, YY_BUF_SIZE);
    // yy_switch_to_buffer(bp);
    // ... yy_flush_buffer(bp);
    // ... yy_delete_buffer(bp);
}