#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define WJCL_LINKED_LIST_IMPLEMENTATION
#include "main.h"
#define WJCL_HASH_MAP_IMPLEMENTATION
//#include "value_operation.h"

#define debug printf("%s:%d: ############### debug\n", __FILE__, __LINE__)

#define iload(var) code("iload %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)
#define lload(var) code("lload %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)
#define fload(var) code("fload %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)
#define dload(var) code("dload %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)
#define aload(var) code("aload %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)

#define istore(var) code("istore %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)
#define lstore(var) code("lstore %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)
#define fstore(var) code("fstore %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)
#define dstore(var) code("dstore %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)
#define astore(var) code("astore %" PRId64 " ; %s", (var)->symbol->addr, (var)->symbol->name)

#define ldz(val) code("ldc %d", getBool(val))
#define ldb(val) code("ldc %d", getByte(val))
#define ldc(val) code("ldc %d", getChar(val))
#define lds(val) code("ldc %d", getShort(val))
#define ldi(val) code("ldc %d", getInt(val))
#define ldl(val) code("ldc_w %" PRId64, getLong(val))
#define ldf(val) code("ldc %.6f", getFloat(val))
#define ldd(val) code("ldc_w %lf", getDouble(val))
#define ldt(val) code("ldc \"%s\"", val)

#define storeVariable(var)                                                     \
  if ((var)->type == OBJECT_TYPE_FLOAT) {                                      \
    fstore(var);                                                               \
  } else if ((var)->type == OBJECT_TYPE_INT ||                                 \
             (var)->type == OBJECT_TYPE_BOOL) {                                \
    istore(var);                                                               \
  } else if ((var)->type == OBJECT_TYPE_LONG) {                                \
    lstore(var);                                                               \
  } else if ((var)->type == OBJECT_TYPE_DOUBLE) {                              \
    dstore(var);                                                               \
  } else if ((var)->type == OBJECT_TYPE_STR) {                                 \
    astore(var);                                                               \
  }

#define loadVariable(var)                                                      \
  if ((var)->type == OBJECT_TYPE_FLOAT) {                                      \
    fload(var);                                                                \
  } else if ((var)->type == OBJECT_TYPE_INT ||                                 \
             (var)->type == OBJECT_TYPE_BOOL) {                                \
    iload(var);                                                                \
  } else if ((var)->type == OBJECT_TYPE_LONG) {                                \
    lload(var);                                                                \
  } else if ((var)->type == OBJECT_TYPE_DOUBLE) {                              \
    dload(var);                                                                \
  } else if ((var)->type == OBJECT_TYPE_STR) {                                 \
    aload(var);                                                                \
  }

