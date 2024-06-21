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
    char c_var;
    int32_t i_var;
    int64_t l_var;
    float f_var;
    double d_var;
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
%token <c_var> CHAR_LIT
%token <l_var> LONG_LIT
%token <d_var> DOUBLE_LIT

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
    : VARIABLE_T IDENT '(' { createFunction($1, $<s_var>2); } FunctionParameterStmtList ')' { funcSigAssign(); } '{' StmtList '}' { dumpScope(); codeRaw("return"); codeRaw(".end method"); }
;
FunctionParameterStmtList 
    : FunctionParameterStmtList ',' FunctionParameterStmt
    | FunctionParameterStmt
    | /* Empty function parameter */
;
FunctionParameterStmt
    : VARIABLE_T IDENT { pushFunParm($<var_type>1, $<s_var>2, VAR_FLAG_DEFAULT); }
    | VARIABLE_T IDENT '[' ']' { pushFunParmArray($<var_type>1, $<s_var>2, VAR_FLAG_DEFAULT); }
;

/* Scope */
StmtList 
    : StmtList Stmt
    | Stmt
;
Stmt
    : VARIABLE_T { assignType($1); } ParamStmtList ';'
    | SimpleStmt
    | Block 
    | IfStmt 
    | ForStmt
    | WhileStmt
    | COUT CoutParmListStmt ';' { stdoutPrint(); }
    | RETURN { returnExpr();}ReturnStmt
    | BREAK { printf("BREAK\n"); } ';' 
;

ReturnStmt
    : Expression ';' { returnObject($1); }
    | ';' { returnObject(NULL); }
;

SimpleStmt
    : IDENTITY assign_op {printLoad(findVariable($<s_var>1));} Expression ';' { assignVariable($<s_var>1, $<s_var>2, $<object_ptr>4); printf("%s_ASSIGN\n", $<s_var>2);}
    | IDENT '[' { loadArray($<s_var>1);} Expression ']' { printIdent(findVariable($<s_var>1)); loadArrayVariable($<s_var>1,  $<object_ptr>3); } VAL_ASSIGN Expression ';' { assignArrayVariable("EQL", $<object_ptr>2); printf("EQL_ASSIGN\n");}
    | IDENT '[' { loadArray($<s_var>1);} Expression ']' '[' Expression ']' { printIdent(findVariable($<s_var>1)); load2DArrayVariable($<s_var>1,  $<object_ptr>3, $<object_ptr>6); } VAL_ASSIGN Expression ';' { assignArrayVariable("EQL", $<object_ptr>2); printf("EQL_ASSIGN\n");}
    | IDENTITY INC_ASSIGN ';' { objectIncAssign(findVariable($<s_var>1)); printf("INC_ASSIGN\n"); }
    | IDENTITY DEC_ASSIGN ';' { objectDecAssign(findVariable($<s_var>1)); printf("DEC_ASSIGN\n"); }   
    | IDENT '(' CallFuncParamList ')' ';' { printIdent(findVariable($<s_var>1)); callFunction($1); }
    | ';' 
;

IfStmt
    : IF '(' { ifStart(); } Condition ')' { printf("IF\n"); } Stmt ElseStmt
;

ElseStmt
    : ELSE { printf("ELSE\n"); } Stmt
    | 
;

WhileStmt
    : WHILE { printf("WHILE\n");} '(' Condition ')' Block
;

ForStmt
    : FOR { printf("FOR\n"); pushScope();} '(' ForCondition ')' '{' StmtList '}'  { dumpScope(); }
;

ForCondition
    : ForAssignStmt ';' Condition ';' ForIncStmt
    | ForRangeStmt
;

ForAssignStmt
    : VARIABLE_T IDENT VAL_ASSIGN Expression {
         createVariable($1, $2, VAR_FLAG_DEFAULT);
         assignVariable($<s_var>2, $<s_var>3, $<object_ptr>4);
      }
    |
;

ForRangeStmt
    : VARIABLE_T IDENT ':' IDENT { assignType(findVariable($<s_var>4)->type); createVariable($1, $2, VAR_FLAG_DEFAULT); printIdent(findVariable($<s_var>4)); }
