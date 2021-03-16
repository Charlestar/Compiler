%{
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
        UNDEFINED_ERROR
    } Error_Code;

    void error_handle(int error_code) {
        printf("\033[;31merror: \033[0m");
        switch (error_code) {
            case UNDEFINED_ERROR: printf("Error type A at Line %d: Mysterious characters \'%s\'.\n", yylineno, yytext); break;
            case DEC_ERROR: printf("Error type A at Line %d: Illegal decimal number \'%s\'.\n", yylineno, yytext); break;
            case OCT_ERROR: printf("Error type A at Line %d: Illegal octal number \'%s\'.\n", yylineno, yytext); break;
            case HEX_ERROR: printf("Error type A at Line %d: Illegal hexadecimal number \'%s\'.\n", yylineno, yytext); break;
            case FLOAT_ERROR: printf("Error type A at Line %d: Illegal floating point number \'%s\'.\n", yylineno, yytext); break;
            default: printf("Unhandled Error.\n"); break;
        }
    }
%}

/* regular definitions */
delim           [ \t\r\n]
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
hex_error       0[xX]({digit}|{letter_})*
float_error     ([0-9]*\.[0-9]*)|([0-9]*\.[0-9]*[Ee]?[+-]?[0-9]*)
/* 是否会让行数记录出错 */
STATE_COMMENT1  "//"[^\n]*"\n"
STATE_COMMENT2  "/*"([^*]*|\*+[^/])*"*/"

%option yylineno

%%

{ws}        {/*no action and no return*/}
{STATE_COMMENT1}    {/*no action and no return*/}
{STATE_COMMENT2}    {/*no action and no return*/}
"if"        {printf("if\n");}
"else"      {printf("else\n");}
"while"     {printf("while\n"); }
"struct"    {printf("struct\n"); }
"return"    {printf("return\n"); }
"int"       {printf("TYPE_INT\n"); }
"float"     {printf("TYPE_FLOAT\n"); }
{dec}       {printf("DEC: %s\n", yytext); }
{oct}       {printf("OTC: %s\n", yytext); }
{hex}       {printf("HEX: %s\n", yytext); }
{float}     {printf("FLOAT: %s\n", yytext); }
{float_sci} {printf("FLOATSci: %s\n", yytext); }
{id}        {printf("ID: %s\n", yytext); }
";"         {printf(";\n");}
","         {printf(",\n");}
"="         {printf("=\n");}
">"         {printf(">\n");}
"<"         {printf("<\n");}
">="        {printf(">=\n");}
"<="        {printf("<=\n");}
"=="        {printf("==\n");}
"!="        {printf("!=\n");}
"&&"        {printf("AND\n");}
"||"        {printf("OR\n");}
"."         {printf(".\n");}
"!"         {printf("!\n");}
"+"         |
"-"         |
"*"         |
"/"         {printf("%s\n", yytext);}
"("         {printf("(\n");}
")"         {printf(")\n");}
"["         {printf("[\n");}
"]"         {printf("]\n");}
"{"         {printf("{\n");}
"}"         {printf("}\n");}
{dec_error} {error_handle(DEC_ERROR);}
{hex_error} {error_handle(HEX_ERROR);}
{oct_error} {error_handle(OCT_ERROR);}
{float_error} {error_handle(FLOAT_ERROR);}
.           {error_handle(UNDEFINED_ERROR);}

%%

int installINT(int base) {
    int val = atoi(yytext);
}

int installFLOAT() {

}

int installID() {
    
}