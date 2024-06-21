/* Definition section */
%{
    #include "compiler_common.h"
    #include "compiler_util.h"
    #include "main.h"

    int yydebug = 1;
%}

/* Variable or self-defined structure */
%union {
    ObjectType var_type;

    bool b_var;
    int i_var;
    float f_var;
    char *s_var;
    
    Object object_val;
    Object* object_ptr; 
}

/* Token without return */
%token COUT
%token SHR SHL BAN BOR BNT BXO ADD SUB MUL DIV REM NOT GTR LES GEQ LEQ EQL NEQ LAN LOR
%token VAL_ASSIGN ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN REM_ASSIGN BAN_ASSIGN BOR_ASSIGN BXO_ASSIGN SHR_ASSIGN SHL_ASSIGN INC_ASSIGN DEC_ASSIGN
%token IF ELSE FOR WHILE RETURN BREAK CONTINUE

/* Token with return, which need to sepcify type */
%token <var_type> VARIABLE_T

%token <b_var> BOOL_LIT
%token <i_var> INT_LIT
%token <f_var> FLOAT_LIT
%token <s_var> STR_LIT IDENT 
%type <s_var> cmp_op add_op mul_op unary_op assign_op shift_op IDENTITY

/* Nonterminal with return, which need to sepcify type */
%type <object_ptr> Expression AddExpr AndExpr CmpExpr LitExpr MulExpr 
%type <object_ptr> ShiftExpr OtherExpr UnaryExpr ConvertExpr CallFuncExpr

%left ADD SUB
%left MUL DIV REM
%left BOR BAN BXO SHL SHR 
%left VAL_ASSIGN
%left GTR LES GEQ LEQ EQL NEQ LAN LOR

/* Yacc will start at this nonterminal */
%start Program

%%
/* Grammar section */

Program
    : { initialize(); pushScope(); } GlobalStmtList { dumpScope();}
    | /* Empty file */
;

GlobalStmtList 
    : GlobalStmtList GlobalStmt
    | GlobalStmt
;

GlobalStmt
    : DefineVariableStmt
    | FunctionDefStmt
;

DefineVariableStmt
    : VARIABLE_T IDENT VAL_ASSIGN Expression ';'
    | VARIABLE_T IDENT ';'
;

/* Function */
FunctionDefStmt
    : VARIABLE_T IDENT '(' { createFunction(OBJECT_TYPE_FUNCTION, $<s_var>2); } FunctionParameterStmtList ')' '{' StmtList '}' { dumpScope(); }
;
FunctionParameterStmtList 
    : FunctionParameterStmtList ',' FunctionParameterStmt
    | FunctionParameterStmt
    | /* Empty function parameter */
;
FunctionParameterStmt
    : VARIABLE_T IDENT { pushFunParm($<var_type>1, $<s_var>2, VAR_FLAG_DEFAULT); }
    | VARIABLE_T IDENT '[' ']' { pushFunParm($<var_type>1, $<s_var>2, VAR_FLAG_DEFAULT); }
;

/* Scope */
StmtList 
    : StmtList Stmt
    | Stmt
;
Stmt
    : ';' 
    | VARIABLE_T { assignType($1); } ParamStmtList ';'
    | SimpleStmt
    | Block 
    | IfStmt 
    | ForStmt
    | WhileStmt
    | COUT CoutParmListStmt ';' { stdoutPrint(); }
    | RETURN Expression ';' { printf("RETURN\n"); }
;

SimpleStmt
    : IDENTITY assign_op Expression ';' { assignVariable($<s_var>1, $<s_var>2, $<object_ptr>3); printf("%s_ASSIGN\n", $<s_var>2);}
    | IDENTITY '[' Expression ']' VAL_ASSIGN Expression ';' { assignArrayVariable($<s_var>1, $<object_ptr>3, "EQL", $<object_ptr>6); }
    | Expression ';'
    | IDENTITY INC_ASSIGN ';' { objectIncAssign(findVariable($<s_var>1)); printf("INC_ASSIGN\n"); }
    | IDENTITY DEC_ASSIGN ';' { objectDecAssign(findVariable($<s_var>1)); printf("DEC_ASSIGN\n"); }   
    | ';' 
;

IfStmt
    : IF '(' Condition ')' { printf("IF\n"); } Block ElseStmt
;

ElseStmt
    : ELSE { printf("ELSE\n"); } Block 
    | ELSE { printf("ELSE\n"); } IfStmt 
    | 
;
WhileStmt
    : WHILE { printf("WHILE\n"); } '(' Condition ')' Block 
;

ForStmt
    : FOR { printf("FOR\n"); pushScope();} '(' ForAssignStmt Condition ';' ForIncStmt ')' '{' StmtList '}'  { dumpScope(); }
;

Condition 
    : Expression
;

ForAssignStmt
    : SimpleStmt
    | VARIABLE_T { assignType($1); } ParamStmtList ';'
;

ForIncStmt
    : IDENTITY INC_ASSIGN { objectIncAssign(findVariable($<s_var>1)); printf("INC_ASSIGN\n"); }
    | IDENTITY DEC_ASSIGN { objectDecAssign(findVariable($<s_var>1)); printf("DEC_ASSIGN\n"); }   
    | IDENTITY assign_op Expression { assignVariable($<s_var>1, $<s_var>2, $<object_ptr>3); printf("%s_ASSIGN\n", $<s_var>2);}
;

Block
    : '{' { pushScope();} StmtList '}' { dumpScope(); }
;

IDENTITY
    : IDENT {printIdent(findVariable($<s_var>1)); $$ = $1;}
;

ParamStmtList
    : ParamStmtList ',' ParamStmt
    | ParamStmt
;

