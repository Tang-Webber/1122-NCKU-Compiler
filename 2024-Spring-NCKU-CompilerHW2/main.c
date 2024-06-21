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
int addr = 0;
ObjectType variableIdentType;

char* functionName;
char* arrayName;
int arrayIndex = 0;
bool funcDeclare = true;

Object* table[100][100];
int table_index[100];

Object *queue[100];
int queue_num = 0;
Object *funcParam[100];
int funcParamNum = 0;

void initialize() {
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 100; j++) {
            table[i][j] = NULL;
        }
        table_index[i] = 0;
        queue[i] = NULL;
        funcParam[i] = NULL;
    }
    funcDeclare = true;
}

Object* createVariable(ObjectType variableType, char* variableName, int variableFlag){
    Object *newObject = (Object *)malloc(sizeof(Object));
    newObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));
    newObject->type = variableType;
    if(variableType == OBJECT_TYPE_UNDEFINED || variableType == OBJECT_TYPE_AUTO){
        newObject->type = variableIdentType;
    }

    newObject->symbol->index = table_index[scopeLevel];
    newObject->symbol->name = strdup(variableName);
    newObject->symbol->addr = addr;
    newObject->symbol->lineno = yylineno;
    newObject->symbol->func_sig = strdup("-");

    if(variableType == OBJECT_TYPE_FUNCTION){
        functionName = strdup(newObject->symbol->name);
        newObject->value.funcType = variableIdentType;
        if(strcmp(newObject->symbol->name, "main") == 0){
            funcDeclare = false;
        }

        newObject->symbol->addr = -1;
    }
    else{
        addr++;
    }
    
    printf("> Insert `%s` (addr: %ld) to scope level %d\n", newObject->symbol->name, newObject->symbol->addr, scopeLevel);    
    table[scopeLevel][table_index[scopeLevel]] = newObject;
    table_index[scopeLevel]++;
    return newObject;
}

Object* createArray(ObjectType variableType, char* variableName, Object* size){
    int num = size->value.int_value;
    arrayName = strdup(variableName);
    arrayIndex = 0;

    Object *newObject = (Object *)malloc(sizeof(Object));
    newObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));

    newObject->symbol->index = table_index[scopeLevel];
    newObject->symbol->name = strdup(variableName);
    newObject->type = variableType;
    if(variableType == OBJECT_TYPE_UNDEFINED){
        newObject->type = variableIdentType;
    }
    newObject->symbol->addr = addr;
    newObject->symbol->lineno = yylineno;
    newObject->symbol->func_sig = strdup("-");
    newObject->value.array_value.length = num;

    addr++;
    table[scopeLevel][table_index[scopeLevel]] = newObject;
    table_index[scopeLevel]++;
    return newObject;
}

Object* create2DArray(ObjectType variableType, char* variableName, Object* size1, Object* size2){
    int num1 = size1->value.int_value;
    int num2 = size2->value.int_value;

    Object *newObject = (Object *)malloc(sizeof(Object));
    newObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));

    newObject->symbol->index = table_index[scopeLevel];
    newObject->symbol->name = strdup(variableName);
    newObject->type = variableType;
    if(variableType == OBJECT_TYPE_UNDEFINED){
        newObject->type = variableIdentType;
    }
    newObject->symbol->addr = addr;
    newObject->symbol->lineno = yylineno;
    newObject->symbol->func_sig = strdup("-");

    newObject->value.array_value.length = num1 * num2;
    newObject->value.array_value.twoDSize = num2;

    addr++;
    table[scopeLevel][table_index[scopeLevel]] = newObject;
    table_index[scopeLevel]++;
    printf("> Insert `%s` (addr: %ld) to scope level %d\n", newObject->symbol->name, newObject->symbol->addr, scopeLevel);
    return newObject;
}

void arrayInit(Object* value){
    Object* array = findVariable(arrayName);
    array->value.array_value.elements[arrayIndex] = value->value.int_value;
    arrayIndex++;
}