const char* objectTypeName[] = {
    [OBJECT_TYPE_UNDEFINED] = "undefined",
    [OBJECT_TYPE_VOID] = "void",
    [OBJECT_TYPE_BOOL] = "bool",
    [OBJECT_TYPE_BYTE] = "byte",
    [OBJECT_TYPE_CHAR] = "char",
    [OBJECT_TYPE_SHORT] = "short",
    [OBJECT_TYPE_INT] = "int",
    [OBJECT_TYPE_LONG] = "long",
    [OBJECT_TYPE_FLOAT] = "float",
    [OBJECT_TYPE_DOUBLE] = "double",
    [OBJECT_TYPE_STR] = "string",
    [OBJECT_TYPE_FUNCTION] = "function",
};
const char* objectJavaTypeName[] = {
    [OBJECT_TYPE_UNDEFINED] = "V",
    [OBJECT_TYPE_VOID] = "V",
    [OBJECT_TYPE_BOOL] = "Z",
    [OBJECT_TYPE_BYTE] = "B",
    [OBJECT_TYPE_CHAR] = "C",
    [OBJECT_TYPE_SHORT] = "S",
    [OBJECT_TYPE_INT] = "I",
    [OBJECT_TYPE_LONG] = "J",
    [OBJECT_TYPE_FLOAT] = "F",
    [OBJECT_TYPE_DOUBLE] = "D",
    [OBJECT_TYPE_STR] = "Ljava/lang/String;",
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
bool pushInStack = true;
bool variableDeclare = false;

Object* table[100][100];
int table_index[100];
Object *queue[100];
int queue_num = 0;
Object *funcParam[100];
int funcParamNum = 0;

bool load_var = false;
int label = 0;
int ifLayer = 0;

bool inIfStmt = false;
bool inElseStmt = false;
char* ifStmt[10];
char* elseStmt[10];

bool arrayInitStmt = false;
Object* arrayTemp = NULL;

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

void returnExpr(){
    pushInStack = false;
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

void arrayEndStmt(char* arrayName){
    Object* temp = findVariable(arrayName);
    arrayInitStmt = false;
    code("astore %d", (int)temp->symbol->addr);
}

void arrayEndCout(){
    arrayInitStmt = false;
}

void arrayStartStmt(){
    arrayInitStmt = true;
}

Object* createArray(ObjectType variableType, char* variableName, Object* size){
    //arrayInitStmt = true;

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
    if(strcmp(variableName, "argv") != 0){
        code("newarray %s", objectTypeName[newObject->type]);    
    }
   
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
    codeRaw("dup");
    code("ldc %d", arrayIndex); 
    code("ldc %d", arrayTemp->value.int_value);
    codeRaw("iastore");
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
    if(type != OBJECT_TYPE_FUNCTION)
        variableDeclare = true;
}

void assignVariable(char* varName, char* op, Object* expr){

    Object* target = findVariable(varName);
    //printf("----%s | %s | %s ----\n", varName, expr->symbol->name, objectTypeName[expr->type]);
    if (target->type == OBJECT_TYPE_AUTO){
        target->type = expr->type;
    }
    if (target->type == OBJECT_TYPE_INT) {
        if(expr->type == OBJECT_TYPE_FLOAT){
            codeRaw("f2i");
        }
        if(strcmp("EQL", op) == 0){         // =
        }else if(strcmp("ADD", op) == 0){    // +=
            codeRaw("iadd");
        }else if(strcmp("SUB", op) == 0){    // -=
            codeRaw("isub");
        }else if(strcmp("MUL", op) == 0){    // *=
            codeRaw("imul");
        }else if(strcmp("DIV", op) == 0){    // /=
            codeRaw("idiv");
        }else if(strcmp("REM", op) == 0){    // %=
            codeRaw("irem");
        }else if(strcmp("BOR", op) == 0){    // |=
            codeRaw("ior");
        }else if(strcmp("BAN", op) == 0){    // &=
            codeRaw("iand");
        }else if (strcmp("SHR", op) == 0){ 
            codeRaw("ishr");
        }else if (strcmp("SHL", op) == 0){
            codeRaw("ishl");
        }
        storeVariable(target);
    }
    if (target->type == OBJECT_TYPE_FLOAT) {
        if(expr->type == OBJECT_TYPE_INT){
            codeRaw("i2f");
        }
        if(strcmp("VAL", op) == 0){         // =
        }else if(strcmp("ADD", op) == 0){    // +=
            codeRaw("fadd");
        }else if(strcmp("SUB", op) == 0){    // -=
            codeRaw("fsub");
        }else if(strcmp("MUL", op) == 0){    // *=
            codeRaw("fmul");
        }else if(strcmp("DIV", op) == 0){    // /=
            codeRaw("fdiv");
        }
        storeVariable(target);
    }
    if (target->type == OBJECT_TYPE_BOOL) {
        if(strcmp("VAL", op) == 0){         // =
        }else if(strcmp("BOR", op) == 0){    // |=
        }else if(strcmp("BAN", op) == 0){    // &=
        }
        storeVariable(target);
    }
    if (target->type == OBJECT_TYPE_STR) {
        if(strcmp("VAL", op) == 0){         // =
        }else if(strcmp("ADD", op) == 0){   // +=

        }   
        storeVariable(target); 
    }
}

void assignArrayVariable(char* op, Object* expr){
    Object* target = findVariable(arrayName);
    int idx = arrayIndex;
    if (idx >= 0 && idx < target->value.array_value.length) {
        //codeRaw("dup");
        //code("ldc %d", idx);  
        codeRaw("iastore");      
        if (strcmp("EQL", op) == 0){ // =
            target->value.array_value.elements[idx] = expr->value.int_value;
        }        
            
    }
}

void loadArray(char* name){
    Object* temp = findVariable(name);
    code("aload %d", (int)temp->symbol->addr);
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
        newObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));
        newObject->symbol->name = strdup("int");
        printf("INT_LIT %d\n", newObject->value.int_value);   
        if(pushInStack){
            //codeRaw("getstatic java/lang/System/out Ljava/io/PrintStream;");
            if(arrayInitStmt){
                arrayTemp = newObject;                
            }
            else{
                code("ldc %d", newObject->value.int_value); 
            }
            //codeRaw("invokevirtual java/io/PrintStream/print(I)V");
        }
    }
    if(strcmp(variableType, "f") == 0){
        newObject->type = OBJECT_TYPE_FLOAT;
        newObject->value.float_value = atof(str);
        newObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));
        newObject->symbol->name = strdup("float");
        printf("FLOAT_LIT %.6f\n", newObject->value.float_value);
        if(pushInStack){
            //codeRaw("getstatic java/lang/System/out Ljava/io/PrintStream;");
            code("ldc %f", newObject->value.float_value);    
            //codeRaw("invokevirtual java/io/PrintStream/print(F)V");        
        }
    }
    if(strcmp(variableType, "b") == 0){
        newObject->type = OBJECT_TYPE_BOOL;
        newObject->value.bool_value = (strcmp(str, "TRUE") == 0);
        newObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));
        newObject->symbol->name = strdup("bool");
        printf("BOOL_LIT %s\n", str);
        if(pushInStack){
            //codeRaw("getstatic java/lang/System/out Ljava/io/PrintStream;");
            //code("ldc %d", (int)newObject->value.bool_value);  
            if(newObject->value.bool_value){
                codeRaw("iconst_1");
            }
            else{
                codeRaw("iconst_0");
            }
            //codeRaw("invokevirtual java/io/PrintStream/print(Z)V");     
        }    
    }
    if(strcmp(variableType, "s") == 0){
        newObject->type = OBJECT_TYPE_STR;
        newObject->value.str_value = strdup(str);
        newObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));
        newObject->symbol->name = strdup(str);
        printf("STR_LIT \"%s\"\n", newObject->value.str_value);
        //ldt(newObject->value.str_value);
        if(pushInStack){
            //codeRaw("getstatic java/lang/System/out Ljava/io/PrintStream;");
            code("ldc \"%s\"", newObject->value.str_value);      
            //codeRaw("invokevirtual java/io/PrintStream/print(Ljava/lang/String;)V");      
        }
    }
    if(strcmp(variableType, "c") == 0){
        newObject->type = OBJECT_TYPE_CHAR;
        newObject->value.char_value = str[0];
        newObject->symbol = (SymbolData *)malloc(sizeof(SymbolData));
        newObject->symbol->name = strdup("char");
        printf("CHAR_LIT \"%c\"\n", newObject->value.char_value);
        if(pushInStack){
            //codeRaw("getstatic java/lang/System/out Ljava/io/PrintStream;");
            code("bipush %d", (int)(newObject->value.char_value));    
            //codeRaw("invokevirtual java/io/PrintStream/print(C)V");        
        }
    }

    return newObject;
}

