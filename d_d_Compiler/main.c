//
//  main.c
//  d_d_Compiler
//
//  Created by d_d on 3/16/16.
//  Copyright © 2016 ddapp. All rights reserved.
//

//#include <stdio.h>
//
//int main(int argc, const char * argv[]) {
//    // insert code here...
//    printf("Hello, World!\n");
//    return 0;
//}

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "notagain.h"

#define BUCKETS 907
#define EOS '\0'
#define KEYWORDNUM 22
#define TOKENLENGTH 100
#define INITDEPTH 10000
#define EMPTY -2
#define FEOF 0
#define ACCEPT 51
#define TABLES 100
#define DATATYPENUM 3

#define TOBEDATATYPED 100

typedef struct idnode {
    char *name;
    int type;
    int offset;
    int datatype;
    struct ST * next_ST;
    struct idnode * next_hash;
}Identifier;

struct ST{
    struct ST * previous;
    int depth;
    int space_taken;
    Identifier * SymbolTableBody[BUCKETS];
};

typedef struct keywordnode{
    char *name;
    int type;
}Keyword;

Keyword *KeywordTable[BUCKETS];

//Identifier *SymbolTableBody[BUCKETS];

struct ST SymbolTable;

//used in scan_token();
FILE *pasfile;
char ch;
char *token;

char *errtoken;

// used in parse();
int look_ahead;

char *KeyWord[KEYWORDNUM] = {
    "program", "begin", "end", "if", "then", "else",
    "array", "const", "do", "downto", "for",
    "nil", "repeat", "to", "until", "while", "var", "of", "label" , "goto",
    "procedure", "function"
};

int KeyWordCode[KEYWORDNUM] = {PROGRAM, PBEGIN, END, IF, THEN, ELSE,
    ARRAY, CONST, DO, DOWNTO, FOR, NIL, REPEAT, TO, UNTIL, WHILE, VAR, OF, LABEL, GOTO,
    PROCEDURE, FUNCTION
};

char *TypeIdentifier[DATATYPENUM] = {
    "Integer", "Real", "String"
};

int TypeWidth[DATATYPENUM] = {
    INTWIDTH, REALWIDTH, STRWIDTH
};

// identifiers who's datatype field has not been filled.
int tobeDatatyped[TOBEDATATYPED];
int *tobe = tobeDatatyped;
int *tobep = tobeDatatyped;

//for string/int/real type variables;
int currentDatatype = 0;

//for subprocesses;
int currentSubProc = -1;

//for array type variables;
int currentArray = 0;
int currentArraySize = 0;

//table pointer stack.
struct ST *tblptrstk[TABLES];
struct ST **tblptr = tblptrstk;
struct ST **tblptrp;

//offset stack.
int offsetstk[TABLES];
int *offset = offsetstk;
int *offsetp;

int hashpjw(char *str);
int gettoken(char *str);
int install_id(char *str);
int install_int(char *str);
float install_real(char *str);
int install_oct(char *str);
int install_hex(char *str);
char* install_str(char *str);
void initialiseKeywordTable();
void cleantoken();
int scan_token();
int parse();

struct ST* mktable(struct ST * previous);
void enter(struct ST * table, int datatype);
void addwidth(struct ST * table, int width);
void enterproc(struct ST * table, int subproc, struct ST * subtable);

int main(int argc, const char * argv[]) {
    initialiseKeywordTable();
    pasfile = fopen("/Users/d_d/Desktop/CompilerPrinciple/hello.txt", "r");
    parse();
    return 0;
}

struct ST* mktable(struct ST * previous){
    int i, pos, *temp;
    struct ST * newTable;
    //To process newTable's name...
    if (tobep != tobe) {
        temp = tobe;
        temp ++;
        currentSubProc = *temp;
        tobep = tobe;
    }
    
    newTable = (struct ST *)malloc(sizeof(struct ST));
    newTable->previous = previous;
    if (previous) {
        newTable->depth = previous->depth + 1;
    }else{
        newTable->depth = 0;
    }
    newTable->space_taken = -1;
    for (i = 0; i < BUCKETS; i++) {
        newTable->SymbolTableBody[i] = (Identifier *)malloc(sizeof(Identifier));
        newTable->SymbolTableBody[i]->name = "";
        newTable->SymbolTableBody[i]->type = -1;
        newTable->SymbolTableBody[i]->datatype = -1;
        newTable->SymbolTableBody[i]->offset = -1;
        newTable->SymbolTableBody[i]->next_ST = NULL;
        newTable->SymbolTableBody[i]->next_hash = NULL;
    }
    for (i = 0; i < DATATYPENUM; i++) {
        pos = hashpjw(TypeIdentifier[i]);
        newTable->SymbolTableBody[pos]->name = TypeIdentifier[i];
        newTable->SymbolTableBody[pos]->type = TypeWidth[i];
    }
    return newTable;
}