void printCreateArray(){
    Object* array = findVariable(arrayName);
    printf("create array: %d\n", arrayIndex);
    printf("> Insert `%s` (addr: %ld) to scope level %d\n", array->symbol->name, array->symbol->addr, scopeLevel);    
    arrayIndex = 0;
    arrayName = NULL;
}

void assignType(ObjectType type){
    variableIdentType = type;
}

void assignVariable(char* varName, char* op, Object* expr){
    Object* target = findVariable(varName);
    if (target-> type == OBJECT_TYPE_AUTO){
        target->type = expr->type;
    }
    if (target->type == OBJECT_TYPE_INT && expr->type == OBJECT_TYPE_INT) {
        if(strcmp("EQL", op) == 0)         // =
            target->value.int_value = expr->value.int_value;
        else if(strcmp("ADD", op) == 0)    // +=
            target->value.int_value += expr->value.int_value;
        else if(strcmp("SUB", op) == 0)    // -=
            target->value.int_value -= expr->value.int_value;
        else if(strcmp("MUL", op) == 0)    // *=
            target->value.int_value *= expr->value.int_value;
        else if(strcmp("DIV", op) == 0)    // /=
            target->value.int_value = (int)(target->value.int_value / expr->value.int_value);
        else if(strcmp("REM", op) == 0)    // %=
            target->value.int_value %= expr->value.int_value;
        else if(strcmp("BOR", op) == 0)    // |=
            target->value.int_value |= expr->value.int_value;
        else if(strcmp("BAN", op) == 0)    // &=
            target->value.int_value &= expr->value.int_value;
        else if (strcmp("SHR", op) == 0) 
            target->value.int_value >>= expr->value.int_value;
        else if (strcmp("SHL", op) == 0)
            target->value.int_value <<= expr->value.int_value;
    }
    if (target->type == OBJECT_TYPE_FLOAT && expr->type == OBJECT_TYPE_FLOAT) {
        if(strcmp("VAL", op) == 0)         // =
            target->value.float_value = expr->value.float_value;
        else if(strcmp("ADD", op) == 0)    // +=
            target->value.float_value += expr->value.float_value;
        else if(strcmp("SUB", op) == 0)    // -=
            target->value.float_value -= expr->value.float_value;
        else if(strcmp("MUL", op) == 0)    // *=
            target->value.float_value *= expr->value.float_value;
        else if(strcmp("DIV", op) == 0)    // /=
            target->value.float_value /= expr->value.float_value;
    }
    if (target->type == OBJECT_TYPE_BOOL && expr->type == OBJECT_TYPE_BOOL) {
        if(strcmp("VAL", op) == 0)         // =
            target->value.bool_value = expr->value.bool_value;
        else if(strcmp("BOR", op) == 0)    // |=
            target->value.bool_value |= expr->value.bool_value;
        else if(strcmp("BAN", op) == 0)    // &=
            target->value.bool_value &= expr->value.bool_value;
    }
    if (target->type == OBJECT_TYPE_STR && expr->type == OBJECT_TYPE_STR) {
        if(strcmp("VAL", op) == 0)         // =
            target->value.str_value = strdup(expr->value.str_value);
        else if(strcmp("ADD", op) == 0){   // +=
            char* new_str = malloc(strlen(target->value.str_value) + strlen(expr->value.str_value) + 1);
            strcpy(new_str, target->value.str_value);
            strcat(new_str, expr->value.str_value);
            free(target->value.str_value);
            target->value.str_value = strdup(new_str);  
        }     
    }
}

void assignArrayVariable(char* op, Object* expr){
    Object* target = findVariable(arrayName);
    int idx = arrayIndex;
    if (idx >= 0 && idx < target->value.array_value.length) {
        if (strcmp("EQL", op) == 0)         // =
            target->value.array_value.elements[idx] = expr->value.int_value;
    }
}

void loadArrayVariable(char* name, Object* index){
    arrayName = name;
    arrayIndex = index->value.int_value;
}

