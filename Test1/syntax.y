%{
    #include<stdio.h>
    #include "lex.yy.c"
    #include "include.h"

    int errorline;

    extern tnode* troot;
    extern int berror;
    

    void yyerror (char const *s);

    int yylex();

    void delEPT(tnode* p);


#define EPT yyval = malloc(sizeof(tnode)); setnode(yyval,0,-1);

#define NewNode(s,n) yyval = malloc(sizeof(tnode));setnode(yyval,(int)s,(yyvsp[1-n])->line); yyval->left = (yyvsp[1-n]);

#define joint2 yyvsp[-1]->right = yyvsp[0];

#define joint3 joint2 yyvsp[-2]->right = yyvsp[-1];

#define joint4 joint3 yyvsp[-3]->right = yyvsp[-2];

#define joint5 joint4 yyvsp[-4]->right = yyvsp[-3];

#define joint6 joint5 yyvsp[-5]->right = yyvsp[-4];

#define joint7 joint6 yyvsp[-6]->right = yyvsp[-5];



void setnode(tnode* p,int s, int l);
void Edfs(tnode *r);

/*
%token SEMI 1 COMMA 2 TYPE 3 STRUCT 4 RETURN 5 DOT 27
%token LC 6 RC 7 LB 8 RB 9 LP 10 RP 11
%token IF 12 ELSE 13 WHILE 14 ASSIGNOP 15
%token AND 16 OR 17 RELOP 18 PLUS 19 MINUS 20 STAR 21 DIV 22 NOT 23 INT 24 FLOAT 25 ID 26
*/
%}

%define api.value.type {struct TNode*}

%error-verbose



%token SEMI COMMA TYPE STRUCT RETURN DOT 
%token LC RC LB RB LP RP 
%token IF ELSE WHILE ASSIGNOP 
%token AND OR RELOP PLUS MINUS STAR DIV NOT INT FLOAT ID

%nonassoc error2
%nonassoc error1
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE 
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT UMINUS
%left LB RB LP RP DOT



%%

/* high level Definitions*/

Program : ExtDefList{
    NewNode(Program,1)
    delEPT( $$ );
    troot = $$;
    }
    ;

ExtDefList : /*empty*/ {EPT}
    |   ExtDef ExtDefList {
        NewNode(ExtDefList,2)
        joint2
        //$$ = malloc(sizeof(tnode));setnode($$,(int)ExtDefList,$1->line); yyval->left = ($1);
        //$1->right = $2;
        delEPT( $$ );
    }
    |   ExpError ExtDefList
    |   ExpError SEMI ExtDefList
    ;

ExpError :ASSIGNOP {Edfs($1);}
    |   AND{Edfs($1);}
    |   OR{Edfs($1);}
    |   RELOP{Edfs($1);}
    |   PLUS{Edfs($1);}
    |   MINUS{Edfs($1);}
    |   STAR{Edfs($1);}
    |   DIV{Edfs($1);}
    |   ID {Edfs($1);}
    |   LP {Edfs($1);}
    |   RP {Edfs($1);}
    |   NOT{Edfs($1);}
    |   LB{Edfs($1);}
    |   RB{Edfs($1);}
    |   DOT{Edfs($1);}
    |   INT{Edfs($1);}
    |   FLOAT{Edfs($1);}
    ;

UID:    IF{yyerror(yytext);}
    |   ELSE{yyerror(yytext);}
    |   WHILE{yyerror(yytext);}
    |   STRUCT{yyerror(yytext);}
    |   INT{yyerror(yytext);}
    |   FLOAT{yyerror(yytext);}
    |   RETURN{yyerror(yytext);}
    ;

ExtDef : Specifier ExtDecList SEMI {
        NewNode(ExtDef,3)
        joint3
    }
    |   Specifier SEMI {
        NewNode(ExtDef,2)
        joint2
    }
    |   Specifier FunDec CompSt {
        NewNode(ExtDef,3)
        joint3
    }
    |   Specifier UID FunDec CompSt
    |   error SEMI
    |   Specifier error
    |   Specifier error FunDec CompSt
    |   error ExtDecList SEMI
    |   Specifier ExtDecList error
    
    |   Specifier FunDec error DefList StmList RC 
    |   Specifier FunDec error error SEMI DefList StmList RC
    |   Specifier FunDec error error
    
    |   error CompSt
    ;

