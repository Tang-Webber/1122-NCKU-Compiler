#ifndef MAIN_H
#define MAIN_H
#include "compiler_common.h"

extern FILE* yyin;
extern bool compileError;
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

#endif