void load2DArrayVariable(char* name, Object* index1, Object* index2){
    arrayName = name;
    Object* array = findVariable(arrayName);
    arrayIndex = array->value.array_value.twoDSize * index1->value.int_value + index2->value.int_value;
}

Object* createObject(char* variableType, char* str){
    Object *newObject = (Object *)malloc(sizeof(Object));   
    if(strcmp(variableType, "i") == 0){
        newObject->type = OBJECT_TYPE_INT;
        newObject->value.int_value = atoi(str);
        printf("INT_LIT %d\n", newObject->value.int_value);        
    }
    if(strcmp(variableType, "f") == 0){
        newObject->type = OBJECT_TYPE_FLOAT;
        newObject->value.float_value = atof(str);
        printf("FLOAT_LIT %.6f\n", newObject->value.float_value);
    }
    if(strcmp(variableType, "b") == 0){
        newObject->type = OBJECT_TYPE_BOOL;
        newObject->value.bool_value = (strcmp(str, "TRUE") == 0);
        printf("BOOL_LIT %s\n", str);
    }
    if(strcmp(variableType, "s") == 0){
        newObject->type = OBJECT_TYPE_STR;
        newObject->value.str_value = strdup(str);
        printf("STR_LIT \"%s\"\n", newObject->value.str_value);
    }

    return newObject;
}

void printIdent(Object* ident){
   printf("IDENT (name=%s, address=%ld)\n", ident->symbol->name, ident->symbol->addr);          
}

Object* findVariable(char* str){
    Object *temp = (Object *)malloc(sizeof(Object));
    for(int i = scopeLevel; i >= 0; i--){
        for(int j = 0; j < table_index[i]; j++){
            if (strcmp(table[i][j]->symbol->name, str) == 0) {
                temp = table[i][j];
                return temp;
            }
        }      
    }

    if(strcmp(str, "endl") == 0){
        temp = (Object *)malloc(sizeof(Object));
        temp->symbol = (SymbolData *)malloc(sizeof(SymbolData));
        temp->symbol->name = strdup("endl");
        temp->value.str_value = strdup("endl");
        temp->symbol->addr = -1;
        temp->type = OBJECT_TYPE_STR;
        return temp;   
    }
    printf("no variable\n");
    return temp;
}

Object* findArrayVariable(char* str, Object* expr){
    Object* array = findVariable(str);
    Object* arrayObject = (Object*)malloc(sizeof(Object));
    arrayObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));
    arrayObject->type = array->type;  
    arrayObject->symbol->index = array->symbol->index;
    arrayObject->symbol->name = strdup(array->symbol->name);
    arrayObject->symbol->addr = array->symbol->addr;
    arrayObject->symbol->lineno = array->symbol->lineno;
    arrayObject->symbol->func_sig = array->symbol->func_sig;

    arrayObject->value.int_value = array->value.array_value.elements[expr->value.int_value];

    return arrayObject;
}

Object* find2DArrayVariable(char* str, Object* expr1, Object* expr2){
    Object* array = findVariable(str);
    Object* arrayObject = (Object*)malloc(sizeof(Object));
    arrayObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));
    arrayObject->type = array->type;  
    arrayObject->symbol->index = array->symbol->index;
    arrayObject->symbol->name = strdup(array->symbol->name);
    arrayObject->symbol->addr = array->symbol->addr;
    arrayObject->symbol->lineno = array->symbol->lineno;
    arrayObject->symbol->func_sig = array->symbol->func_sig;

    arrayObject->value.int_value = array->value.array_value.elements[expr1->value.int_value * array->value.array_value.twoDSize + expr2->value.int_value];

    return arrayObject;
}

