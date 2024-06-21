#ifndef MAIN_H
#define MAIN_H
#include "compiler_common.h"

#define code(format, ...) \
    fprintf(yyout, "%*s" format "\n", scopeLevel << 2, "", __VA_ARGS__)
#define codeRaw(code) \
    fprintf(yyout, "%*s" code "\n", scopeLevel << 2, "")

extern FILE* yyout;
extern FILE* yyin;
extern bool compileError;
extern int scopeLevel;
int yyparse();
int yylex();
int yylex_destroy();

#define VAR_FLAG_DEFAULT 0
#define VAR_FLAG_ARRAY 0b00000001
#define VAR_FLAG_POINTER 0b00000010

void initialize();
void pushScope();
void dumpScope();

void pushFunParm(ObjectType variableType, char* variableName, int parmFlag);
void createFunction(ObjectType variableType, char* funcName);
void pushFunInParm(Object* b);
void pushFunParmArray(ObjectType variableType, char* variableName, int variableFlag);
void printFunInParm();

Object* createObject(char* variableType, char* str);
Object* createArray(ObjectType variableType, char* variableName, Object* size);
Object* create2DArray(ObjectType variableType, char* variableName, Object* size1, Object* size2);
Object* findVariable(char* str);
Object* findArrayVariable(char* str, Object* expr);
Object* find2DArrayVariable(char* str, Object* expr1, Object* expr2);
Object* createVariable(ObjectType variableType, char* variableName, int variableFlag);
Object* callFunction(char* funcName);

//bool initVariable(ObjectType variableType, LinkedList* arraySubscripts, char* variableName);
//Object* createVariable(ObjectType variableType, LinkedList* arraySubscripts, char* variableName, Object* value);

void assignType(ObjectType type);
void assignVariable(char* varName, char* op, Object* expr);
void assignArrayVariable(char* op, Object* expr);
void printIdent(Object* ident);
void arrayInit(Object* value);
void printCreateArray();
void loadArrayVariable(char* name, Object* index);
void load2DArrayVariable(char* name, Object* index1, Object* index2);
void funcSigAssign();
char* buildJniDescriptor();
void returnExpr();
void functionLocalsBegin();
//void functionParmPush(ObjectType variableType, LinkedList* arraySubscripts, char* variableName);
//void functionBegin(ObjectType returnType, LinkedList* arraySubscripts, char* funcName);
void functionEnd(ObjectType returnType);

bool returnObject(Object* obj);
void breakLoop();

void functionArgsBegin();
void functionArgPush(Object* obj);
void functionCall(char* funcName, Object* out);

// Expressions

Object* objectExpression(char* op, Object* a, Object* b);
Object* objectExpBinary(char* op, Object* a, Object* b);
Object* objectExpBoolean(char* op, Object* a, Object* b);
Object* objectValueAssign(Object* dest, Object* val);
Object* objectNotBinaryExpression(Object* dest);
Object* objectNotExpression(Object* dest);
Object* objectNegExpression(Object* dest);
Object* objectIncAssign(Object* a);
Object* objectDecAssign(Object* a);
Object* objectCast(ObjectType variableType, Object* dest);

void stdoutPrint();
void printInvoke(Object* Expr);
void printLoad(Object* temp);
void other_assign();
void val_assign();

void loadArray(char* name);

void ifBegin();
void ifOnlyEnd();
void ifEnd();
void elseBegin();
void elseEnd();

void whileBegin();
void whileBodyBegin();
void whileEnd();

void forBegin();
void forInitEnd();
void forConditionEnd(Object* result);
void forHeaderEnd();
void foreachHeaderEnd(Object* obj);
void forEnd();
Object* objectIncAssignFor(Object* a);
Object* objectDecAssignFor(Object* a);
void assignVariableFor(char* varName, char* op, Object* expr);
void printLoadFor(Object* temp);
void printExpr1(Object* expr);
bool arrayCreate(Object* out);

void arrayEndStmt();
void arrayEndCout();
void arrayStartStmt();
void arrayExprStmt(char* name);
//bool objectArrayGet(Object* arr, LinkedList* arraySubscripts, Object* out);
//LinkedList* arraySubscriptBegin(Object* index);
//bool arraySubscriptPush(LinkedList* arraySubscripts, Object* index);
//bool arraySubscriptEnd(LinkedList* arraySubscripts);

#endif