ExtDecList : VarDec {
        NewNode(ExtDecList,1)
}
    |   VarDec COMMA ExtDecList {
        NewNode(ExtDecList,3)
        joint3
    }
    ;

/*Specifiers*/

Specifier : TYPE {
        NewNode(Specifier,1)
    }
    |   StructSpecifier {
        NewNode(Specifier,1)
    }
    ;

StructSpecifier : STRUCT OptTag LC DefList RC {
        NewNode(StructSpecifier,5)
        joint5
        delEPT($$);
    }
    |   STRUCT Tag {
        NewNode(StructSpecifier,2)
        joint2
    }
    
    |   STRUCT OptTag LC error SEMI DefList error
    |   STRUCT OptTag LC error SEMI DefList RC
    |   STRUCT OptTag LC error RC
    ;

OptTag : /*empty*/ {EPT}
    |   ID {
        NewNode(OptTag,1)
    }
    ;

Tag :   ID {
        NewNode(Tag,1)
}
    ;

/*Declarators*/

VarDec : ID {
        NewNode(VarDec,1)
}
    |   VarDec LB INT RB {
        NewNode(VarDec,4)
        joint4
    }
    |   VarDec LB error RB
    |   VarDec LB INT error
    |   VarDec LB error INT RB
    |   VarDec LB INT error RB 
    ;

FunDec : ID LP VarList RP {
        NewNode(FunDec,4)
        joint4
}
    |   ID LP RP {
        NewNode(FunDec,3)
        joint3
    }
    |   ID LP error
    
    
    ;

VarList : ParamDec COMMA VarList {
        NewNode(VarList,3)
        joint3
}
    |   ParamDec {
        NewNode(VarList,1)
    }
    ;

ParamDec : Specifier VarDec {
        NewNode(ParamDec,2)
        joint2
    }
    |   error VarDec
    ;

/*statements*/

CompSt : LC DefList StmList RC {
        NewNode(CompSt,4)
        joint4
        delEPT($$);
    }
    |   LC error SEMI DefList StmList error
    |   LC error SEMI DefList StmList RC
    |   LC DefList StmList error    
    
    ;

StmList : /*empty*/ {EPT}
    |   Stmt StmList {
        NewNode(StmList,2)
        joint2
        delEPT( $$ );
    }
    |   Stmt error SEMI StmList
    |   Stmt Def StmList {yyerror(yytext);}
    |   Stmt error SEMI Def StmList {
        berror = 1;
    printf("Error type B at line %d:Def after Stmt\n",$3->line+1);
    errorline = $3->line+1;
    }
    ;

Stmt : Exp SEMI {
        NewNode(Stmt,2)
        joint2
    }
    |   CompSt {
        NewNode(Stmt,1)
    }
    |   RETURN Exp SEMI {
        NewNode(Stmt,3)
        joint3
    }
    |   RETURN error SEMI
    |   IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
        NewNode(Stmt,5)
        joint5
    }
    |   IF LP Exp RP Stmt ELSE Stmt {
        NewNode(Stmt,7)
        joint7
    }
    |   IF LP error Stmt %prec LOWER_THAN_ELSE
    |   IF LP error Stmt ELSE Stmt
    |   IF LP error Exp RP Stmt %prec LOWER_THAN_ELSE
    |   IF LP error Exp RP Stmt ELSE Stmt
    |   IF LP error Exp error Stmt %prec LOWER_THAN_ELSE
    |   IF LP error Exp error Stmt ELSE Stmt
    |   IF error Stmt %prec LOWER_THAN_ELSE
    |   IF error Stmt ELSE Stmt
    |   WHILE LP Exp RP Stmt {
        NewNode(Stmt,5)
        joint5
    }
    |   WHILE error Stmt

    ;