void printFunInParm() {
    printf("\n> Dump symbol table (scope level: %d)\n", scopeLevel);
    printf("Index     Name                Type      Addr      Lineno    Func_sig  \n");
    
    for (int i = 0; i < table_index[scopeLevel]; i++) {
        printf("%-10d%-20s%-10s%-10ld%-10d%-10s\n",
               i,
               table[scopeLevel][i]->symbol->name,
               objectTypeName[table[scopeLevel][i]->type],
               table[scopeLevel][i]->symbol->addr,
               table[scopeLevel][i]->symbol->lineno,
               table[scopeLevel][i]->symbol->func_sig);
    }
    for(int i=0;i<funcParamNum;i++){
        funcParam[i] = NULL;
    }
    funcParamNum = 0;
}

void pushScope(){
    scopeLevel++; 
    printf("> Create symbol table (scope level %d)\n", scopeLevel);
}

void dumpScope() {
    printFunInParm();
    for(int j = 0; j < table_index[scopeLevel];j++){
        free(table[scopeLevel][j]);
    }
    table_index[scopeLevel] = 0;
    scopeLevel--;
}

void pushFunParm(ObjectType variableType, char* variableName, int variableFlag) {
    funcParam[funcParamNum] = createVariable(variableType, variableName, variableFlag);
    funcParamNum++;
}

void pushFunParmArray(ObjectType variableType, char* variableName, int variableFlag) {
    Object* size = malloc(sizeof(Object));
    size->value.int_value = -1;
    funcParam[funcParamNum] = createArray(variableType, variableName, size);
    printf("> Insert `%s` (addr: %ld) to scope level %d\n", funcParam[funcParamNum]->symbol->name, funcParam[funcParamNum]->symbol->addr, scopeLevel); 
    funcParamNum++;
}

void pushFunInParm(Object* variable) {
    queue[queue_num] = variable;
    queue_num++;
}

void createFunction(ObjectType variableType, char* funcName) {
    printf("func: %s\n",funcName);
    variableIdentType = variableType;
    Object* temp = createVariable(OBJECT_TYPE_FUNCTION, funcName, 0);
    pushScope();
}

void stdoutPrint() {
    printf("cout");
    for(int i = 0; i < queue_num; i++){
        printf(" %s",objectTypeName[queue[i]->type]);
        queue[i] = NULL;
    }
    printf("\n");
    queue_num = 0;
}

void debugPrintInst(char instc, Object* a, Object* b) {
}

Object* objectExpression(char* op, Object* a, Object* b) {
    if(!funcDeclare){
        Object* result = malloc(sizeof(Object));
        if(strcmp(op, "NOT") == 0) {
            result = objectNotExpression(a);
        } else if(strcmp(op, "NEG") == 0) {
            result = objectNotBinaryExpression(a);
        } else if(strcmp(op, "BNT") == 0) {
            result = objectNotBinaryExpression(a);
        } else {
            if (a->type == OBJECT_TYPE_INT && b->type == OBJECT_TYPE_INT) {
                result->type = OBJECT_TYPE_INT;
                if (strcmp(op, "ADD") == 0) {
                    result->value.int_value = a->value.int_value + b->value.int_value;
                } else if (strcmp(op, "SUB") == 0) {
                    result->value.int_value = a->value.int_value - b->value.int_value;
                } else if (strcmp(op, "MUL") == 0) {
                    result->value.int_value = a->value.int_value * b->value.int_value;
                } else if (strcmp(op, "DIV") == 0) {
                    result->value.int_value = a->value.int_value / b->value.int_value;
                } else if (strcmp(op, "REM") == 0) {
                    result->value.int_value = a->value.int_value % b->value.int_value;
                }
            } else if (a->type == OBJECT_TYPE_FLOAT && b->type == OBJECT_TYPE_FLOAT) {
                result->type = OBJECT_TYPE_FLOAT;
                if (strcmp(op, "ADD") == 0) {
                    result->value.float_value = a->value.float_value + b->value.float_value;
                } else if (strcmp(op, "SUB") == 0) {
                    result->value.float_value = a->value.float_value - b->value.float_value;
                } else if (strcmp(op, "MUL") == 0) {
                    result->value.float_value = a->value.float_value * b->value.float_value;
                } else if (strcmp(op, "DIV") == 0) {
                    result->value.float_value = a->value.float_value / b->value.float_value;
                }
            }
        }   
        return result;    
    }
    return a;
}

