%{
	// #define YYDEBUG 1

	#include "tree.h"
	#include "lex.yy.c"
	#include <stdio.h>

	void yyerror(const char *);
	extern struct TreeNode* root;
	extern int lexerrline;
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
			AddChild($$, $1);
			}
		;

ExtDefList	: ExtDef ExtDefList {
				$$ = CreateNode(TYPE_NONTERMINAL, "ExtDefList", @$.first_line, @$.first_column);
				AddChildren($$, 2, $1, $2);
				}
		   	| /* empty */ {$$ = NULL;}
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
		;

ExtDecList	: VarDec {
				$$ = CreateNode(TYPE_NONTERMINAL, "ExtDecList", @$.first_line, @$.first_column);
				AddChild($$, $1);
				}
			| VarDec "," ExtDecList {
				$$ = CreateNode(TYPE_NONTERMINAL, "ExtDecList", @$.first_line, @$.first_column);
				AddChildren($$, 3, $1, $2, $3);
				}
			| VarDec error ExtDecList {yyerrok;}
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
			| error {yyerrok;}
			;

StructSpecifier : STRUCT OptTag "{" DefList "}" {
					$$ = CreateNode(TYPE_NONTERMINAL, "StructSpecifier", @$.first_line, @$.first_column);
					AddChildren($$, 5, $1, $2, $3, $4, $5);
					}
				| STRUCT Tag {
					$$ = CreateNode(TYPE_NONTERMINAL, "StructSpecifier", @$.first_line, @$.first_column);
					AddChildren($$, 2, $1, $2);
					}
				| STRUCT OptTag "{" DefList error {yyerrok;}
				| STRUCT OptTag error "}" {yyerrok;}
				| error "}" {yyerrok;}
				;

OptTag	: ID {
			$$ = CreateNode(TYPE_NONTERMINAL, "OptTag", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| /* empty */ {$$ = NULL;}
		| error {yyerrok;}
		;

Tag 	: ID {
			$$ = CreateNode(TYPE_NONTERMINAL, "Tag", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| error {yyerrok;}
		;

// Declarators

VarDec	: ID {
			$$ = CreateNode(TYPE_NONTERMINAL, "VarDec", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| VarDec "[" INT "]" {
			$$ = CreateNode(TYPE_NONTERMINAL, "VarDec", @$.first_line, @$.first_column);
			AddChildren($$, 4, $1, $2, $3, $4);
			}
		| VarDec error "]" {yyerrok;}
		| VarDec "[" INT error {yyerrok;}
		| error {yyerrok;}
		;

FunDec 	: ID "(" VarList ")" {
			$$ = CreateNode(TYPE_NONTERMINAL, "FunDec", @$.first_line, @$.first_column);
			AddChildren($$, 4, $1, $2, $3, $4);
			}
		| ID "(" ")" {
			$$ = CreateNode(TYPE_NONTERMINAL, "FunDec", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| error ")" { yyerrok; }
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
		| ParamDec error VarList {yyerrok;}
		;

ParamDec : Specifier VarDec {
			$$ = CreateNode(TYPE_NONTERMINAL, "ParamDec", @$.first_line, @$.first_column);
			AddChildren($$, 2, $1, $2);
			}
		 ;

// Statements
// TODO: CompSt have an shift/reduce when meet an error.

CompSt	: "{" DefList StmtList "}" {
			$$ = CreateNode(TYPE_NONTERMINAL, "CompSt", @$.first_line, @$.first_column);
			AddChildren($$, 4, $1, $2, $3, $4);
			}
		| error "}" {yyerrok;}
		| "{" DefList StmtList error {yyerrok;}
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
		| Exp error ";" {yyerrok;}
		| RETURN Exp error ";" {yyerrok;}
		| error Exp ";" {yyerrok;}
		| IF error Stmt %prec LOWER_THAN_ELSE {yyerrok;}
		| IF error Stmt ELSE Stmt {yyerrok;}
		| WHILE error Stmt {yyerrok;}
		| IF "(" Exp error Stmt %prec LOWER_THAN_ELSE {yyerrok;}
		| IF "(" Exp error Stmt ELSE Stmt {yyerrok;}
		| WHILE "(" Exp error Stmt {yyerrok;}
		;

// Local Definitions
// TODO: DefList have reduce/shift when meet error.

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
		| Dec error DecList {yyerrok;}
		;

Dec 	: VarDec {
			$$ = CreateNode(TYPE_NONTERMINAL, "Dec", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| VarDec "=" Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Dec", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| VarDec error Exp {yyerrok;}
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
		| error {yyerrok;}
		| ID error ")" {yyerrok;}
		| Exp error "]" {yyerrok;}
		| ID "(" error {yyerrok;}
		| ID "(" Args error {yyerrok;}
		| Exp "[" Exp error {yyerrok;}
		| error Exp {yyerrok;}
		| Exp error Exp {yyerrok;}
		;

Args    : Exp "," Args {
			$$ = CreateNode(TYPE_NONTERMINAL, "Args", @$.first_line, @$.first_column);
			AddChildren($$, 3, $1, $2, $3);
			}
		| Exp {
			$$ = CreateNode(TYPE_NONTERMINAL, "Args", @$.first_line, @$.first_column);
			AddChild($$, $1);
			}
		| Exp error Args {yyerrok;}
		;


%%

void yyerror (const char* msg) {
	if (lexerrline == yylineno) return;
	extern char* yytext;
	fprintf(stderr, "Error type B at Line %d: %s.\n", yylineno, msg);
}