DefList : /*empty*/ {EPT}
    |   Def DefList {
        NewNode(DefList,2)
        joint2
        delEPT( $$ );
    }
    ;

Def : Specifier DecList SEMI {
    NewNode(Def,3)
        joint3
    }
    |   Specifier DecList error
    
    ;

DecList : Dec {
        NewNode(DecList,1)
}
    |   Dec COMMA DecList {
        NewNode(DecList,3)
        joint3
    }
    ;

Dec : VarDec {
    NewNode(Dec,1)
}
    | VarDec ASSIGNOP Exp {
        NewNode(Dec,3)
        joint3
    }
    |   VarDec RELOP Exp  {
        berror = 1;
    printf("Error type B at line %d:Def after Stmt\n",$2->line+1);
    errorline = $2->line+1;
    }
    ;

/*Expressions*/

Exp : Exp ASSIGNOP Exp {
        NewNode(Exp,3)
        joint3
}
    |   Exp AND Exp {
        NewNode(Exp,3)
        joint3
    }
    |   Exp OR Exp {
        NewNode(Exp,3)
        joint3
    }
    |   Exp RELOP Exp {
        NewNode(Exp,3)
        joint3
    }
    |   Exp PLUS Exp {
        NewNode(Exp,3)
        joint3
    }
    |   Exp MINUS Exp {
        NewNode(Exp,3)
        joint3
    }
    |   Exp STAR Exp {
        NewNode(Exp,3)
        joint3
    }
    |   Exp DIV Exp {
        NewNode(Exp,3)
        joint3
    }
    |   LP Exp RP {
        NewNode(Exp,3)
        joint3
    }
    |   MINUS Exp %prec UMINUS {
        NewNode(Exp,2)
        joint2
    }
    |   NOT Exp {
        NewNode(Exp,2)
        joint2
    }
    |   ID LP Args RP {
        NewNode(Exp,4)
        joint4
    }
    |   ID LP RP {
        NewNode(Exp,3)
        joint3
    }
    |   Exp LB Exp RB {
        NewNode(Exp,4)
        joint4
    }
    |   Exp DOT ID {
        NewNode(Exp,3)
        joint3
    }
    |   ID{
        NewNode(Exp,1)
    }
    |   INT {
        NewNode(Exp,1)
    }
    |   FLOAT {
        NewNode(Exp,1)
    }
    ;

Args : Exp COMMA Args {
        NewNode(Args,3)
        joint3
}
    |   Exp {
        NewNode(Args,1)
    }
    ;
%%

void yyerror(char const * str){
    berror = 1;
    if(yylineno > errorline)
    {
        printf("Error type B at line %d,col %d:%s,yytext:%s\n",yylineno,yycolumn,str,yytext);
    errorline = yylineno;
    }
    
}

void setnode(tnode* p,int s, int l){
        p->state = s;
        p->line = l;
        p->left = null;
        p->right = null;
        p->first = null;
        p->last = null;
        p->syn = null;
        p->inh = null;
    }

void Edfs(tnode *r)
{
    
    if (r->left)
    {
        Edfs(r->left);
    }
    tnode *p = r->right;
    while (p)
    {
        if (p->left)
            Edfs(p->left);
        p = p->right;
    }
    if(r->line > errorline){
        berror = 1;
        errorline = r->line;
    printf("Error type B at line %d:Exp location error\n",errorline);
    }
    return;
}


void delEPT(tnode * r){
    tnode *p = r, *q = null;
    if(p->left == null)
    {
        p->state = 0;
        return;
    } 
    else{
        q = p->left;
        while(q){
            if(q->state ==0){
                p->left = q->right;
                free(q);
                q = p->left;
            }
            else{
                break;
            }
        }
        if(p->left == null)
        {
            p->state = 0;
            return;
        } 
        else{
            p = q;
            q = p-> right;
            while(q){
                if(q->state == 0){
                    p->right = q->right;
                    free(q);
                }
                else{
                    p = q;
                }
                q = p->right;
            }
        }

    }
    return;
}
        
