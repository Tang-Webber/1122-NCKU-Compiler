#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define debug printf("%s:%d: ############### debug\n", __FILE__, __LINE__)

#define toupper(_char) (_char - (char)32)

const char* objectTypeName[] = {
    [OBJECT_TYPE_UNDEFINED] = "undefined",
    [OBJECT_TYPE_VOID] = "void",
    [OBJECT_TYPE_INT] = "int",
    [OBJECT_TYPE_FLOAT] = "float",
    [OBJECT_TYPE_BOOL] = "bool",
    [OBJECT_TYPE_STR] = "string",
    [OBJECT_TYPE_FUNCTION] = "function",
};

char* yyInputFileName;
bool compileError;

int indent = 0;
int scopeLevel = -1;
int funcLineNo = 0;
int addr = -1;
ObjectType variableIdentType;

typedef struct entry {
    Object *object;
    struct entry *next;
}Entry;
Entry* table[100];
int table_index[100];

Object *queue[100];
int queue_num = 0;
Object *funcParam[100];
int funcParamNum = 0;

void initialize() {
    for (int i = 0; i < 100; i++) {
        table[i] = NULL;
        table_index[i] = 0;
        queue[i] = NULL;
        funcParam[i] = NULL;
    }
}

Object* createVariable(ObjectType variableType, char* variableName, int variableFlag){

    Entry *newEntry = (Entry *)malloc(sizeof(Entry));
    newEntry->next = NULL;
    newEntry->object = (Object *)malloc(sizeof(Object));
    newEntry->object->symbol = (SymbolData *)malloc(sizeof(SymbolData));

    newEntry->object->symbol->index = table_index[scopeLevel];
    newEntry->object->symbol->name = strdup(variableName);
    newEntry->object->type = variableType;
    newEntry->object->symbol->addr = addr;
    newEntry->object->symbol->lineno = yylineno;

    int newScope = (variableType == OBJECT_TYPE_FUNCTION) ? 1 : 0; 
    if(newScope == 1){
        newEntry->object->symbol->lineno += 1;
        newEntry->object->symbol->addr = -1;
    }
    else{
        addr++;
    }
    printf("> Insert `%s` (addr: %ld) to scope level %d\n", newEntry->object->symbol->name, newEntry->object->symbol->addr, scopeLevel);    
    if (table[scopeLevel] == NULL) {
        table[scopeLevel] = newEntry;
    } else {
        Entry *temp = table[scopeLevel];
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newEntry;
    }
    table_index[scopeLevel]++;
    return newEntry->object;
}

Object* createObject(char* variableType, char* str){
    ObjectType type;
    if(variableType == "i")
        type = OBJECT_TYPE_INT;
    if(variableType == "f")
        type = OBJECT_TYPE_FLOAT;
    if(variableType == "s")
        type = OBJECT_TYPE_STR;
    if(variableType == "b")
        type = OBJECT_TYPE_BOOL;

    Object *newObject = (Object *)malloc(sizeof(Object));
    newObject->type = type;
    switch (type) {
        case OBJECT_TYPE_INT:
            newObject->value.int_value = atoi(str);
            printf("INT_LIT %d\n", newObject->value.int_value);
            break;
        case OBJECT_TYPE_FLOAT:
            newObject->value.float_value = atof(str);
            printf("FLOAT_LIT %.6f\n", newObject->value.float_value);
            break;
        case OBJECT_TYPE_BOOL:
            newObject->value.bool_value = (strcmp(str, "TRUE") == 0);
            printf("BOOL_LIT %s\n", str);
            break;
        case OBJECT_TYPE_STR:
            newObject->value.str_value = strdup(str);
            printf("STR_LIT %s\n", newObject->value.str_value);
            break;
        default:
            
            break;
    }
    return newObject;
}

Object* findVar(char* str){
    Object *temp = (Object *)malloc(sizeof(Object));
    for(int i = scopeLevel; i >= 0; i--){
        
        if(table[i] != NULL){
            Entry *id = table[i];
            for(int j = 0; j < table_index[i]; j++){
                printf("%d %d %s\n", i, j, id->object->symbol->name);
                /*
                if (strcmp(id->object->symbol->name, str) == 0) {
                    //find it 
                    temp = id->object;
                    printf("IDENT (name=%s, address=%ld)\n", temp->symbol->name, temp->symbol->addr);
                    return temp;
                }
                */
                id = id->next;
            }      
        }
    }
    return temp;
}

