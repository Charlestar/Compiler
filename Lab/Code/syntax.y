%{
	#include "lex.yy.c"
	#include "Tree.h"
%}

/* //just for later coding
 * typedef struct YYLTYPE {
	 int first_line;
	 int first_column;
	 int last_line;
	 int last_column;
 }
 使用@$,@1等调用
 */

/* declared types */
%union {
	struct Node* type_node;
	int type_int;
	float type_float;
	char* type_string;
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
%left '+' '-'
%left '*' '/'
%right '!' MINUS
%left '(' ')' '[' ']' '.'
// 负号既能表示取负，又能表示相减，两者结合性不同，该如何区分

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
			$$ = createNode(TYPE_NONTERMINAL, "Program", @$.first_line, @$.first_column);
			root = $$;
			addChild($$, $1);
			}
		;

ExtDefList	: ExtDef ExtDefList {
				$$ = createNode(TYPE_NONTERMINAL, "ExtDefList", @$.first_line, @$.first_column);
				addChild($$, $1);
				addChild($$, $2);
				}
		   	| /* empty */ {$$ = NULL;}
		   	;

ExtDef 	: Specifier ExtDecList ';' {
			$$ = createNode(TYPE_NONTERMINAL, "ExtDef", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| Specifier ';' {
			$$ = createNode(TYPE_NONTERMINAL, "ExtDef", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			}
		| Specifier FunDec CompSt {
			$$ = createNode(TYPE_NONTERMINAL, "ExtDef", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		;

ExtDecList	: VarDec {
				$$ = createNode(TYPE_NONTERMINAL, "ExtDecList", @$.first_line, @$.first_column);
				addChild($$, $1);
				}
			| VarDec ',' ExtDecList {
				$$ = createNode(TYPE_NONTERMINAL, "ExtDecList", @$.first_line, @$.first_column);
				addChild($$, $1);
				addChild($$, $2);
				addChild($$, $3);
				}
			;

// Specifiers

Specifier 	: TYPE {
				$$ = createNode(TYPE_NONTERMINAL, "Specifier", @$.first_line, @$.first_column);
				addChild($$, $1);
				}
			| StructSpecifier {
				$$ = createNode(TYPE_NONTERMINAL, "Specifier", @$.first_line, @$.first_column);
				addChild($$, $1);
				}
			;

StructSpecifier : STRUCT OptTag '{' DefList '}' {
					$$ = createNode(TYPE_NONTERMINAL, "StructSpecifier", @$.first_line, @$.first_column);
					addChild($$, $1);
					addChild($$, $2);
					addChild($$, $3);
					addChild($$, $4);
					addChild($$, $5);
					}
				| STRUCT Tag {
					$$ = createNode(TYPE_NONTERMINAL, "StructSpecifier", @$.first_line, @$.first_column);
					addChild($$, $1);
					addChild($$, $2);
					}
				;

OptTag	: ID {
			$$ = createNode(TYPE_NONTERMINAL, "OptTag", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		| /* empty */ {$$ = NULL;}
		;

Tag 	: ID {
			$$ = createNode(TYPE_NONTERMINAL, "Tag", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		;

// Declarators

VarDec	: ID {
			$$ = createNode(TYPE_NONTERMINAL, "VarDec", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		| VarDec '[' INT ']' {
			$$ = createNode(TYPE_NONTERMINAL, "VarDec", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			addChild($$, $4);
			}
		;

FunDec 	: ID '(' VarList ')' {
			$$ = createNode(TYPE_NONTERMINAL, "FunDec", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			addChild($$, $4);
			}
		| ID '(' ')' {
			$$ = createNode(TYPE_NONTERMINAL, "FunDec", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		;

VarList : ParamDec ',' VarList {
			$$ = createNode(TYPE_NONTERMINAL, "VarList", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| ParamDec {
			$$ = createNode(TYPE_NONTERMINAL, "VarList", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		;

ParamDec : Specifier VarDec {
			$$ = createNode(TYPE_NONTERMINAL, "ParamDec", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			}
		 ;

// Statements

CompSt	: '{' DefList StmtList '}' {
			$$ = createNode(TYPE_NONTERMINAL, "CompSt", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			addChild($$, $4);
			}
		;

StmtList : Stmt StmtList {
			$$ = createNode(TYPE_NONTERMINAL, "StmtList", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			}
		 | /* empty */ {$$ = NULL;}
		 ;

Stmt 	: Exp ';' {
			$$ = createNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			}
		| CompSt {
			$$ = createNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		| RETURN Exp ';' {
			$$ = createNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| IF '(' Exp ')' Stmt %prec LOWER_THAN_ELSE {
			$$ = createNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			addChild($$, $4);
			addChild($$, $5);
			}
		| IF '(' Exp ')' Stmt ELSE Stmt {
			$$ = createNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			addChild($$, $4);
			addChild($$, $5);
			addChild($$, $6);
			addChild($$, $7);
			}
		| WHILE '(' Exp ')' Stmt {
			$$ = createNode(TYPE_NONTERMINAL, "Stmt", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			addChild($$, $4);
			addChild($$, $5);
			}
		;

// Local Definitions

DefList : Def DefList {
			$$ = createNode(TYPE_NONTERMINAL, "DefList", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			}
		| /* empty */ {$$ = NULL;}
		;

Def 	: Specifier DecList ';' {
			$$ = createNode(TYPE_NONTERMINAL, "Def", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		;

DecList : Dec {
			$$ = createNode(TYPE_NONTERMINAL, "DecList", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		| Dec ',' DecList {
			$$ = createNode(TYPE_NONTERMINAL, "DecList", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		;

Dec 	: VarDec {
			$$ = createNode(TYPE_NONTERMINAL, "Dec", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		| VarDec '=' Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Dec", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		;



Exp    : Exp '=' Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| Exp "&&" Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| Exp "||" Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| Exp RELOP Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| Exp '+' Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| Exp '-' Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| Exp '*' Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| Exp '/' Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| '(' Exp ')' {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| '-' Exp %prec MINUS {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			}
		| '!' Exp {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			}
		| ID '(' Args ')' {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			addChild($$, $4);
			}
		| ID '(' ')' {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| Exp '[' Exp ']' {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			addChild($$, $4);
			}
		| Exp '.' ID {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			addChild($$, $2);
			addChild($$, $3);
			}
		| INT {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		| FLOAT {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		| ID {
			$$ = createNode(TYPE_NONTERMINAL, "Exp", @$.first_line, @$.first_column);
			addChild($$, $1);
			}
		;

Args    : Exp ',' Args
		| Exp
		;


%%

yyerror (char* msg) {
	fprintf(stderr, "error: %s\n", msg);
}