;


ForIncStmt
    : IDENTITY INC_ASSIGN { objectIncAssign(findVariable($<s_var>1)); printf("INC_ASSIGN\n"); }
    | IDENTITY DEC_ASSIGN { objectDecAssign(findVariable($<s_var>1)); printf("DEC_ASSIGN\n"); }   
    | IDENTITY assign_op {printLoad(findVariable($<s_var>1));} Expression { assignVariable($<s_var>1, $<s_var>2, $<object_ptr>4); printf("%s_ASSIGN\n", $<s_var>2);}
;

Block
    : '{' { pushScope();} StmtList '}' { dumpScope(); }
;

Condition 
    : Expression
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
    | IDENT VAL_ASSIGN Expression { createVariable(OBJECT_TYPE_UNDEFINED, $<s_var>1, VAR_FLAG_DEFAULT); assignVariable($<s_var>1, $<s_var>2, $<object_ptr>3);}
    | IDENT '[' Expression ']' '[' Expression ']' { create2DArray(OBJECT_TYPE_UNDEFINED, $<s_var>1, $3, $6);}
    | IDENT '[' Expression ']' { createArray(OBJECT_TYPE_UNDEFINED, $<s_var>1, $3);} ArrayAssign { arrayEndStmt($<s_var>1);}
;

ArrayAssign
    : VAL_ASSIGN { arrayStartStmt();} ArrayAssignSingle
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
    : CoutParmListStmt CoutParm
    | CoutParm 
;

CoutParm
    : SHL {codeRaw("getstatic java/lang/System/out Ljava/io/PrintStream;");} Expression {
        pushFunInParm($<object_ptr>3);
        printInvoke($<object_ptr>3);
    }
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
    : CmpExpr cmp_op ShiftExpr { $$ = objectExpBoolean($2, $1, $3); printf("%s\n", $2);}
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
    : IDENT '[' { arrayStartStmt();} Expression ']' { $$ = findArrayVariable($1, $4); printIdent($$); arrayEndCout();}
    | IDENT '[' { arrayStartStmt();} Expression ']' '[' Expression ']' { $$ = find2DArrayVariable($1, $4, $7); printIdent($$);}
    | IDENT { 
        $$ = findVariable(strdup($1)); 
        printIdent($$); 
        other_assign(); 
        printLoad($$);
      }
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
    | CHAR_LIT {
        char str[2];
        str[0] = $1;
        str[1] = '\0';
        $$ = createObject("c", strdup(str));
    }
;  

ConvertExpr
    : '(' VARIABLE_T ')' LitExpr { $$ = objectCast($2, $4); }
    | '(' VARIABLE_T ')' '(' LitExpr ')' { $$ = objectCast($2, $5); }
;

CallFuncExpr
    : IDENT '(' CallFuncParamList ')' { 
        printIdent(findVariable($<s_var>1));
        $$ = callFunction($1); 
    }
;

CallFuncParamList
    : CallFuncParamList ',' Expression
    | Expression
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
    | ADD_ASSIGN {$$ = strdup("ADD"); other_assign();}
    | SUB_ASSIGN {$$ = strdup("SUB"); other_assign();}
    | MUL_ASSIGN {$$ = strdup("MUL"); other_assign();}
    | DIV_ASSIGN {$$ = strdup("DIV"); other_assign();}
    | REM_ASSIGN {$$ = strdup("REM"); other_assign();}
    | BOR_ASSIGN {$$ = strdup("BOR"); other_assign();}
    | BAN_ASSIGN {$$ = strdup("BAN"); other_assign();}
    | BXO_ASSIGN {$$ = strdup("BXO"); other_assign();}
    | SHR_ASSIGN {$$ = strdup("SHR"); other_assign();}
    | SHL_ASSIGN {$$ = strdup("SHL"); other_assign();}
;

shift_op
    : SHR {$$ = strdup("SHR");}
    /*| SHL {$$ = strdup("SHL");}*/
;

%%
/* C code section */