void printIdent(Object* ident){
    printf("IDENT (name=%s, address=%ld)\n", ident->symbol->name, ident->symbol->addr); 
}

void other_assign(){
    load_var = true;
}
void val_assign(){
    load_var = false;
}
void ifStart(){
    inIfStmt = true;
}
void ifEnd(){
    inIfStmt = false;
}
void printLoad(Object* temp){
    if(temp != NULL){
        if(load_var){
            if(strcmp("endl", temp->symbol->name) != 0){

                loadVariable(temp);   
            }
        } 
    }
    load_var = false;
}

void printInvoke(Object* Expr){
    if(strcmp("string", objectTypeName[Expr->type]) == 0){
        if(strcmp("endl", Expr->symbol->name) == 0){
            codeRaw("invokevirtual java/io/PrintStream/println()V");
        }
        else{
            //ldt(queue[i]->value.str_value);
            codeRaw("invokevirtual java/io/PrintStream/print(Ljava/lang/String;)V");
        }
    }
    if(strcmp("char", objectTypeName[Expr->type]) == 0){
        //code("bipush %d", (int)(queue[i]->value.char_value));
        codeRaw("invokevirtual java/io/PrintStream/print(C)V");
    }
    if(strcmp("int", objectTypeName[Expr->type]) == 0){
        //code("ldc %d", queue[i]->value.int_value);
        codeRaw("invokevirtual java/io/PrintStream/print(I)V");
    }
    if(strcmp("float", objectTypeName[Expr->type]) == 0){
        //code("ldc %f", queue[i]->value.float_value);
        codeRaw("invokevirtual java/io/PrintStream/print(F)V");
    }        
    if(strcmp("bool", objectTypeName[Expr->type]) == 0){    
        codeRaw("invokevirtual java/io/PrintStream/print(Z)V");
    }
}