Object* objectExpBinary(char* op, Object* a, Object* b) {
    Object* result = malloc(sizeof(Object));

    result->type = a->type;

    if (strcmp(op, "BAN") == 0) {           // Bitwise AND
        result->value.int_value = a->value.int_value & b->value.int_value;
    } 
    else if (strcmp(op, "BOR") == 0) {      // Bitwise OR
        result->value.int_value = a->value.int_value | b->value.int_value;
    } 
    else if (strcmp(op, "BXO") == 0) {      // Bitwise XOR
        result->value.int_value = a->value.int_value ^ b->value.int_value;
    }
    else if (strcmp(op, "SHL") == 0) {      // Shift Left
        result->value.int_value = a->value.int_value << b->value.int_value;
    }
    else if (strcmp(op, "SHR") == 0) {      // Shift Right
        result->value.int_value = a->value.int_value >> b->value.int_value;
    }
    
    return result;
}


Object* objectExpBoolean(char* op, Object* a, Object* b) {
    Object* result = malloc(sizeof(Object));
    result->type = OBJECT_TYPE_BOOL;

    if (strcmp(op, "LOR") == 0) {
        result->value.bool_value = a->value.bool_value || b->value.bool_value;
    } 
    else if (strcmp(op, "LAN") == 0) {
        result->value.bool_value = a->value.bool_value && b->value.bool_value;
    } 
    else if (strcmp(op, "GTR") == 0) {      // Greater than
        result->value.bool_value = a->value.float_value > b->value.float_value;
    } 
    else if (strcmp(op, "LES") == 0) {      // Less than
        result->value.bool_value = a->value.float_value < b->value.float_value;
    } 
    else if (strcmp(op, "GEQ") == 0) {      // Greater than or equal to
        result->value.bool_value = a->value.float_value >= b->value.float_value;
    } 
    else if (strcmp(op, "LEQ") == 0) {      // Less than or equal to
        result->value.bool_value = a->value.float_value <= b->value.float_value;
    } 
    else if (strcmp(op, "EQL") == 0) {      // Equal to
        if (a->type == OBJECT_TYPE_INT && b->type == OBJECT_TYPE_INT) {
            result->value.bool_value = a->value.int_value == b->value.int_value;
        } 
        else if (a->type == OBJECT_TYPE_FLOAT && b->type == OBJECT_TYPE_FLOAT) {
            result->value.bool_value = a->value.float_value == b->value.float_value;
        } 
        else if (a->type == OBJECT_TYPE_BOOL && b->type == OBJECT_TYPE_BOOL) {
            result->value.bool_value = a->value.bool_value == b->value.bool_value;
        } else {
            result->value.bool_value = false;
        }
    } 
    else if (strcmp(op, "NEQ") == 0) {      // Not equal to
        if (a->type == OBJECT_TYPE_INT && b->type == OBJECT_TYPE_INT) {
            result->value.bool_value = a->value.int_value != b->value.int_value;
        } 
        else if (a->type == OBJECT_TYPE_FLOAT && b->type == OBJECT_TYPE_FLOAT) {
            result->value.bool_value = a->value.float_value != b->value.float_value;
        } 
        else if (a->type == OBJECT_TYPE_BOOL && b->type == OBJECT_TYPE_BOOL) {
            result->value.bool_value = a->value.bool_value != b->value.bool_value;
        } else {
            result->value.bool_value = false;
        }
    } 
    else {                                  // default = false
        result->value.bool_value = false;
    }

    return result;
}

Object* objectValueAssign(Object* dest, Object* val) {
    return false;
}

Object* objectNotBinaryExpression(Object* dest) {
    dest->value.int_value = ~dest->value.int_value;
    return dest;
}

