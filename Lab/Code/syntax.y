%{
	// #define YYDEBUG 1

	#include "tree.h"
	#include "lex.yy.c"
	#include <stdio.h>

	void yyerror(const char *);
	extern struct TreeNode* root;
	extern int errline;
	int start_line = 0;
%}

%locations
%error-verbose

/* declared types */
%union {
	int type_int;
	float type_float;
	struct TreeNode* type_node;
}

/* declared tokens */

%token <type_node> ID
%token <type_node> INT
%token <type_node> FLOAT
%token <type_node> STRUCT
%token <type_node> RETURN
%token <type_node> IF
%token <type_node> ELSE
%token <type_node> WHILE

%token <type_node> RELOP
%token <type_node> TYPE

%token <type_node> SEMI     ";"
%token <type_node> COMMA    ","
%token <type_node> ASSIGNOP "=" 
%token <type_node> PLUS     "+"
%token <type_node> SUB      "-"
%token <type_node> STAR     "*"
%token <type_node> DIV      "/"
%token <type_node> AND      "&&"
%token <type_node> OR       "||"
%token <type_node> DOT      "."
%token <type_node> NOT      "!"
%token <type_node> LP       "(" 
%token <type_node> RP       ")"
%token <type_node> LB       "["
%token <type_node> RB       "]"
%token <type_node> LC       "{"
%token <type_node> RC       "}"

%nonassoc REDUCE_ERROR
%nonassoc error
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right "="
%left "||"
%left "&&"
%left RELOP
%left "+" "-"
%left "*" "/"
%right "!" MINUS
%left "(" ")" "[" "]" "."

/* declared non-terminals */

%type <type_node> Program ExtDefList ExtDef ExtDecList
%type <type_node> Specifier StructSpecifier OptTag Tag
%type <type_node> VarDec FunDec VarList ParamDec
%type <type_node> CompSt StmtList Stmt
%type <type_node> DefList Def DecList Dec
%type <type_node> Exp Args

%start Program

%%

/* High-level Definitions */

Program : ExtDefList {
			$$ = CreateNode(TYPE_NONTERMINAL, "Program", @$.first_line, @$.first_column);
			root = $$;
			if ($1 == NULL) root->line = start_line;
			AddChild($$, $1);
			}
		;

ExtDefList	: ExtDef ExtDefList {
				$$ = CreateNode(TYPE_NONTERMINAL, "ExtDefList", @$.first_line, @$.first_column);
				AddChildren($$, 2, $1, $2);
				}
		   	| /* empty */ { $$ = NULL; start_line = yylineno;}
		   	;