Object* findVariable(char* str){
    Object *temp = (Object *)malloc(sizeof(Object));
    if(str == NULL)
        return NULL;
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
    return NULL;
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

    code("aload %d", (int)array->symbol->addr);
    code("ldc %d", arrayTemp->value.int_value);
    codeRaw("iaload");

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
    //codeRaw("return");
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
        //codeRaw("getstatic java/lang/System/out Ljava/io/PrintStream;");
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
            codeRaw("iconst_1");
            codeRaw("ixor");
        } else if(strcmp(op, "NEG") == 0) {
            result = objectNotBinaryExpression(a);
            if (a->type == OBJECT_TYPE_INT) {
                codeRaw("ineg");
            } 
            else if (a->type == OBJECT_TYPE_FLOAT) {
                codeRaw("fneg");
            } 
        } else if(strcmp(op, "BNT") == 0) {
            result = objectNotBinaryExpression(a);
            codeRaw("iconst_m1");
            codeRaw("ixor");
        } else {
            if ((a->type == OBJECT_TYPE_INT || a->type == OBJECT_TYPE_BOOL) && (b->type == OBJECT_TYPE_INT || b->type == OBJECT_TYPE_BOOL)) {
                result->type = OBJECT_TYPE_INT;
                if (strcmp(op, "ADD") == 0) {
                    codeRaw("iadd");
                } else if (strcmp(op, "SUB") == 0) {
                    codeRaw("isub");
                } else if (strcmp(op, "MUL") == 0) {
                    codeRaw("imul");
                } else if (strcmp(op, "DIV") == 0) {
                    codeRaw("idiv");
                } else if (strcmp(op, "REM") == 0) {
                    codeRaw("irem");
                }
            } else if (a->type == OBJECT_TYPE_FLOAT && b->type == OBJECT_TYPE_FLOAT) {
                result->type = OBJECT_TYPE_FLOAT;
                if (strcmp(op, "ADD") == 0) {
                    codeRaw("fadd");
                } else if (strcmp(op, "SUB") == 0) {
                    codeRaw("fsub");
                } else if (strcmp(op, "MUL") == 0) {
                    codeRaw("fmul");
                } else if (strcmp(op, "DIV") == 0) {
                    codeRaw("fdiv");
                }
            }
        }   
        return a;    
    }
    return a;
}

