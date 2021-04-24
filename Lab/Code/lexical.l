%{
    #include "syntax.tab.h"
    #include "tree.h"
    #include <stdio.h>
    #include <stdlib.h>

    YYSTYPE yylval;
    int lexnerrs = 0;
    int errline = -1;
    int comment_start = 0;

    enum {
        ID_ERROR = 1,
        OCT_ERROR,
        HEX_ERROR,
        COMMENT_ERROR,
        UNDEFINED_ERROR
    } Error_Code;

    void error_handle(int error_code) {
        lexnerrs += 1;
        if (errline >= yylineno) return;
        errline = yylineno;
        switch (error_code) {
            case UNDEFINED_ERROR: fprintf(stderr, "Error type A at Line %d: Mysterious characters \"%s\".\n", yylineno, yytext); break;
            case ID_ERROR: fprintf(stderr, "Error type A at Line %d: Illegal Identifier \"%s\".\n", yylineno, yytext); break;
            case OCT_ERROR: fprintf(stderr, "Error type A at Line %d: Illegal octal number \"%s\".\n", yylineno, yytext); break;
            case HEX_ERROR: fprintf(stderr, "Error type A at Line %d: Illegal hexadecimal number \"%s\".\n", yylineno, yytext); break;
            //! 因为float error中存在".e"的情况，会将"a.e"等结果识别为错误，所以弃用这种识别。
            // case FLOAT_ERROR: fprintf(stderr, "Error type A at Line %d: Illegal floating point number \"%s\".\n", yylineno, yytext); break;
            case COMMENT_ERROR: fprintf(stderr, "Error type A at Line %d: Unclosed comment \"/*\".\n", comment_start); break;
            default: printf("Unhandled Error.\n"); break;
        }
    }

    /* Get locations */
    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
%}

/* regular definitions */
delim           [ \t\r]
ws              {delim}+
letter_         [A-Za-z_]
digit           [0-9]
/* int */
dec             0|([1-9][0-9]*)
oct             0[0-7]+
hex             0[xX]([0-9]|[A-Fa-f])+
/* float */
float           [0-9]+\.[0-9]+
float_sci       (([0-9]+\.[0-9]*)|([0-9]*\.[0-9]+))[Ee][+-]?[0-9]+

id              {letter_}({letter_}|{digit})*

comment1        "//"[^\n]*"\n"
/* comment2        \/\*(?:[^\*]|\*+[^\/\*])*\*+\/ */

/* error case (应该在正确情况之后)八进制与十进制错误存在冲突*/
id_error        [0-9]({digit}*{letter_}{digit}*)+
oct_error       0([0-7]*[89][0-7]*)+
hex_error       0[xX](([0-9]|[A-Fa-f])*[G-Zg-z]([0-9]|[A-Fa-f])*)+

%s COMMENT2

%option yylineno

%%