Object* findVariable(char* variableName) {
    Object* variable = NULL;
    for(int i = scopeLevel; i >= 0; i--){
        if(table[i] != NULL){
            Entry *id = table[i];
            while (id != NULL) {
                if (strcmp(id->object->symbol->name, variableName) == 0) {
                    //find it 
                    variable = id->object;
                    printf("IDENT (name=%s, address=%ld)\n", variable->symbol->name, variable->symbol->addr);
                    return variable;
                }
                id = id->next;
            }             
        }
    }

    if(strcmp(variableName, "endl") == 0){
        printf("? %s", variableName);
        variable = (Object *)malloc(sizeof(Object));
        variable->symbol = (SymbolData *)malloc(sizeof(SymbolData));
        variable->symbol->name = strdup("endl");
        variable->symbol->addr = -1;
        variable->type = OBJECT_TYPE_UNDEFINED;
        printf("IDENT (name=%s, address=%ld)\n", variable->symbol->name, variable->symbol->addr);      
    }
  
    return variable;
}
/*
void printTable() {
    for (int i = 0; i < 100; i++) {
        Entry *temp = table[i];
        printf("Index %d: ", i);
        while (temp != NULL) {
            printf("%ld ", temp->object->value);
            temp = temp->next;
        }
        printf("\n");
    }
}

void freeTable() {
    for (int i = 0; i < 100; i++) {
        struct entry *temp = table[i];
        while (temp != NULL) {
            Entry *temp2 = temp;
            temp = temp->next;
            free(temp2);
        }
        table[i] = NULL;
    }
}
*/
void pushScope(){
    scopeLevel++; 
    if(table[scopeLevel] == NULL){
        printf("> Create symbol table (scope level %d)\n", scopeLevel);
        table[scopeLevel] = (Entry *)malloc(sizeof(Entry));
        table[scopeLevel]->object = (Object *)malloc(sizeof(Object));
        table[scopeLevel]->object->symbol = (SymbolData *)malloc(sizeof(SymbolData));
        table[scopeLevel]->next = NULL;
    } 
}

void dumpScope() {
    struct entry *temp = table[scopeLevel];
    while (temp != NULL) {
        Entry *temp2 = temp;
        temp = temp->next;
        free(temp2);
    }
    table[scopeLevel] = NULL;
    scopeLevel--;
}

void pushFunParm(ObjectType variableType, char* variableName, int variableFlag) {
    funcParam[funcParamNum] = createVariable(variableType, variableName, variableFlag);
    funcParamNum++;
}

void createFunction(ObjectType variableType, char* funcName) {
    printf("func: %s\n",funcName);
    Object* temp = createVariable(variableType, funcName, 0);
    pushScope();
}

void debugPrintInst(char instc, Object* a, Object* b, Object* out) {
}

bool objectExpression(char op, Object* dest, Object* val, Object* out) {
    return false;
}

bool objectExpBinary(char op, Object* a, Object* b, Object* out) {
    return false;
}

bool objectExpBoolean(char op, Object* a, Object* b, Object* out) {
    return false;
}

bool objectExpAssign(char op, Object* dest, Object* val, Object* out) {
    return false;
}

bool objectValueAssign(Object* dest, Object* val, Object* out) {
    return false;
}

bool objectNotBinaryExpression(Object* dest, Object* out) {
    return false;
}

bool objectNegExpression(Object* dest, Object* out) {
    return false;
}
bool objectNotExpression(Object* dest, Object* out) {
    return false;
}

bool objectIncAssign(Object* a, Object* out) {
    return false;
}

bool objectDecAssign(Object* a, Object* out) {
    return false;
}

bool objectCast(ObjectType variableType, Object* dest, Object* out) {
    return false;
}

void pushFunInParm(Object* variable) {
    queue[queue_num] = variable;
    queue_num++;
}

/*
char* createIdentifier(const char* str) {
    char *temp;
    strcpy(temp, str);
    if (strcmp(temp, "endl") == 0) {
        printf("(name=endl, address=-1)\n");
        return "endl";
    } 

    Object* newObj = (Object*)malloc(sizeof(Object));
    newObj = findVariable(temp);
    printf("(name=%s, address=%ld)\n", newObj->symbol->name, newObj->symbol->addr);

    return newObj->symbol->name;
}

Object* createStringObject(const char* str) {
    Object* newObj = (Object*)malloc(sizeof(Object));
    newObj->type = OBJECT_TYPE_STR;
    newObj->symbol = (SymbolData*)malloc(sizeof(SymbolData));
    strcpy(newObj->symbol->name, str);

    printf("%s", str);

    return newObj;
}
*/

char* printString(const char* str){
    char* temp = strdup(str);
    printf("%s", temp);
    return temp;
}

void stdoutPrint() {
    printf("cout");
    for(int i = 0; i < queue_num; i++){
        switch (queue[i]->type) {
            case OBJECT_TYPE_INT:
                printf(" int");
                break;
            case OBJECT_TYPE_FLOAT:
                printf(" float");
                break;
            case OBJECT_TYPE_BOOL:
                printf(" bool");
                break;
            case OBJECT_TYPE_STR:
                printf(" string");
                break;
            default:
                
                break;
        }
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    initialize();
    if (argc == 2) {
        yyin = fopen(yyInputFileName = argv[1], "r");
    } else {
        yyin = stdin;
    }
    if (!yyin) {
        printf("file `%s` doesn't exists or cannot be opened\n", yyInputFileName);
        exit(1);
    }

    // Start parsing
    yyparse();
    printf("Total lines: %d\n", yylineno);
    fclose(yyin);

    yylex_destroy();
    return 0;
}