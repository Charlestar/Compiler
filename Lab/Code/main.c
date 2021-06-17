#include <stdio.h>

#include "intercode.h"
#include "mips.h"
#include "semantic.h"
#include "tree.h"

extern FILE* yyin;
extern int yyparse(void);
extern int yyrestart(FILE*);
extern int yylineno;
extern int yynerrs;   // 语法错误的数量
extern int lexnerrs;  // 词法错误的数量

int yydebug;

struct TreeNode* root = NULL;

int main(int argc, char** argv)
{
    if (argc <= 1) return 1;
    FILE* fin = fopen(argv[1], "r");
    if (!fin) {
        perror(argv[1]);
        return 1;
    }
    yyrestart(fin);
    yylineno = 1;
    // yydebug = 1;
    // yyparse()对输入文件进行语法分析
    yyparse();
    if ((lexnerrs != 0) || (yynerrs != 0)) {
        printf("There are some syntax errors!\n");
        return -1;
    } else {
        // PrintTree(root, 0);
        // analyseSemantic(root);
        FILE* fout = fopen(argv[2], "w+");
        buildInterCode();
        // printInterCode(fout);
        printMIPS(fout);
        fclose(fout);
    }
    fclose(fin);
    return 0;

    // YY_BUFFER_STATE bp;
    // FILE* f;
    // f = fopen("", "r");
    // bp = yy_create_buffer(f, YY_BUF_SIZE);
    // yy_switch_to_buffer(bp);
    // ... yy_flush_buffer(bp);
    // ... yy_delete_buffer(bp);
}