<INITIAL>{
    {ws}        {/*no action and no return*/}
    {comment1}  {/*no action and no return*/}
    "/*"        {comment_start = yylineno; BEGIN(COMMENT2);}
    "\n"        {yycolumn = 1;}
    "if"        {yylval.type_node = CreateNode(TYPE_TERMINAL, "IF", yylineno, yycolumn); return IF;}
    "else"      {yylval.type_node = CreateNode(TYPE_TERMINAL, "ELSE", yylineno, yycolumn); return ELSE;}
    "while"     {yylval.type_node = CreateNode(TYPE_TERMINAL, "WHILE", yylineno, yycolumn); return WHILE;}
    "struct"    {yylval.type_node = CreateNode(TYPE_TERMINAL, "STRUCT", yylineno, yycolumn); return STRUCT;}
    "return"    {yylval.type_node = CreateNode(TYPE_TERMINAL, "RETURN", yylineno, yycolumn); return RETURN;}
    "int"       {yylval.type_node = CreateNode(TYPE_TYPE, "int", yylineno, yycolumn); return TYPE;}
    "float"     {yylval.type_node = CreateNode(TYPE_TYPE, "float", yylineno, yycolumn); return TYPE;}
    {dec}       {yylval.type_node = CreateNode(TYPE_INT, yytext, yylineno, yycolumn); return INT;}
    {oct}       {yylval.type_node = CreateNode(TYPE_INT, itoa(Conv2Dec(yytext, 8)), yylineno, yycolumn); return INT;}
    {hex}       {yylval.type_node = CreateNode(TYPE_INT, itoa(Conv2Dec(yytext, 16)), yylineno, yycolumn); return INT;}
    {float}     |
    {float_sci} {yylval.type_node = CreateNode(TYPE_FLOAT, yytext, yylineno, yycolumn); return FLOAT;}
    {id}        {yylval.type_node = CreateNode(TYPE_ID, yytext, yylineno, yycolumn); return ID;}
    ";"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "SEMI", yylineno, yycolumn); return SEMI;}
    ","         {yylval.type_node = CreateNode(TYPE_TERMINAL, "COMMA", yylineno, yycolumn); return COMMA;}
    "="         {yylval.type_node = CreateNode(TYPE_TERMINAL, "ASSIGNOP", yylineno, yycolumn); return ASSIGNOP;}
    ">"         {yylval.type_node = CreateNode(TYPE_RELOP, "GT", yylineno, yycolumn); return RELOP;}
    "<"         {yylval.type_node = CreateNode(TYPE_RELOP, "LT", yylineno, yycolumn); return RELOP;}
    ">="        {yylval.type_node = CreateNode(TYPE_RELOP, "GE", yylineno, yycolumn); return RELOP;}
    "<="        {yylval.type_node = CreateNode(TYPE_RELOP, "LE", yylineno, yycolumn); return RELOP;}
    "=="        {yylval.type_node = CreateNode(TYPE_RELOP, "EQ", yylineno, yycolumn); return RELOP;}
    "!="        {yylval.type_node = CreateNode(TYPE_RELOP, "NE", yylineno, yycolumn); return RELOP;}
    "&&"        {yylval.type_node = CreateNode(TYPE_TERMINAL, "AND", yylineno, yycolumn); return AND;}
    "||"        {yylval.type_node = CreateNode(TYPE_TERMINAL, "OR", yylineno, yycolumn); return OR;}
    "."         {yylval.type_node = CreateNode(TYPE_TERMINAL, "DOT", yylineno, yycolumn); return DOT;}
    "!"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "NOT", yylineno, yycolumn); return NOT;}
    "+"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "PLUS", yylineno, yycolumn); return PLUS;}
    "-"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "MINUS", yylineno, yycolumn); return SUB;}
    "*"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "STAR", yylineno, yycolumn); return STAR;}
    "/"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "DIV", yylineno, yycolumn); return DIV;}
    "("         {yylval.type_node = CreateNode(TYPE_TERMINAL, "LP", yylineno, yycolumn); return LP;}
    ")"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "RP", yylineno, yycolumn); return RP;}
    "["         {yylval.type_node = CreateNode(TYPE_TERMINAL, "LB", yylineno, yycolumn); return LB;}
    "]"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "RB", yylineno, yycolumn); return RB;}
    "{"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "LC", yylineno, yycolumn); return LC;}
    "}"         {yylval.type_node = CreateNode(TYPE_TERMINAL, "RC", yylineno, yycolumn); return RC;}
    {oct_error} {error_handle(OCT_ERROR); yylval.type_node = NULL; return INT;}
    {hex_error} {error_handle(HEX_ERROR); yylval.type_node = NULL; return INT;}
    {id_error} {error_handle(ID_ERROR); yylval.type_node = NULL; return ID;}
    .           {error_handle(UNDEFINED_ERROR);}
}

<COMMENT2>{
    "*/"        {BEGIN(INITIAL);}
    .|"\n"      {/*no action and no return*/}
    <<EOF>>     {error_handle(COMMENT_ERROR); yyterminate();}
}

%%


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

/* 
 * 前期实现中用到的函数

int installINT(int base) {
    int val = 0;
    switch (base) {
        case 10: val = atoi(yytext); break;
        case 8: {
            for (int i = 1; i < yyleng; i++){
                int temp = yytext[i] - '0';
                val = 8 * val + temp;
            }
            break;
        }
        case 16: {
            for (int i = 2; i < yyleng; i++) {
                int temp = 0;
                if (yytext[i] >= '0' && yytext[i] <= '9'){
                    temp = yytext[i] - '0';
                }
                else if (yytext[i] >= 'a' && yytext[i] <= 'f') {
                    temp = yytext[i] - 'a' + 10;
                }
                else if (yytext[i] >= 'A' && yytext[i] <= 'F') {
                    temp = yytext[i] - 'A' + 10;
                }
                else {
                    printf("There must be something wrong!!");
                }
                val = 16 * val + temp;
            }
            break;
        }
        default: printf("This is impossible!"); break;
    }
    return val;
}

*/