ParamStmt
    : IDENT { createVariable(OBJECT_TYPE_UNDEFINED, $<s_var>1, VAR_FLAG_DEFAULT);}
    | IDENT '[' Expression ']' { createArray(OBJECT_TYPE_UNDEFINED, $<s_var>1, $3);} ArrayAssign
    | IDENT VAL_ASSIGN Expression { createVariable(OBJECT_TYPE_UNDEFINED, $<s_var>1, VAR_FLAG_DEFAULT); assignVariable($<s_var>1, $<s_var>2, $<object_ptr>3);}
;

ArrayAssign
    : VAL_ASSIGN ArrayAssignSingle
    |
;

ArrayAssignSingle
    : '{' ArrayValList '}'  { printCreateArray();}
;

ArrayValList
    : ArrayValEmpty
    |
;

ArrayValEmpty
    : ArrayValEmpty ',' ArrayVal
    | ArrayVal 
;

ArrayVal
    : Expression {arrayInit($1);}
;

CoutParmListStmt
    : CoutParmListStmt SHL Expression { pushFunInParm($<object_ptr>3); }
    | SHL Expression { pushFunInParm($<object_ptr>2); }
;

Expression
    : Expression LOR AndExpr { $$ = objectExpBoolean("LOR", $1, $3); printf("LOR\n");}
    | AndExpr { $$ = $1; }
;

AndExpr
    : AndExpr LAN CmpExpr { $$ = objectExpBoolean("LAN", $1, $3); printf("LAN\n");}
    | CmpExpr { $$ = $1; }
;

CmpExpr
    : CmpExpr cmp_op ShiftExpr { $$ = objectExpBinary($2, $1, $3); printf("%s\n", $2);}
    | ShiftExpr { $$ = $1; }
;

ShiftExpr
    : ShiftExpr shift_op INT_LIT {
         char intStr[20];
         sprintf(intStr, "%d", $3);
         $$ = objectExpBinary($2, $1, createObject("i", intStr));
        printf("%s\n", $2);
      } 
    | AddExpr { $$ = $1; } 
;

AddExpr
    : AddExpr add_op MulExpr { $$ = objectExpression($2, $1, $3); printf("%s\n", $2);}
    | MulExpr { $$ = $1; }
;

MulExpr
    : MulExpr mul_op UnaryExpr { $$ = objectExpression($2, $1, $3); printf("%s\n", $2);}
    | UnaryExpr { $$ = $1; }
;

UnaryExpr
    : unary_op UnaryExpr { $$ = objectExpression($1, $2, $2); printf("%s\n", $1);}
    | OtherExpr { $$ = $1; }
;

OtherExpr
    : LitExpr { $$ = $1; }
    | CallFuncExpr { $$ = $1; }
    | ConvertExpr { $$ = $1; }
    | '(' Expression ')' { $$ = $2; }
;

LitExpr 
    : IDENT '[' Expression ']' { $$ = findArrayVariable($1, $3); printIdent($$);}
    | IDENT { $$ = findVariable(strdup($1)); printIdent($$);}
    | INT_LIT { 
        char intStr[20];
        sprintf(intStr, "%d", $1);
        $$ = createObject("i", intStr);
      }
    | FLOAT_LIT { 
        char floatStr[20]; 
        sprintf(floatStr, "%f", $1);
        $$ = createObject("f", floatStr);
      }
    | STR_LIT { $$ = createObject("s", strdup($1)); }
    | BOOL_LIT { 
        char *boolStr;
        if($1){
            boolStr = "TRUE";
        }
        else{
            boolStr = "FALSE";
        }
        $$ = createObject("b", boolStr);
      }
;  

ConvertExpr
    : '(' VARIABLE_T ')' LitExpr { $$ = objectCast($2, $4); }
    | '(' VARIABLE_T ')' '(' LitExpr ')' { $$ = objectCast($2, $5); }
;

CallFuncExpr
    : IDENT '(' ')' { $$ = callFunction($1); }
;

cmp_op
    : GTR {$$ = strdup("GTR");}
    | LES {$$ = strdup("LES");}
    | GEQ {$$ = strdup("GEQ");}
    | LEQ {$$ = strdup("LEQ");}
    | EQL {$$ = strdup("EQL");}
    | NEQ {$$ = strdup("NEQ");}
    | BAN {$$ = strdup("BAN");}
    | BOR {$$ = strdup("BOR");}
    | BXO {$$ = strdup("BXO");}

;


add_op
    : ADD {$$ = strdup("ADD");}
    | SUB {$$ = strdup("SUB");}
;

mul_op
    : MUL {$$ = strdup("MUL");}
    | DIV {$$ = strdup("DIV");}
    | REM {$$ = strdup("REM");}
;

unary_op
    : SUB {$$ = strdup("NEG");}
    | NOT {$$ = strdup("NOT");}
    | BNT {$$ = strdup("BNT");}
;

assign_op
    : VAL_ASSIGN {$$ = strdup("EQL");}
    | ADD_ASSIGN {$$ = strdup("ADD");}
    | SUB_ASSIGN {$$ = strdup("SUB");}
    | MUL_ASSIGN {$$ = strdup("MUL");}
    | DIV_ASSIGN {$$ = strdup("DIV");}
    | REM_ASSIGN {$$ = strdup("REM");}
    | BOR_ASSIGN {$$ = strdup("BOR");}
    | BAN_ASSIGN {$$ = strdup("BAN");}
    | BXO_ASSIGN {$$ = strdup("BXO");}
    | SHR_ASSIGN {$$ = strdup("SHR");}
    | SHL_ASSIGN {$$ = strdup("SHL");}
;

shift_op
    : SHR {$$ = strdup("SHR");}
    /*| SHL {$$ = strdup("SHL");}*/
;

%%
/* C code section */