ExtDef 	: Specifier ExtDecList ";" {
			$$ = CreateNode(TYPE_NONTERMINAL, "ExtDef", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Specifier ";" {
			$$ = CreateNode(TYPE_NONTERMINAL, "ExtDef", @$.first_line, @$.first_column);
			AddChildren($$, 2, $1, $2);
			}
		| Specifier FunDec CompSt {
			$$ = CreateNode(TYPE_NONTERMINAL, "ExtDef", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| error ";" {yyerrok;}
		| Specifier error ";" {yyerrok;}
		| Specifier ExtDecList error ";" {yyerrok;}
		| Specifier FunDec error CompSt {yyerrok;}
		;

ExtDecList	: VarDec {
				$$ = CreateNode(TYPE_NONTERMINAL, "ExtDecList", @$.first_line, @$.first_column);
				AddChild($$, $1);
				}
			| VarDec "," ExtDecList {
				$$ = CreateNode(TYPE_NONTERMINAL, "ExtDecList", @$.first_line, @$.first_column);
				AddChildren($$, 3, $1, $2, $3);
				}
			;

// Specifiers

Specifier 	: TYPE {
				$$ = CreateNode(TYPE_NONTERMINAL, "Specifier", @$.first_line, @$.first_column);
				AddChild($$, $1);
				}
			| StructSpecifier {
				$$ = CreateNode(TYPE_NONTERMINAL, "Specifier", @$.first_line, @$.first_column);
				AddChild($$, $1);
				}
			;

StructSpecifier : STRUCT OptTag "{" DefList "}" {
					$$ = CreateNode(TYPE_NONTERMINAL, "StructSpecifier", @$.first_line, @$.first_column);
					AddChildren($$, 5, $1, $2, $3, $4, $5);
					}
				| STRUCT Tag {
					$$ = CreateNode(TYPE_NONTERMINAL, "StructSpecifier", @$.first_line, @$.first_column);
					AddChildren($$, 2, $1, $2);
					}
				| STRUCT OptTag "{" DefList error "}" {yyerrok;}
				| STRUCT error {yyerrok;}
				| STRUCT error IF "{" DefList "}" {yyerrok;}
				| STRUCT error ELSE "{" DefList "}" {yyerrok;}
				| STRUCT error WHILE "{" DefList "}" {yyerrok;}
				| STRUCT error STRUCT "{" DefList "}" {yyerrok;}
				;

OptTag	: ID {
			$$ = CreateNode(TYPE_NONTERMINAL, "OptTag", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| /* empty */ {$$ = NULL;}
		;

Tag 	: ID {
			$$ = CreateNode(TYPE_NONTERMINAL, "Tag", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		;

// Declarators

VarDec	: ID %prec REDUCE_ERROR {
			$$ = CreateNode(TYPE_NONTERMINAL, "VarDec", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| VarDec "[" INT "]" {
			$$ = CreateNode(TYPE_NONTERMINAL, "VarDec", @$.first_line, @$.first_column);
			AddChildren($$, 4, $1, $2, $3, $4);
			}
		| VarDec "[" error {yyerrok;}
		| VarDec "[" INT error {yyerrok;}
		;

FunDec 	: ID "(" VarList ")" {
			$$ = CreateNode(TYPE_NONTERMINAL, "FunDec", @$.first_line, @$.first_column);
			AddChildren($$, 4, $1, $2, $3, $4);
			}
		| ID "(" ")" {
			$$ = CreateNode(TYPE_NONTERMINAL, "FunDec", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| error {yyerrok;}
		| ID error {yyerrok;}
		| ID "(" error {yyerrok;}
		| ID "(" VarList error {yyerrok;}
		;

VarList : ParamDec "," VarList {
			$$ = CreateNode(TYPE_NONTERMINAL, "VarList", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| ParamDec {
			$$ = CreateNode(TYPE_NONTERMINAL, "VarList", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		;

ParamDec : Specifier VarDec {
			$$ = CreateNode(TYPE_NONTERMINAL, "ParamDec", @$.first_line, @$.first_column);
			AddChildren($$, 2, $1, $2);
			}
		 | error VarDec {yyerrok;}
		 | Specifier error {yyerrok;}
		 ;

// Statements

CompSt	: "{" DefList StmtList "}" {
			$$ = CreateNode(TYPE_NONTERMINAL, "CompSt", @$.first_line, @$.first_column);
			AddChildren($$, 4, $1, $2, $3, $4);
			}
		| "{" DefList error "}" {yyerrok;}
		;

StmtList : Stmt StmtList {
			$$ = CreateNode(TYPE_NONTERMINAL, "StmtList", @$.first_line, @$.first_column);
			AddChildren($$, 2, $1, $2);
			}
		 | /* empty */ {$$ = NULL;}
		 ;

Stmt 	: Exp ";" {
			$$ = CreateNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			AddChildren($$, 2, $1, $2);
			}
		| CompSt {
			$$ = CreateNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| RETURN Exp ";" {
			$$ = CreateNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| IF "(" Exp ")" Stmt %prec LOWER_THAN_ELSE {
			$$ = CreateNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			AddChildren($$, 5, $1, $2, $3, $4, $5);
			}
		| IF "(" Exp ")" Stmt ELSE Stmt {
			$$ = CreateNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			AddChildren($$, 7, $1, $2, $3, $4, $5, $6, $7);
			}
		| WHILE "(" Exp ")" Stmt {
			$$ = CreateNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			AddChildren($$, 5, $1, $2, $3, $4, $5);
			}
		/* 处理末尾分号缺失 */
		| Exp error {yyerrok;}
		| RETURN Exp error {yyerrok;}
		/* 解决在StmtList中出现变量定义 */
		| error ";" {yyerrok;}
		| Exp error ";" {yyerrok;}
		| RETURN error ";" {yyerrok;}
		| IF error Stmt %prec LOWER_THAN_ELSE {yyerrok;}
		| IF error Stmt ELSE Stmt {yyerrok;}
		| WHILE error Stmt {yyerrok;}
		| IF "(" Exp error Stmt %prec LOWER_THAN_ELSE {yyerrok;}
		| IF "(" Exp error Stmt ELSE Stmt {yyerrok;}
		| WHILE "(" Exp error Stmt {yyerrok;}
		;

// Local Definitions

DefList : Def DefList {
			$$ = CreateNode(TYPE_NONTERMINAL, "DefList", @$.first_line, @$.first_column);
			AddChildren($$, 2, $1, $2);
			}
		| /* empty */ {$$ = NULL;}
		;

Def 	: Specifier DecList ";" {
			$$ = CreateNode(TYPE_NONTERMINAL, "Def", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Specifier error ";" {yyerrok;}
		| Specifier DecList error ";" {yyerrok;}
		;

DecList : Dec {
			$$ = CreateNode(TYPE_NONTERMINAL, "DecList", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| Dec "," DecList {
			$$ = CreateNode(TYPE_NONTERMINAL, "DecList", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		;

Dec 	: VarDec {
			$$ = CreateNode(TYPE_NONTERMINAL, "Dec", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| VarDec "=" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Dec", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		;



Exp    : Exp "=" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp "&&" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp "||" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp RELOP Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp "+" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp "-" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp "*" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp "/" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| "(" Exp ")" {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| "-" Exp %prec MINUS {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 2, $1, $2);
			}
		| "!" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 2, $1, $2);
			}
		| ID "(" Args ")" {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 4, $1, $2, $3, $4);
			}
		| ID "(" ")" {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp "[" Exp "]" {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 4, $1, $2, $3, $4);
			}
		| Exp "." ID {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| INT {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| FLOAT {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| ID {
			$$ = CreateNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| error "." Exp    %prec "." {yyerrok;}
		| Exp "." error    %prec "." {yyerrok;}
		| Exp "=" error    %prec "=" {yyerrok;}
		| Exp "&&" error   %prec "&&" {yyerrok;}
		| Exp "||" error   %prec "||" {yyerrok;}
		| Exp RELOP error  %prec RELOP {yyerrok;}
		| Exp "+" error    %prec "+" {yyerrok;}
		| Exp "-" error    %prec "-" {yyerrok;}
		| Exp "*" error    %prec "*" {yyerrok;}
		| Exp "/" error    %prec "/" {yyerrok;}
		| error "="        %prec "=" {yyerrok;}
		| error "&&"       %prec "&&" {yyerrok;}
		| error "||"       %prec "||" {yyerrok;}
		| error RELOP      %prec RELOP {yyerrok;}
		| error "+"        %prec "+" {yyerrok;}
		| error "-"        %prec "-" {yyerrok;}
		| error "*"        %prec "*" {yyerrok;}
		| error "/"        %prec "/" {yyerrok;}
		| "(" error ")" {yyerrok;}
		| "-" error        %prec MINUS {yyerrok;}
		| "!" error        %prec "!" {yyerrok;}
		| ID "(" error ")" {yyerrok;}
		| Exp "[" error "]" {yyerrok;}
		| ID "(" Args error {yyerrok;}
		| Exp "[" Exp error {yyerrok;}
		;

Args    : Exp "," Args {
			$$ = CreateNode(TYPE_NONTERMINAL, "Args", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Args", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		;


%%

void yyerror (const char* msg) {
	if (errline == yylineno) return;
	errline = yylineno;
	extern char* yytext;
	fprintf(stderr, "Error type B at Line %d: %s, near \"%s\".\n", yylineno, msg, yytext);
}