Object* objectExpBinary(char* op, Object* a, Object* b) {
    Object* result = malloc(sizeof(Object));

    result->type = a->type;
    result->symbol = (SymbolData *)malloc(sizeof(SymbolData));
    result->symbol->name = strdup(objectTypeName[a->type]);
    //printf("@@@%s %s @@@\n", result->symbol->name, objectTypeName[a->type]);

    if (strcmp(op, "BAN") == 0) {           // Bitwise AND
        codeRaw("iand");
    } 
    else if (strcmp(op, "BOR") == 0) {      // Bitwise OR
        codeRaw("ior");
    } 
    else if (strcmp(op, "BXO") == 0) {      // Bitwise XOR
        codeRaw("ixor");
    }
    else if (strcmp(op, "SHL") == 0) {      // Shift Left
        codeRaw("ishl");
    }
    else if (strcmp(op, "SHR") == 0) {      // Shift Right
        codeRaw("ishr");
    }
    return result;
}


Object* objectExpBoolean(char* op, Object* a, Object* b) {
    if (strcmp(op, "LOR") == 0) {
        codeRaw("ior");
    } 
    else if (strcmp(op, "LAN") == 0) {
        codeRaw("iand");
    } 
    else if (strcmp(op, "GTR") == 0) {      // Greater than
        //result->value.bool_value = a->value.float_value > b->value.float_value;
        if (a->type == OBJECT_TYPE_INT && b->type == OBJECT_TYPE_INT) {
            code("if_icmpgt greaterThanLabel%d", label);
            codeRaw("iconst_0");
            code("goto endLabel%d", label);
            code("greaterThanLabel%d:", label);
            codeRaw("iconst_1");
            code("endLabel%d:", label);
            label++;            
        } 
        else if (a->type == OBJECT_TYPE_FLOAT && b->type == OBJECT_TYPE_FLOAT) {
            codeRaw("fcmpg");
            code("ifgt greaterThanLabel%d", label);
            codeRaw("iconst_0");
            code("goto endLabel%d", label);
            code("greaterThanLabel%d:", label);
            codeRaw("iconst_1");
            code("endLabel%d:", label);
            label++;  
        } 

    } 
    else if (strcmp(op, "LES") == 0) {      // Less than
        
    } 
    else if (strcmp(op, "GEQ") == 0) {      // Greater than or equal to
        
    } 
    else if (strcmp(op, "LEQ") == 0) {      // Less than or equal to
        
    } 
    else if (strcmp(op, "EQL") == 0) {      // Equal to
        code("if_icmpeq equalToLabel%d", label);
        codeRaw("iconst_0");
        code("goto endLabel%d", label);
        code("equalToLabel%d:", label);
        codeRaw("iconst_1");
        code("endLabel%d:", label);
        label++;  

    } 
    else if (strcmp(op, "NEQ") == 0) {      // Not equal to
        if (a->type == OBJECT_TYPE_INT && b->type == OBJECT_TYPE_INT) {
            code("if_icmpne notEqualToLabel%d", label);
            codeRaw("iconst_0");
            code("goto endLabel%d", label);
            code("notEqualToLabel%d:", label);
            codeRaw("iconst_1");
            code("endLabel%d:", label);
            label++;            
        } 
        else if (a->type == OBJECT_TYPE_FLOAT && b->type == OBJECT_TYPE_FLOAT) {
            codeRaw("fcmpg");
            code("if_icmpne notEqualToLabel%d", label);
            codeRaw("iconst_0");
            code("goto endLabel%d", label);
            code("notEqualToLabel%d:", label);
            codeRaw("iconst_1");
            code("endLabel%d:", label);
            label++; 
        } 
    } 
    ObjectType temp = OBJECT_TYPE_BOOL; 

    if (strcmp(op, "BAN") == 0) {           // Bitwise AND
        codeRaw("iand");
        temp = a->type;
    } 
    else if (strcmp(op, "BOR") == 0) {      // Bitwise OR
        codeRaw("ior");
        temp = a->type;
    } 
    else if (strcmp(op, "BXO") == 0) {      // Bitwise XOR
        codeRaw("ixor");
        temp = a->type;
    }
    a->type = temp;
    
    return a;
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

//TODO：???????????????????????
Object* objectCast(ObjectType variableType, Object* dest) {
    Object *result = malloc(sizeof(Object));

    if (variableType == OBJECT_TYPE_INT) {
        if (dest->type == OBJECT_TYPE_FLOAT) {
            //result->value.int_value = (int)dest->value.float_value;
            codeRaw("f2i");
        } else if (dest->type == OBJECT_TYPE_BOOL) {
            //result->value.int_value = dest->value.bool_value ? 1 : 0;
        } else if (dest->type == OBJECT_TYPE_STR) {
            //result->value.int_value = atoi(dest->value.str_value);
        }
        result->type = OBJECT_TYPE_INT;
        printf("Cast to int\n");
    } 
    else if (variableType == OBJECT_TYPE_FLOAT) {
        if (dest->type == OBJECT_TYPE_INT) {
            //result->value.float_value = (float)dest->value.int_value;
            codeRaw("i2f");
        } else if (dest->type == OBJECT_TYPE_BOOL) {
            //result->value.float_value = dest->value.bool_value ? 1.0f : 0.0f;
        } else if (dest->type == OBJECT_TYPE_STR) {
            //result->value.float_value = atof(dest->value.str_value);
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
    if(strcmp(functionName, "main" ) == 0){
        codeRaw(".method public static main([Ljava/lang/String;)V");
    }
    else{
        code(".method public static %s%s", functionName, func->symbol->func_sig);
    }
    
    code(".limit stack %d", 100);
    code(".limit locals %d", 100);
}

char* buildJniDescriptor() {
    char* jni = (char*)malloc(100 * sizeof(char));
    //int length = 0;
    jni[0] = '\0';
    strcat(jni, "(");
    for (int i = 0; i < funcParamNum; i++) {
        if(funcParam[i]->type == OBJECT_TYPE_STR){
            //if(strcmp(funcParam[i]->symbol->name, "argv") == 0){
            if(funcParam[i]->value.int_value == -1){
                strcat(jni,"[Ljava/lang/String;");
                //length += 19;
            }        
            else{
                strcat(jni,"Ljava/lang/String;");
                //length += 18;
            }
        }
        else{
            strcat(jni, objectJavaTypeName[funcParam[i]->type]);
            //length ++;            
        }
    }
    strcat(jni, ")");
    strcat(jni, objectJavaTypeName[variableIdentType]);

    return jni;
}

bool returnObject(Object* obj){
    if(strcmp(objectTypeName[obj->type], "int") == 0){
        if(obj->value.int_value == 0){
            //codeRaw("iconst_0");
            //codeRaw("ireturn");
            //codeRaw("return");
        }
        
    }
    else if(strcmp(objectTypeName[obj->type], "void") == 0){
        //codeRaw("return");
    }
    
}

int main(int argc, char* argv[]) {
    char* outputFileName = NULL;
    if (argc == 3) {
        yyin = fopen(yyInputFileName = argv[1], "r");
        yyout = fopen(outputFileName = argv[2], "w");
    } else if (argc == 2) {
        yyin = fopen(yyInputFileName = argv[1], "r");
        yyout = stdout;
    } else {
        printf("require input file");
        exit(1);
    }
    if (!yyin) {
        printf("file `%s` doesn't exists or cannot be opened\n", yyInputFileName);
        exit(1);
    }
    if (!yyout) {
        printf("file `%s` doesn't exists or cannot be opened\n", outputFileName);
        exit(1);
    }
    codeRaw(".source Main.j");
    codeRaw(".class public Main");
    codeRaw(".super java/lang/Object");
    scopeLevel = -1;

    yyparse();
    printf("Total lines: %d\n", yylineno);
    fclose(yyin);

    yylex_destroy();
    return 0;
}