void enter(struct ST * table, int datatype){
    int pos;
    Identifier * temp;
    while (tobep != tobe) {
        pos = *tobep;
        temp = table->SymbolTableBody[pos];
        while (temp) {
            if (temp->datatype == -1) {
                temp->datatype = datatype;
                temp->offset = *offsetp;
                if (currentArray) {
                    printf("currentArray: %d\ncurrentArraySize: %d\n", currentArray, currentArraySize);
                    *offsetp = *offsetp + datatype * currentArraySize;
                    currentArray = 0;
                    currentArraySize = 0;
                }else{
                    *offsetp += datatype;
                }
                tobep --;
            }
            temp = temp->next_hash;
        }
    }
}

void addwidth(struct ST * table, int width){
    table->space_taken = width;
}

void enterproc(struct ST * table, int subproc, struct ST * subtable){
    Identifier * temp;
    temp = table->SymbolTableBody[subproc];
    while (temp) {
        if (temp->datatype == -1) {
            table->SymbolTableBody[subproc]->next_ST = subtable;
        }
        temp = temp->next_hash;
    }
}

int parse() {
    int state; /* current state */
    int n;
    int i;
    
    //for print debug;
    int k;
    
    /* State Stack*/
    int ssa[INITDEPTH];
    int *ss = ssa; /* Bottom of state stack */
    int *ssp; /* Top of state stack */
    
    /* Semantic Value Stack */
    int vsa[INITDEPTH];
    int *vs = vsa; /* Bottom of state stack */
    int *vsp; /* Top of state stack */
    
    struct ST *temp;
    
    #define POPSTACK(N)   (vsp -= (N), ssp -= (N))
    
    state = 0;
    
    
    for (i = 0; i < TABLES; i ++) {
        tblptrstk[i] = NULL;
    }
    tblptrp = tblptr;
    offsetp = offset;
    
    /* Top = bottom for stacks */
    ssp = ss;
    vsp = vs;
    
    ssp ++;
    *ssp = state;
    printf("Entering State %d\n", *ssp);
    
    look_ahead = scan_token();
    
    while (1) {
        n = ParsingTable[*ssp][look_ahead];
        if (n > 0 && n != 1353) {
            vsp ++;
            *vsp = look_ahead;
            
            ssp ++;
            *ssp = n;
            
            printf("\nEntering State %d\nNext Token: ", *ssp);
            
            look_ahead = scan_token();
            
            //for dbg; declaration translation.
            temp = *tblptrp;
            if (temp) {
                for (k = 0; k < BUCKETS; k++) {
                    if (temp->SymbolTableBody[k]->datatype != -1) {
                        printf("name: %-5s ", temp->SymbolTableBody[k]->name);
                        printf("type: %d ", temp->SymbolTableBody[k]->type);
                        printf("datatype: %d ", temp->SymbolTableBody[k]->datatype);
                        printf("offset: %d\n", temp->SymbolTableBody[k]->offset);
                    }
                }
            }
            
            if (look_ahead == 0) {
                currentArray = 1;
            }
            
            
        }else if (n < 0) {
            POPSTACK(ProductionRightLENGTH[-n-1]);
            state = *ssp;
            
            vsp ++;
            *vsp = ProductionLeftPOS[-n-1];
            
            ssp ++;
            *ssp = ParsingTable[state][ProductionLeftPOS[-n-1]];
            
            printf("Reduce using %d: %s\n",-n-1, ProductionTable[-n-1]);
            
            switch (-n-1) {
                case 5: //program -> program_heading semicolon block DOT
                    addwidth(*tblptrp, *offsetp);
                    tblptrp --;
                    offsetp --;
                    break;
                    
                case 14: //m1 -> EPSILON
                    tblptrp ++;
                    *tblptrp = mktable(NULL);
                    offsetp ++;
                    *offsetp = 0;
                    break;
                    
                case 48: //n1 -> EPSILON
                    temp = mktable(*tblptrp);
                    tblptrp ++;
                    *tblptrp = temp;
                    
                    offsetp ++;
                    *offsetp = 0;
                    break;
                    
                case 115: //variable_declaration -> identifier_list COLON type_denoter
                    enter(*tblptrp, currentDatatype);
                    break;
                    
                case 138: //procedure_declaration -> procedure_heading semicolon n1 procedure_block
                    temp = *tblptrp;
                    addwidth(temp, *offsetp);
                    tblptrp --;
                    offsetp --;
                    enterproc(*tblptrp, currentSubProc, temp);
                    break;
                    
                case 149:
                    
                    break;
                    
                default:
                    break;
            }
            
        }else if (n == 1353) {
            
            printf("\nCongrats! ACCEPT !\n\n");
            return 1;
        }else {
            printf("\nError before %s where shouldn't exist a(an) %s!\n\n",errtoken, Terminos[look_ahead]);
            return 0;
        }
    }
    return 0;
}