Object* objectNegExpression(Object* dest) {
    if (dest->type == OBJECT_TYPE_INT) {
        dest->value.int_value = -(dest->value.int_value);
    }
    else if (dest->type == OBJECT_TYPE_FLOAT) {
        dest->value.float_value = -(dest->value.float_value);
    }  
    return dest;
}
Object* objectNotExpression(Object* dest) {
    dest->value.bool_value = !dest->value.bool_value;
    return dest;
}

Object* objectIncAssign(Object* a) {
    a->value.int_value++;
    return a;
}

Object* objectDecAssign(Object* a) {
    a->value.int_value--;
    return a;
}

Object* objectCast(ObjectType variableType, Object* dest) {
    Object *result = malloc(sizeof(Object));

    if (variableType == OBJECT_TYPE_INT) {
        if (result->type == OBJECT_TYPE_FLOAT) {
            result->value.int_value = (int)dest->value.float_value;
        } else if (result->type == OBJECT_TYPE_BOOL) {
            result->value.int_value = dest->value.bool_value ? 1 : 0;
        } else if (result->type == OBJECT_TYPE_STR) {
            result->value.int_value = atoi(dest->value.str_value);
        }
        result->type = OBJECT_TYPE_INT;
        printf("Cast to int\n");
    } 
    else if (variableType == OBJECT_TYPE_FLOAT) {
        if (result->type == OBJECT_TYPE_INT) {
            result->value.float_value = (float)dest->value.int_value;
        } else if (result->type == OBJECT_TYPE_BOOL) {
            result->value.float_value = dest->value.bool_value ? 1.0f : 0.0f;
        } else if (result->type == OBJECT_TYPE_STR) {
            result->value.float_value = atof(dest->value.str_value);
        }
        result->type = OBJECT_TYPE_FLOAT;
        printf("Cast to float\n");
    }
    
    return result;
}

Object* callFunction(char* funcName){
    Object* func = findVariable(funcName);
    Object* result = (Object*)malloc(sizeof(Object));
    result->symbol = (SymbolData *)malloc(sizeof(SymbolData));
    result->type = func->value.funcType;
    result->symbol->name = strdup(func->symbol->name);
    result->symbol->addr = func->symbol->addr;
    result->symbol->func_sig = func->symbol->func_sig;
    printf("call: %s%s\n", funcName, result->symbol->func_sig);
    return result;
}

void funcSigAssign(){
    scopeLevel--;
    Object* func = findVariable(functionName);
    scopeLevel++;
    func->symbol->func_sig = buildJniDescriptor();
}

char* buildJniDescriptor() {
    char* jni = (char*)malloc(100 * sizeof(char));
    int length = 0;
    jni[0] = '\0';
    strcat(jni, "(");
    for (int i = 0; i < funcParamNum; i++) {
        if(funcParam[i]->type == OBJECT_TYPE_INT){
            strcat(jni,"I");
            length++;
        }
        else if(funcParam[i]->type == OBJECT_TYPE_FLOAT){
            strcat(jni,"F");
            length++;
        }
        else if(funcParam[i]->type == OBJECT_TYPE_BOOL){
            strcat(jni,"B");
            length++;
        }
        else if(funcParam[i]->type == OBJECT_TYPE_STR){
            //if(strcmp(funcParam[i]->symbol->name, "argv") == 0){
            if(funcParam[i]->value.int_value == -1){
                strcat(jni,"[Ljava/lang/String;");
                length += 19;
            }        
            else{
                strcat(jni,"Ljava/lang/String;");
                length += 18;
            }
        }
    }
    strcat(jni, ")");
    if(variableIdentType == OBJECT_TYPE_INT){
        strcat(jni, "I");
    }
    else if(variableIdentType == OBJECT_TYPE_FLOAT){
        strcat(jni, "F");
    }
    else if(variableIdentType == OBJECT_TYPE_BOOL){
        strcat(jni, "B");
    }
    else if(variableIdentType == OBJECT_TYPE_VOID){
        strcat(jni, "V");
    }    

    return jni;
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