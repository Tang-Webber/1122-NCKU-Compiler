%{
    #define YY_NO_UNPUT
    #define YY_NO_INPUT
    #define printToken(token, value) (value ? printf("%-8s \t %s\n", token, (char*)value) : printf("%s\n", token))
    #define printComment(token, value) (value ? printf("%-8s \t %s", token, (char*)value) : printf("%s", token))
    
    FILE* fout;
    int in_multi_comment = 0; // Flag to track if in multi-line comment state
%}

%option yylineno

/* Define regular expression label */
letter [a-zA-Z_]
digit [0-9]

/* Rules section */
%x IN_MULTI_COMMENT

%%
"//".*              { printToken("COMMENT", yytext); }
'.'                 { printToken("CHAR_LIT", yytext); }
\"[^\"\n]*\"        { printToken("STRING_LIT", yytext); }

"cout"              { printToken("COUT", NULL); }
"auto"              { printToken("AUTO_T", NULL); }
"void"              { printToken("VOID_T", NULL); }
"char"              { printToken("CHAR_T", NULL); }
"int"               { printToken("INT_T", NULL); }
"long"              { printToken("LONG_T", NULL); }
"float"             { printToken("FLOAT_T", NULL); }
"double"            { printToken("DOUBLE_T", NULL); }
"bool"              { printToken("BOOL_T", NULL); }
"string"            { printToken("STR_T", NULL); }
"if"                { printToken("IF", NULL); }
"else"              { printToken("ELSE", NULL); }
"for"               { printToken("FOR", NULL); }
"while"             { printToken("WHILE", NULL); }
"return"            { printToken("RETURN", NULL); }
"break"             { printToken("BREAK", NULL); }
"true"              { printToken("BOOL_LIT", yytext); }
"false"             { printToken("BOOL_LIT", yytext); }
"<<"                { printToken("SHL", NULL); }
">>"                { printToken("SHR", NULL); }
"&"                 { printToken("BAN", NULL); }
"|"                 { printToken("BOR", NULL); }
"~"                 { printToken("BNT", NULL); }
"^"                 { printToken("BXO", NULL); }
"+"                 { printToken("ADD", NULL); }
"-"                 { printToken("SUB", NULL); }
"*"                 { printToken("MUL", NULL); }
"/"                 { printToken("DIV", NULL); }
"%"                 { printToken("MOD", NULL); }
"!"                 { printToken("NOT", NULL); }
">"                 { printToken("GTR", NULL); }
"<"                 { printToken("LES", NULL); }
">="                { printToken("GEQ", NULL); }
"<="                { printToken("LEQ", NULL); }
"=="                { printToken("EQL", NULL); }
"!="                { printToken("NEQ", NULL); }
"&&"                { printToken("LAN", NULL); }
"||"                { printToken("LOR", NULL); }
"="                 { printToken("VAL_ASSIGN", NULL); }
"+="                { printToken("ADD_ASSIGN", NULL); }
"-="                { printToken("SUB_ASSIGN", NULL); }
"*="                { printToken("MUL_ASSIGN", NULL); }
"/="                { printToken("DIV_ASSIGN", NULL); }
"%="                { printToken("REM_ASSIGN", NULL); }
">>="               { printToken("SHR_ASSIGN", NULL); }
"<<="               { printToken("SHL_ASSIGN", NULL); }
"&="                { printToken("BAN_ASSIGN", NULL); }
"|="                { printToken("BOR_ASSIGN", NULL); }
"++"                { printToken("INC_ASSIGN", NULL); }
"--"                { printToken("DEC_ASSIGN", NULL); }
"("                 { printToken("'('", NULL); }
")"                 { printToken("')'", NULL); }
"["                 { printToken("'['", NULL); }
"]"                 { printToken("']'", NULL); }
"{"                 { printToken("'{'", NULL); }
"}"                 { printToken("'}'", NULL); }
":"                 { printToken("':'", NULL); }
";"                 { printToken("';'", NULL); }
","                 { printToken("','", NULL); }
{digit}+            { printToken("INT_LIT", yytext); }
{digit}+"."{digit}* { printToken("FLOAT_LIT", yytext); }
{letter}({letter}|{digit})* { printToken("IDENT", yytext); }
"/*"                { yymore(); BEGIN(IN_MULTI_COMMENT); }
\n                  {}
.                   {}
<IN_MULTI_COMMENT>{
"*/"                { printToken("MULTI_COMMENT", yytext); BEGIN(INITIAL); }
[^*]+               { yymore(); }
"*"                 { yymore(); }
}

%%

/* C Code section */
int yywrap(void) {
    return 1;
}

int main(int argc, char* argv[]) {
    char *inputFileName = NULL;
    if (argc == 2) {
        yyin = fopen(inputFileName = argv[1], "r");
    } else {
        yyin = stdin;
    }
    if (!yyin) {
        printf("file `%s` doesn't exist or cannot be opened\n", inputFileName);
        exit(1);
    }

    yylex();
    printf("\nTotal line: %d\n", yylineno);

    fclose(yyin);
    return 0;
}