int scan_token() {
    char *temp = NULL;
    int spenum;
    char check;
    errtoken = (char *)malloc(TOKENLENGTH * sizeof(char));
    token = (char *)malloc(TOKENLENGTH * sizeof(char));
    while((ch = fgetc(pasfile)) != '\xff') {
        while (ch == ' ' || ch == '\t' || ch == '\n') {
            ch = fgetc(pasfile);
        }
        if (isalpha(ch)) {
            temp = token;
            while (isalnum(ch)) {
                *temp++ = ch;
                ch = fgetc(pasfile);
            }
            fseek(pasfile, -1, SEEK_CUR);
            strcpy(errtoken, token);
            printf("%s ",token);
            spenum = gettoken(token);
            printf("(%d, %d)\n", spenum, install_id(token));
            cleantoken();
            return spenum;
        }else if (isdigit(ch)) {
            temp = token;
            if (ch == '0') {
                *temp++ = ch;
                ch = fgetc(pasfile);
                if (ch == 'x') {
                    *temp++ = ch;
                    while ((ch >= 48 && ch <= 57) || (ch >= 65 && ch <=70) || (ch >= 97 && ch <=102)) {
                        *temp++ = ch;
                        ch = fgetc(pasfile);
                    }
                    fseek(pasfile, -1, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    printf("(%d, %d)\n", HEX, install_hex(token));
                    return HEX;
                }else if (isdigit(ch)){
                    if (ch >= 48 && ch <= 55) {
                        ch = fgetc(pasfile);
                        *temp++ = ch;
                        while (ch >= 48 && ch <= 55) {
                            *temp++ = ch;
                            ch = fgetc(pasfile);
                        }
                    }
                    fseek(pasfile, -1, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    printf("(%d, %d)\n", OCT, install_oct(token));
                    return OCT;
                }else if (ch == '.') {
                    *temp++ = ch;
                    ch = fgetc(pasfile);
                    while (isdigit(ch)) {
                        *temp++ = ch;
                        ch = fgetc(pasfile);
                    }
                    fseek(pasfile, -1, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    printf("(%d, %f)\n", REALNUMBER, install_real(token));
                    return REALNUMBER;
                }
            }else if (ch >= 49 && ch <= 57) {
                while (isdigit(ch)) {
                    *temp++ = ch;
                    ch = fgetc(pasfile);
                }
                check = fgetc(pasfile);
                if (ch == '.' &&  check != '.') {
                    *temp++ = ch;
                    ch = fgetc(pasfile);
                    while (isdigit(ch)) {
                        *temp++ = ch;
                        ch = fgetc(pasfile);
                    }
                    fseek(pasfile, -1, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    printf("(%d, %f)\n", REALNUMBER, install_real(token));
                    return REALNUMBER;
                }else if (ch == '.' &&  check == '.'){
                    fseek(pasfile, -2, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    printf("(%d, %d)\n", DIGSEQ, install_int(token));
                    return DIGSEQ;
                }else {
                    fseek(pasfile, -2, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    printf("(%d, %d)\n", DIGSEQ, install_int(token));
                    return DIGSEQ;
                }
            }
            cleantoken();
        }else if (ch == '\'') {
            temp = token;
            ch = fgetc(pasfile);
            while (ch != '\'') {
                *temp++ = ch;
                ch = fgetc(pasfile);
            }
            fseek(pasfile, -1, SEEK_CUR);
            strcpy(errtoken, token);
            printf("%s ",token);
            printf("(%d, %s)\n", CHARACTER_STRING, install_str(token));
            cleantoken();
            ch = fgetc(pasfile);
            return CHARACTER_STRING;
        }else{
            switch (ch) {
                case '*':
                    ch = fgetc(pasfile);
                    if (ch == '*') {
                        errtoken = "**";
                        printf("** (%d, %d)\n", STARSTAR, 0);
                        return STARSTAR;
                    }else{
                        fseek(pasfile, -1, SEEK_CUR);
                        errtoken = "*";
                        printf("*  (%d, %d)\n", STAR, 0);
                        return STAR;
                    }
                    break;
                case '.':
                    ch = fgetc(pasfile);
                    if (ch == '.') {
                        errtoken = "..";
                        printf(".. (%d, %d)\n", DOTDOT, 0);
                        return DOTDOT;
                    }else if (ch != EOF) {
                        fseek(pasfile, -1, SEEK_CUR);
                        errtoken = ".";
                        printf(".  (%d, %d)\n", DOT, 0);
                        return DOT;
                    }else {
                        errtoken = ".";
                        printf(".  (%d, %d)\n", DOT, 0);
                        return DOT;
                    }
                    break;
                case ':':
                    ch = fgetc(pasfile);
                    if (ch == '=') {
                        errtoken = ":=";
                        printf(":= (%d, %d)\n", ASSIGNMENT, 0);
                        return ASSIGNMENT;
                    }else{
                        fseek(pasfile, -1, SEEK_CUR);
                        errtoken = ":";
                        printf(":  (%d, %d)\n", COLON, 0);
                        return COLON;
                    }
                    break;
                case '<':
                    ch = fgetc(pasfile);
                    if (ch == '=') {
                        errtoken = "<=";
                        printf("<= (%d, %d)\n", LE, 0);
                        return LE;
                    }else if (ch == '>'){
                        errtoken = "<>";
                        printf("<> (%d, %d)\n", NOTEQUAL, 0);
                        return NOTEQUAL;
                    }else{
                        fseek(pasfile, -1, SEEK_CUR);
                        errtoken = "<";
                        printf("<  (%d, %d)\n", LT, 0);
                        return LT;
                    }
                    break;
                case '=':
                    errtoken = "=";
                    printf("=  (%d, %d)\n", EQUAL, 0);
                    return EQUAL;
                    break;
                case '>':
                    ch = fgetc(pasfile);
                    if (ch == '=') {
                        errtoken = ">=";
                        printf(">= (%d, %d)\n", GE, 0);
                        return GE;
                    }else{
                        fseek(pasfile, -1, SEEK_CUR);
                        errtoken = ">";
                        printf(">  (%d, %d)\n", GT, 0);
                        return GT;
                    }
                    break;
                case '+':
                    errtoken = "+";
                    printf("+  (%d, %d)\n", PLUS, 0);
                    return PLUS;
                    break;
                case '-':
                    errtoken = "-";
                    printf("-  (%d, %d)\n", MINUS, 0);
                    return MINUS;
                    break;
                case '/':
                    errtoken = "/";
                    printf("/  (%d, %d)\n", SLASH, 0);
                    return SLASH;
                    break;
                case ',':
                    errtoken = ",";
                    printf(",  (%d, %d)\n", COMMA, 0);
                    return COMMA;
                    break;
                case ';':
                    errtoken = ";";
                    printf(";  (%d, %d)\n", SEMICOLON, 0);
                    return SEMICOLON;
                    break;
                case '(':
                    errtoken = "(";
                    printf("(  (%d, %d)\n", LPAREN, 0);
                    return LPAREN;
                    break;
                case ')':
                    errtoken = ")";
                    printf(")  (%d, %d)\n", RPAREN, 0);
                    return RPAREN;
                    break;
                case '[':
                    errtoken = "[";
                    printf("[  (%d, %d)\n", LBRAC, 0);
                    return LBRAC;
                    break;
                case ']':
                    errtoken = "]";
                    printf("]  (%d, %d)\n", RBRAC, 0);
                    return RBRAC;
                    break;
                    
                default:
                    printf("ERROR!\n");
                    break;
            }
        }
    }
    fclose(pasfile);
    printf("(End of file...)\n");
    return ACCEPT;
}

int hashpjw(char *str) {
    char *p;
    unsigned h = 0, g;
    for (p = str; *p != EOS; p++) {
        h = (h << 4) + (*p);
        if ((g = h & 0xf0000000)) {
            h = h ^ (g >> 24);
            h = h ^ g;
        }
    }
    return h % BUCKETS;
}

int gettoken(char *str) {
    int pos = hashpjw(str);
    if (KeywordTable[pos]->type != -1) {
        return KeywordTable[pos]->type;
    }
    return IDENTIFIER;
}

int isDatatype(char *str){
    int i;
    for (i = 0; i < DATATYPENUM; i++) {
        if (strcmp(str, TypeIdentifier[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int install_id(char *str) {
    struct ST * sttemp;
    Identifier *idnodetemp;
    int pos, typetemp;
    char *strtemp;
    strtemp = (char *)malloc(100 * sizeof(char));
    strcpy(strtemp, str);
    if (gettoken(str) != IDENTIFIER) {
        return 0;
    }
    if (*tblptrp != NULL) {
        pos = hashpjw(str);
        sttemp = *tblptrp;
        typetemp = sttemp->SymbolTableBody[pos]->type;
        if (typetemp == -1) {
            sttemp->SymbolTableBody[pos]->name = strtemp;
            sttemp->SymbolTableBody[pos]->type = IDENTIFIER;
            sttemp->SymbolTableBody[pos]->next_hash = NULL;
            tobep ++;
            *tobep = pos;
        }else if(isDatatype(str)){
            currentDatatype = typetemp;
        }else if(!isDatatype(str)){
            idnodetemp = sttemp->SymbolTableBody[pos];
            while (idnodetemp->next_hash) {
                idnodetemp = idnodetemp->next_hash;
            }
            idnodetemp->next_hash = (Identifier *)malloc(sizeof(Identifier));
            idnodetemp->next_hash->name = str;
            idnodetemp->next_hash->type = IDENTIFIER;
            idnodetemp->next_hash->datatype = -1;
            idnodetemp->next_hash->offset = -1;
            idnodetemp->next_hash->next_hash = NULL;
            idnodetemp->next_hash->next_ST = NULL;
            tobep ++;
            *tobep = pos;
        }
    }
//            if (SymbolTableBody[pos]->type != IDENTIFIER && SymbolTableBody[pos]->type != -1) {
//                return 0;
//            }else{
//                SymbolTableBody[pos]->name = str;
//                SymbolTableBody[pos]->type = IDENTIFIER;
//                SymbolTableBody[pos]->next_hash = NULL;
//            }
    //To store those identifiers with the same datatype temperarily.
    return pos;
}

int install_int(char *str) {
    if (currentArray) {
        currentArraySize = atoi(str);
    }
    return atoi(str);
}

float install_real(char *str) {
    return atof(str);
}

int install_oct(char *str) {  //haven't finished yet
    return atoi(str);
}

int install_hex(char *str) {  //haven't finished yet
    return atoi(str);
}

char* install_str(char *str) {
    return str;
}

void initialiseKeywordTable() {
    int i, pos;
    for (i = 0; i < BUCKETS; i++) {
        KeywordTable[i] = (Keyword *)malloc(sizeof(Keyword));
        KeywordTable[i]->name = "";
        KeywordTable[i]->type = -1;
    }
    for (i = 0; i < KEYWORDNUM; i++) {
        pos = hashpjw(KeyWord[i]);
        KeywordTable[pos]->name = KeyWord[i];
        KeywordTable[pos]->type = KeyWordCode[i];
    }
//    for (i = 0; i < BUCKETS; i++) {
//        SymbolTableBody[i] = (Identifier *)malloc(sizeof(Identifier));
//        SymbolTableBody[i]->name = "";
//        SymbolTableBody[i]->type = -1;
//        SymbolTableBody[i]->next_hash = NULL;
//    }
//    for (i = 0; i < KEYWORDNUM; i++) {
//        pos = hashpjw(KeyWord[i]);
//        SymbolTableBody[pos]->name = KeyWord[i];
//        SymbolTableBody[pos]->type = KeyWordCode[i];
//        SymbolTableBody[pos]->next_hash = NULL;
//    }
}

void cleantoken() {
    char *temp;
    int i;
    temp = token;
    for (i = 0; i < TOKENLENGTH; i++) {
        *temp++ = '\0';
    };
}
