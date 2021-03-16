%{
    #include "syntax.tab.h"
    #include "tree.h"

    YYSTYPE yylval;
    
    /* definitions of manifest constants
    LT, LE, EQ, NE, GT, GE 
    INT, FLOAT, ID, SEMI, COMMA, ASSIGNOP,
    RELOP, PLUS, MINUS, STAR, DIV, AND, OR,
    DOT, NOT, TYPE, LP, RP, LB, RB, LC, RC,
    STRUCT, RETURN, IF, ELSE, WHILE*/
    enum {
        DEC_ERROR = 1,
        OCT_ERROR,
        HEX_ERROR,
        FLOAT_ERROR,
        COMMENT_ERROR,
        UNDEFINED_ERROR
    } Error_Code;

    void error_handle(int error_code) {
        // printf("\033[;31merror: \033[0m");
        switch (error_code) {
            case UNDEFINED_ERROR: printf("Error type A at Line %d: Mysterious characters \'%s\'.\n", yylineno, yytext); break;
            case DEC_ERROR: printf("Error type A at Line %d: Illegal decimal number \'%s\'.\n", yylineno, yytext); break;
            case OCT_ERROR: printf("Error type A at Line %d: Illegal octal number \'%s\'.\n", yylineno, yytext); break;
            case HEX_ERROR: printf("Error type A at Line %d: Illegal hexadecimal number \'%s\'.\n", yylineno, yytext); break;
            case FLOAT_ERROR: printf("Error type A at Line %d: Illegal floating point number \'%s\'.\n", yylineno, yytext); break;
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
/* error case (应该在正确情况之后)八进制与十进制错误存在冲突*/
dec_error       [1-9]({digit}|{letter_})*
oct_error       0({digit}|{letter_})+
hex_error       0[xX]({digit}|{letter_})+
float_error     ([0-9]*\.[0-9]*)|([0-9]*\.[0-9]*[Ee]?[+-]?[0-9]*)

STATE_COMMENT1  "//"[^\n]*"\n"
STATE_COMMENT2  "/*"([^*]*|\*+[^/])*"*/"

%option yylineno

%%

{ws}        {/*no action and no return*/}
{STATE_COMMENT1}    {/*no action and no return*/}
{STATE_COMMENT2}    {/*no action and no return*/}
"\n"        {yycolumn = 1;}
"if"        {yylval.type_node = createNode(TYPE_TERMINAL, "IF", yylineno, yycolumn); return IF;}
"else"      {yylval.type_node = createNode(TYPE_TERMINAL, "ELSE", yylineno, yycolumn); return ELSE;}
"while"     {yylval.type_node = createNode(TYPE_TERMINAL, "WHILE", yylineno, yycolumn); return WHILE;}
"struct"    {yylval.type_node = createNode(TYPE_TERMINAL, "STRUCT", yylineno, yycolumn); return STRUCT;}
"return"    {yylval.type_node = createNode(TYPE_TERMINAL, "RETURN", yylineno, yycolumn); return RETURN;}
"int"       {yylval.type_node = createNode(TYPE_TYPE, "int", yylineno, yycolumn); return TYPE;}
"float"     {yylval.type_node = createNode(TYPE_TYPE, "float", yylineno, yycolumn); return TYPE;}
{dec}       {yylval.type_node = createNode(TYPE_DEC, yytext, yylineno, yycolumn); return INT;}
{oct}       {yylval.type_node = createNode(TYPE_OCT, yytext, yylineno, yycolumn); return INT;}
{hex}       {yylval.type_node = createNode(TYPE_HEX, yytext, yylineno, yycolumn); return INT;}
{float}     |
{float_sci} {yylval.type_node = createNode(TYPE_FLOAT, yytext, yylineno, yycolumn); return FLOAT;}
{id}        {yylval.type_node = createNode(TYPE_ID, yytext, yylineno, yycolumn); return ID;}
";"         {yylval.type_node = createNode(TYPE_TERMINAL, "SEMI", yylineno, yycolumn); return SEMI;}
","         {yylval.type_node = createNode(TYPE_TERMINAL, "COMMA", yylineno, yycolumn); return COMMA;}
"="         {yylval.type_node = createNode(TYPE_TERMINAL, "ASSIGNOP", yylineno, yycolumn); return ASSIGNOP;}
">"         {yylval.type_node = createNode(TYPE_RELOP, "GT", yylineno, yycolumn); return RELOP;}
"<"         {yylval.type_node = createNode(TYPE_RELOP, "LT", yylineno, yycolumn); return RELOP;}
">="        {yylval.type_node = createNode(TYPE_RELOP, "GE", yylineno, yycolumn); return RELOP;}
"<="        {yylval.type_node = createNode(TYPE_RELOP, "LE", yylineno, yycolumn); return RELOP;}
"=="        {yylval.type_node = createNode(TYPE_RELOP, "EQ", yylineno, yycolumn); return RELOP;}
"!="        {yylval.type_node = createNode(TYPE_RELOP, "NE", yylineno, yycolumn); return RELOP;}
"&&"        {yylval.type_node = createNode(TYPE_TERMINAL, "AND", yylineno, yycolumn); return AND;}
"||"        {yylval.type_node = createNode(TYPE_TERMINAL, "OR", yylineno, yycolumn); return OR;}
"."         {yylval.type_node = createNode(TYPE_TERMINAL, "DOT", yylineno, yycolumn); return DOT;}
"!"         {yylval.type_node = createNode(TYPE_TERMINAL, "NOT", yylineno, yycolumn); return NOT;}
"+"         {yylval.type_node = createNode(TYPE_TERMINAL, "PLUS", yylineno, yycolumn); return PLUS;}
"-"         {yylval.type_node = createNode(TYPE_TERMINAL, "MINUS", yylineno, yycolumn); return SUB;}
"*"         {yylval.type_node = createNode(TYPE_TERMINAL, "STAR", yylineno, yycolumn); return STAR;}
"/"         {yylval.type_node = createNode(TYPE_TERMINAL, "DIV", yylineno, yycolumn); return DIV;}
"("         {yylval.type_node = createNode(TYPE_TERMINAL, "LP", yylineno, yycolumn); return LP;}
")"         {yylval.type_node = createNode(TYPE_TERMINAL, "RP", yylineno, yycolumn); return RP;}
"["         {yylval.type_node = createNode(TYPE_TERMINAL, "LB", yylineno, yycolumn); return LB;}
"]"         {yylval.type_node = createNode(TYPE_TERMINAL, "RB", yylineno, yycolumn); return RB;}
"{"         {yylval.type_node = createNode(TYPE_TERMINAL, "LC", yylineno, yycolumn); return LC;}
"}"         {yylval.type_node = createNode(TYPE_TERMINAL, "RC", yylineno, yycolumn); return RC;}
{dec_error} {error_handle(DEC_ERROR);}
{hex_error} {error_handle(HEX_ERROR);}
{oct_error} {error_handle(OCT_ERROR);}
{float_error} {error_handle(FLOAT_ERROR);}
.           {error_handle(UNDEFINED_ERROR);}

%%

/* 目前的实现都是直接返回属性值，并没有涉及到符号表，待完善 */

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

float installFLOAT() {
    float val = 0;
    val = atof(yytext);
    return val;
}

char* installID() {
    return yytext;
}