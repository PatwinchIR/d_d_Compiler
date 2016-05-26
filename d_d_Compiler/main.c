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
#define VARIABLEACCESS 100
#define SIMPLEEXP 100
#define OPER 100
#define TERMNUM 100

typedef struct idnode {
    char *name;
    int type;
    int offset;
    int datatype;
    int datatypesize;
    int intval;
    float floatval;
    char *strval;
    struct ST * next_ST;
    struct idnode * next_hash;
}Identifier;

struct ST{
    struct ST * previous;
    char *name;
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
typedef struct tokennode{
    char *tokenname;
    int pos;
    int type;
    int varname;
    int val;
    int intval;
    float floatval;
    char *strval;
}Token;

Token *tokenScanned;
char *token;


char *errtoken;

// used in parse();
Token *look_ahead;

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

int Type[DATATYPENUM] = {
    INTTYPE, REALTYPE, STRTYPE
};

// identifiers who's datatype field has not been filled.
Token identifierList[TOBEDATATYPED];
Token *idlst = identifierList;
Token *idlstp = identifierList;

// for simple_expression;
Token variableAccessList[VARIABLEACCESS];
Token *varacc = variableAccessList;
Token *varaccp = variableAccessList;

Token simpleExpressionList[SIMPLEEXP];
Token *splex = simpleExpressionList;
Token *splexp = simpleExpressionList;

Token addOperatorList[OPER];
Token *addoper = addOperatorList;
Token *addoperp = addOperatorList;

Token termList[TERMNUM];
Token *termlst = termList;
Token *termlstp = termList;

Token mulOperatorList[OPER];
Token *muloper = mulOperatorList;
Token *muloperp = mulOperatorList;

//for string/int/real/array type variables;
int typeDenoter = 0;
int datatypeSize = 0;

//for subprocesses;
Token currentSubProc;

//for array type variables;
int currentArraySize = 0;

//for gencode;
int currentTemp = 0;

Token nonTerminos[108];

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
Token *scan_token();
int parse();

struct ST* mktable(struct ST * previous);
void enter(struct ST * table, Token vartoken, int vardatatype, int vardatatypesize, int varoffset);
void addwidth(struct ST * table, int width);
void enterproc(struct ST * table, Token subproc, struct ST * subtable);
int newtemp();

int main(int argc, const char * argv[]) {
    initialiseKeywordTable();
    pasfile = fopen("/Users/d_d/Desktop/CompilerPrinciple/hello.txt", "r");
    parse();
    return 0;
}

struct ST* mktable(struct ST * previous){
    int i, pos;
    struct ST * newTable;
    newTable = (struct ST *)malloc(sizeof(struct ST));
    newTable->previous = previous;
    newTable->name = currentSubProc.tokenname;
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
        newTable->SymbolTableBody[i]->datatypesize = -1;
        newTable->SymbolTableBody[i]->intval = 0;
        newTable->SymbolTableBody[i]->floatval = 0;
        newTable->SymbolTableBody[i]->strval = "";
        newTable->SymbolTableBody[i]->offset = -1;
        newTable->SymbolTableBody[i]->next_ST = NULL;
        newTable->SymbolTableBody[i]->next_hash = NULL;
    }
    for (i = 0; i < DATATYPENUM; i++) {
        pos = hashpjw(TypeIdentifier[i]);
        newTable->SymbolTableBody[pos]->name = TypeIdentifier[i];
        newTable->SymbolTableBody[pos]->type = Type[i];
        newTable->SymbolTableBody[pos]->datatypesize = TypeWidth[i];
    }
    return newTable;
}

void enter(struct ST * table, Token vartoken, int vardatatype, int vardatatypesize, int varoffset){
    int pos;
    Identifier * temp;
    pos = vartoken.pos;
    temp = table->SymbolTableBody[pos];
    while (temp) {
        if (temp->datatypesize == -1) {
            temp->datatype = vardatatype;
            temp->datatypesize = vardatatypesize;
            temp->offset = varoffset;
        }
        temp = temp->next_hash;
    }
}

void addwidth(struct ST * table, int width){
    table->space_taken = width;
}

void enterproc(struct ST * table, Token subproc, struct ST * subtable){
    Identifier * temp;
    temp = table->SymbolTableBody[subproc.pos];
    while (temp) {
        if (temp->datatypesize == -1) {
            table->SymbolTableBody[subproc.pos]->next_ST = subtable;
        }
        temp = temp->next_hash;
    }
}

int newtemp(){
    int temp;
    currentTemp ++;
    temp = currentTemp;
    return temp;
}

int parse() {
    int state; /* current state */
    int n;
    int i;
    int varname;
    
    //for print debug;
    int k;
    
    /* State Stack*/
    int ssa[INITDEPTH];
    int *ss = ssa; /* Bottom of state stack */
    int *ssp; /* Top of state stack */
    
    /* Semantic Value Stack */
    Token vsa[INITDEPTH];
    Token *vs = vsa; /* Bottom of semantic value stack */
    Token *vsp; /* Top of semantic value stack */
    
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
        n = ParsingTable[*ssp][look_ahead->type];
        if (n > 0 && n != 1353) {
            vsp ++;
            *vsp = *look_ahead;
            
            ssp ++;
            *ssp = n;
            
            printf("\nEntering State %d\nToken: %s\nType: %d\nNext Token: ", *ssp, look_ahead->tokenname, look_ahead->type);
            
            look_ahead = scan_token();
            
//            //for dbg; declaration translation.
//            temp = *tblptrp;
//            if (temp) {
//                for (k = 0; k < BUCKETS; k++) {
//                    if (temp->SymbolTableBody[k]->offset != -1) {
//                        printf("pos: %-3d ", k);
//                        printf("name: %-10s ", temp->SymbolTableBody[k]->name);
//                        printf("type: %-5d ", temp->SymbolTableBody[k]->type);
//                        printf("datatypesize: %-5d ", temp->SymbolTableBody[k]->datatypesize);
//                        printf("value: ");
//                        switch (temp->SymbolTableBody[k]->datatype) {
//                            case INTTYPE:
//                                printf("%-10d ", temp->SymbolTableBody[k]->intval);
//                                break;
//                                
//                            case REALTYPE:
//                                printf("%-10f ", temp->SymbolTableBody[k]->floatval);
//                                break;
//                                
//                            case STRTYPE:
//                                printf("%-10s ", temp->SymbolTableBody[k]->strval);
//                                break;
//                                
//                            default:
//                                break;
//                        }
//                        printf("offset: %-5d\n", temp->SymbolTableBody[k]->offset);
//                    }
//                }
//            }
        }else if (n < 0) {
            //printf("Reduce using %d: %s\n",-n-1, ProductionTable[-n-1]);
            switch (-n-1) {
                case 2:
                    tblptrp ++;
                    *tblptrp = mktable(NULL);
                    offsetp ++;
                    *offsetp = 0;
                    idlstp = idlst;
                    break;
                    
                case 3:
                    nonTerminos[identifier] = *vsp;
                    break;
                    
                case 5: //program -> program_heading semicolon block DOT
                    addwidth(*tblptrp, *offsetp);
                    tblptrp --;
                    offsetp --;
                    break;
                    
                case 9:
                    idlstp ++;
                    *idlstp = nonTerminos[identifier];
                    break;
                    
                case 14: //m1 -> EPSILON
                    tblptrp ++;
                    *tblptrp = mktable(NULL);
                    offsetp ++;
                    *offsetp = 0;
                    idlstp = idlst;
                    break;
                    
                case 20:
                    idlstp ++;
                    *idlstp = nonTerminos[identifier];
                    break;
                    
                case 34:
                    nonTerminos[unsigned_constant] = nonTerminos[unsigned_number];
                    break;
                    
                case 35:
                    nonTerminos[unsigned_number] = nonTerminos[unsigned_integer];
                    break;
                    
                case 36:
                    nonTerminos[unsigned_number] = nonTerminos[unsigned_real];
                    break;
                    
                case 38:
                    nonTerminos[unsigned_constant] = *vsp;
                    
                case 39:
                    nonTerminos[unsigned_integer] = *vsp;
                    break;
                    
                case 40:
                    nonTerminos[sign] = *vsp;
                    break;
                    
                case 43:
                    nonTerminos[unsigned_real] = *vsp;
                    break;
                    
                case 48: //n1 -> EPSILON
                    currentSubProc = nonTerminos[identifier];
                    temp = mktable(*tblptrp);
                    tblptrp ++;
                    *tblptrp = temp;
                    
                    offsetp ++;
                    *offsetp = 0;
                    break;
                    
                case 61:
                    temp = *tblptrp;
                    typeDenoter = temp->SymbolTableBody[vsp->pos]->type;
                    datatypeSize = TypeWidth[typeDenoter];
                    break;
                    
                case 71:
                    nonTerminos[addop] = *vsp;
                    addoperp ++;
                    *addoperp = nonTerminos[addop];
                    break;
                    
                case 74:
                    nonTerminos[addop] = *vsp;
                    addoperp ++;
                    *addoperp = nonTerminos[addop];
                    break;
                    
                case 75:
                    nonTerminos[mulop] = *vsp;
                    muloperp ++;
                    *muloperp = nonTerminos[mulop];
                    break;
                
                case 76:
                    nonTerminos[mulop] = *vsp;
                    muloperp ++;
                    *muloperp = nonTerminos[mulop];
                    break;
                    
                case 83:
                    nonTerminos[variable_access] = nonTerminos[identifier];
                    temp = *tblptrp;
                    if (temp->SymbolTableBody[nonTerminos[variable_access].pos]->datatypesize == -1) {
                        printf("\nERROR[UNDEFINED], variable %s not defined.\n", nonTerminos[variable_access].tokenname);
                        return 0;
                    }else{
                        nonTerminos[variable_access].val = temp->SymbolTableBody[nonTerminos[variable_access].pos]->datatype;
                        nonTerminos[variable_access].intval = temp->SymbolTableBody[nonTerminos[variable_access].pos]->intval;
                        nonTerminos[variable_access].floatval = temp->SymbolTableBody[nonTerminos[variable_access].pos]->floatval;
                    }
                    varaccp ++;
                    *varaccp = nonTerminos[variable_access];
                    break;
                    
                case 115: //variable_declaration -> identifier_list COLON type_denoter
                    while (idlstp != idlst) {
                        enter(*tblptrp, *idlstp, typeDenoter, datatypeSize, *offsetp);
                        *offsetp += datatypeSize;
                        idlstp --;
                    }
                    break;
                    
                case 128:
                    nonTerminos[primary] = nonTerminos[unsigned_constant];
                    
                    break;
                    
                case 129:
                    nonTerminos[primary] = nonTerminos[variable_access];
                    varaccp --;
                    nonTerminos[variable_access] = *varaccp;
                    break;
                    
                case 131:
                    nonTerminos[expression] = nonTerminos[simple_expression];
                    splexp --;
                    
                    break;
                    
                case 132:
                    nonTerminos[simple_expression] = nonTerminos[term];
                    splexp ++;
                    *splexp = nonTerminos[simple_expression];
                    
                    termlstp --;
                    break;
                    
                case 133:
                    nonTerminos[term] = nonTerminos[factor];
                    termlstp ++;
                    *termlstp = nonTerminos[term];
                    break;
                    
                case 134:
                    nonTerminos[factor] = nonTerminos[primary];
                    break;
                    
                case 138: //procedure_declaration -> procedure_heading semicolon n1 procedure_block
                    temp = *tblptrp;
                    addwidth(temp, *offsetp);
                    tblptrp --;
                    offsetp --;
                    enterproc(*tblptrp, currentSubProc, temp);
                    break;
                    
                case 142: //subrange_type -> constant DOTDOT constant
                    currentArraySize = vsp->intval;
                    break;
                    
                case 149:
                    temp = *tblptrp;
                    if(temp->SymbolTableBody[nonTerminos[variable_access].pos]->datatype == nonTerminos[expression].val){
                        
                        // gencode;
                        varname = nonTerminos[expression].varname;
                        printf("%s := ", temp->SymbolTableBody[nonTerminos[variable_access].pos]->name);
                        if (varname == 0) {
                            printf("%s\n", nonTerminos[expression].tokenname);
                        }else{
                            printf("t%d\n", nonTerminos[expression].varname);
                        }
                        currentTemp = 0;
                        
                        switch (nonTerminos[expression].val) {
                            case INTTYPE:
                                temp->SymbolTableBody[nonTerminos[variable_access].pos]->intval = nonTerminos[expression].intval;
                                break;
                                
                            case REALTYPE:
                                temp->SymbolTableBody[nonTerminos[variable_access].pos]->floatval = nonTerminos[expression].floatval;
                                break;
                                
                            case STRTYPE:
                                temp->SymbolTableBody[nonTerminos[variable_access].pos]->strval = nonTerminos[expression].strval;
                                break;
                                
                            default:
                                break;
                        }
                        
                    }else{
                        printf("\nERROR[TYPE], assign type %s to type %s\n",TypeIdentifier[nonTerminos[expression].val], TypeIdentifier[temp->SymbolTableBody[nonTerminos[variable_access].pos]->datatype]);
                        return 0;
                    }
                    break;
                    
                case 153:
                    if (nonTerminos[sign].type == MINUS) {
                        // gencode;
                        nonTerminos[factor].varname = newtemp();
                        printf("t%d := minus %s\n", nonTerminos[factor].varname, nonTerminos[factor].tokenname);
                        
                        // calculation;
                        nonTerminos[factor].intval = -nonTerminos[factor].intval;
                        nonTerminos[factor].floatval = -nonTerminos[factor].floatval;
                    }
                    
                    break;
                    
                case 162:
                    nonTerminos[simple_expression] = *splexp;
                    nonTerminos[addop] = *addoperp;
                    nonTerminos[term] = *termlstp;
                    
                    // gencode;
                    varname = nonTerminos[simple_expression].varname;
                    nonTerminos[simple_expression].varname = newtemp();
                    if (varname == 0) {
                        printf("t%d := %s %s", nonTerminos[simple_expression].varname, nonTerminos[simple_expression].tokenname, nonTerminos[addop].tokenname);
                    }else{
                        printf("t%d := t%d %s", nonTerminos[simple_expression].varname, varname, nonTerminos[addop].tokenname);
                    }
                    varname = nonTerminos[term].varname;
                    if (varname == 0) {
                        printf(" %s\n", nonTerminos[term].tokenname);
                    }else{
                        printf(" t%d\n", nonTerminos[term].varname);
                    }
                        
                    if (nonTerminos[simple_expression].val == nonTerminos[term].val) {
                        if (nonTerminos[addop].type == PLUS) {
                            switch (nonTerminos[term].val) {
                                case INTTYPE:
                                    nonTerminos[simple_expression].intval = nonTerminos[simple_expression].intval + nonTerminos[term].intval;
                                    break;
                                    
                                case REALTYPE:
                                    nonTerminos[simple_expression].floatval = nonTerminos[simple_expression].floatval + nonTerminos[term].floatval;
                                    break;
                                    
                                default:
                                    break;
                            }
                        }else if (nonTerminos[addop].type == MINUS){
                            switch (nonTerminos[term].val) {
                                case INTTYPE:
                                    nonTerminos[simple_expression].intval = nonTerminos[simple_expression].intval - nonTerminos[term].intval;
                                    break;
                                    
                                case REALTYPE:
                                    nonTerminos[simple_expression].floatval = nonTerminos[simple_expression].floatval - nonTerminos[term].floatval;
                                    break;
                                    
                                default:
                                    break;
                            }
                        }
                    }else{
                        printf("\nERROR[TYPE].\n");
                        return 0;
                    }
                    *splexp = nonTerminos[simple_expression];
                    
                    termlstp --;
                    
                    addoperp --;
                    nonTerminos[addop] = *addoperp;
                    break;
                    
                case 164:
                    nonTerminos[term] = *termlstp;
                    nonTerminos[mulop] = *muloperp;
                    
                    // gencode;
                    varname = nonTerminos[term].varname;
                    nonTerminos[term].varname = newtemp();
                    if (varname == 0) {
                        printf("t%d := %s %s", nonTerminos[term].varname, nonTerminos[term].tokenname, nonTerminos[mulop].tokenname);
                    }else{
                        printf("t%d := t%d %s", nonTerminos[term].varname, varname, nonTerminos[mulop].tokenname);
                    }
                    varname = nonTerminos[factor].varname;
                    if (varname == 0) {
                        printf(" %s\n", nonTerminos[factor].tokenname);
                    }else{
                        printf(" t%d\n", nonTerminos[factor].varname);
                    }
                    
                    if (nonTerminos[term].val == nonTerminos[factor].val) {
                        if (nonTerminos[mulop].type == STAR) {
                            switch (nonTerminos[factor].val) {
                                case INTTYPE:
                                    nonTerminos[term].intval = nonTerminos[term].intval * nonTerminos[factor].intval;
                                    break;
                                    
                                case REALTYPE:
                                    nonTerminos[term].floatval = nonTerminos[term].floatval * nonTerminos[factor].floatval;
                                    break;
                                    
                                default:
                                    break;
                            }
                        }else if (nonTerminos[mulop].type == SLASH){
                            switch (nonTerminos[factor].val) {
                                case INTTYPE:
                                    nonTerminos[term].intval = nonTerminos[term].intval / nonTerminos[factor].intval;
                                    break;
                                    
                                case REALTYPE:
                                    nonTerminos[term].floatval = nonTerminos[term].floatval / nonTerminos[factor].floatval;
                                    break;
                                    
                                default:
                                    break;
                            }
                        }
                    }else{
                        printf("\nERROR[TYPE].\n");
                    }
                    *termlstp = nonTerminos[term];
                    
                    muloperp --;
                    nonTerminos[mulop] = *muloperp;
                    break;
                    
                case 165:
                    nonTerminos[primary] = nonTerminos[expression];
                    break;
                    
                case 176: //array_type -> ARRAY LBRAC index_list RBRAC OF component_type
                    datatypeSize = TypeWidth[typeDenoter] * currentArraySize;
                    break;
                    
                default:
                    break;
            }
            
            
            POPSTACK(ProductionRightLENGTH[-n-1]);
            state = *ssp;
            
            vsp ++;
            vsp->type = ProductionLeftPOS[-n-1];
            
            ssp ++;
            *ssp = ParsingTable[state][ProductionLeftPOS[-n-1]];
            
//            printf("Type: %d\n", vsp->type);
            
        }else if (n == 1353) {
            
            printf("\nCongrats! ACCEPT !\n\n");
            return 1;
        }else {
            printf("\nError before %s where shouldn't exist a(an) %s!\n\n",errtoken, Terminos[look_ahead->type]);
            return 0;
        }
    }
    return 0;
}

Token *scan_token() {
    char *temp = NULL;
    int spenum, pos;
    char check;
    errtoken = (char *)malloc(TOKENLENGTH * sizeof(char));
    token = (char *)malloc(TOKENLENGTH * sizeof(char));
    tokenScanned = (Token *)malloc(sizeof(Token));
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
            pos = install_id(token);
            
            tokenScanned->tokenname = token;
            tokenScanned->type = spenum;
            tokenScanned->pos = pos;
            tokenScanned->varname = 0;
            tokenScanned->val = -1;
            tokenScanned->floatval = 0;
            tokenScanned->intval = 0;
            tokenScanned->strval = "";
            
            printf("(%d, %d)\n", spenum, pos);
            return tokenScanned;
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
                    
                    tokenScanned->tokenname = token;
                    tokenScanned->type = HEX;
                    tokenScanned->pos = 0;
                    tokenScanned->varname = 0;
                    tokenScanned->val = HEX;
                    tokenScanned->floatval = 0;
                    tokenScanned->intval = install_hex(token);
                    tokenScanned->strval = "";
                    
                    printf("(%d, %d)\n", HEX, tokenScanned->intval);
                    return tokenScanned;
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
                    
                    tokenScanned->tokenname = token;
                    tokenScanned->type = OCT;
                    tokenScanned->pos = 0;
                    tokenScanned->varname = 0;
                    tokenScanned->val = OCT;
                    tokenScanned->floatval = 0;
                    tokenScanned->intval = install_oct(token);
                    tokenScanned->strval = "";
                    
                    printf("(%d, %d)\n", OCT, tokenScanned->intval);
                    return tokenScanned;
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
                    
                    tokenScanned->tokenname = token;
                    tokenScanned->type = REALNUMBER;
                    tokenScanned->pos = 0;
                    tokenScanned->varname = 0;
                    tokenScanned->val = REALTYPE;
                    tokenScanned->floatval = install_real(token);
                    tokenScanned->intval = 0;
                    tokenScanned->strval = "";
                    
                    printf("(%d, %f)\n", REALNUMBER, tokenScanned->floatval);
                    return tokenScanned;
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
                    
                    tokenScanned->tokenname = token;
                    tokenScanned->type = REALNUMBER;
                    tokenScanned->pos = 0;
                    tokenScanned->varname = 0;
                    tokenScanned->val = REALTYPE;
                    tokenScanned->floatval = install_real(token);
                    tokenScanned->intval = 0;
                    tokenScanned->strval = "";
                    
                    printf("(%d, %f)\n", REALNUMBER, tokenScanned->floatval);
                    return tokenScanned;
                }else if (ch == '.' &&  check == '.'){
                    fseek(pasfile, -2, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    
                    tokenScanned->tokenname = token;
                    tokenScanned->type = DIGSEQ;
                    tokenScanned->pos = 0;
                    tokenScanned->varname = 0;
                    tokenScanned->val = INTTYPE;
                    tokenScanned->floatval = 0;
                    tokenScanned->intval = install_int(token);
                    tokenScanned->strval = "";
                    
                    printf("(%d, %d)\n", DIGSEQ, tokenScanned->intval);
                    return tokenScanned;
                }else {
                    fseek(pasfile, -2, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    
                    tokenScanned->tokenname = token;
                    tokenScanned->type = DIGSEQ;
                    tokenScanned->pos = 0;
                    tokenScanned->varname = 0;
                    tokenScanned->val = INTTYPE;
                    tokenScanned->floatval = 0;
                    tokenScanned->intval = install_int(token);
                    tokenScanned->strval = "";
                    
                    printf("(%d, %d)\n", DIGSEQ, tokenScanned->intval);
                    return tokenScanned;
                }
            }

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
            
            tokenScanned->tokenname = token;
            tokenScanned->type = CHARACTER_STRING;
            tokenScanned->pos = 0;
            tokenScanned->varname = 0;
            tokenScanned->val = STRTYPE;
            tokenScanned->floatval = 0;
            tokenScanned->intval = 0;
            tokenScanned->strval = install_str(token);
            
            printf("(%d, %s)\n", CHARACTER_STRING, tokenScanned->strval);
            
            ch = fgetc(pasfile);
            
            return tokenScanned;
        }else{
            tokenScanned->pos = 0;
            tokenScanned->varname = 0;
            tokenScanned->val = -1;
            tokenScanned->floatval = 0;
            tokenScanned->intval = 0;
            tokenScanned->strval = "";
            switch (ch) {
                case '*':
                    ch = fgetc(pasfile);
                    if (ch == '*') {
                        errtoken = "**";
                        
                        tokenScanned->tokenname = "**";
                        tokenScanned->type = STARSTAR;
                        
                        printf("** (%d, %d)\n", STARSTAR, 0);
                        return tokenScanned;
                    }else{
                        fseek(pasfile, -1, SEEK_CUR);
                        
                        errtoken = "*";
                        
                        tokenScanned->tokenname = "*";
                        tokenScanned->type = STAR;
                        
                        printf("*  (%d, %d)\n", STAR, 0);
                        return tokenScanned;
                    }
                    break;
                case '.':
                    ch = fgetc(pasfile);
                    if (ch == '.') {
                        errtoken = "..";
                        
                        tokenScanned->tokenname = "..";
                        tokenScanned->type = DOTDOT;
                        
                        printf(".. (%d, %d)\n", DOTDOT, 0);
                        return tokenScanned;
                    }else if (ch != EOF) {
                        fseek(pasfile, -1, SEEK_CUR);
                        errtoken = ".";
                        
                        tokenScanned->tokenname = ".";
                        tokenScanned->type = DOT;
                        
                        printf(".  (%d, %d)\n", DOT, 0);
                        return tokenScanned;
                    }else {
                        errtoken = ".";
                        
                        tokenScanned->tokenname = ".";
                        tokenScanned->type = DOT;
                        
                        printf(".  (%d, %d)\n", DOT, 0);
                        return tokenScanned;
                    }
                    break;
                case ':':
                    ch = fgetc(pasfile);
                    if (ch == '=') {
                        errtoken = ":=";
                        
                        tokenScanned->tokenname = ":=";
                        tokenScanned->type = ASSIGNMENT;
                        
                        printf(":= (%d, %d)\n", ASSIGNMENT, 0);
                        return tokenScanned;
                    }else{
                        fseek(pasfile, -1, SEEK_CUR);
                        errtoken = ":";
                        
                        tokenScanned->tokenname = ":";
                        tokenScanned->type = COLON;
                        
                        printf(":  (%d, %d)\n", COLON, 0);
                        return tokenScanned;
                    }
                    break;
                case '<':
                    ch = fgetc(pasfile);
                    if (ch == '=') {
                        errtoken = "<=";
                        
                        tokenScanned->tokenname = "<=";
                        tokenScanned->type = LE;
                        
                        printf("<= (%d, %d)\n", LE, 0);
                        return tokenScanned;
                    }else if (ch == '>'){
                        errtoken = "<>";
                        
                        tokenScanned->tokenname = "<>";
                        tokenScanned->type = NOTEQUAL;
                        
                        printf("<> (%d, %d)\n", NOTEQUAL, 0);
                        return tokenScanned;
                    }else{
                        fseek(pasfile, -1, SEEK_CUR);
                        errtoken = "<";
                        
                        tokenScanned->tokenname = "<";
                        tokenScanned->type = LT;
                        
                        printf("<  (%d, %d)\n", LT, 0);
                        return tokenScanned;
                    }
                    break;
                case '=':
                    errtoken = "=";
                    
                    tokenScanned->tokenname = "=";
                    tokenScanned->type = EQUAL;
                    
                    printf("=  (%d, %d)\n", EQUAL, 0);
                    return tokenScanned;
                    break;
                case '>':
                    ch = fgetc(pasfile);
                    if (ch == '=') {
                        errtoken = ">=";
                        
                        tokenScanned->tokenname = ">=";
                        tokenScanned->type = GE;
                        
                        printf(">= (%d, %d)\n", GE, 0);
                        return tokenScanned;
                    }else{
                        fseek(pasfile, -1, SEEK_CUR);
                        errtoken = ">";
                        
                        tokenScanned->tokenname = ">";
                        tokenScanned->type = GT;
                        
                        printf(">  (%d, %d)\n", GT, 0);
                        return tokenScanned;
                    }
                    break;
                case '+':
                    errtoken = "+";
                    
                    tokenScanned->tokenname = "+";
                    tokenScanned->type = PLUS;
                    
                    printf("+  (%d, %d)\n", PLUS, 0);
                    return tokenScanned;
                    break;
                case '-':
                    errtoken = "-";
                    
                    tokenScanned->tokenname = "-";
                    tokenScanned->type = MINUS;
                    
                    printf("-  (%d, %d)\n", MINUS, 0);
                    return tokenScanned;
                    break;
                case '/':
                    errtoken = "/";
                    
                    tokenScanned->tokenname = "/";
                    tokenScanned->type = SLASH;
                    
                    printf("/  (%d, %d)\n", SLASH, 0);
                    return tokenScanned;
                    break;
                case ',':
                    errtoken = ",";
                    
                    tokenScanned->tokenname = ",";
                    tokenScanned->type = COMMA;
                    
                    printf(",  (%d, %d)\n", COMMA, 0);
                    return tokenScanned;
                    break;
                case ';':
                    errtoken = ";";
                    
                    tokenScanned->tokenname = ";";
                    tokenScanned->type = SEMICOLON;
                    
                    printf(";  (%d, %d)\n", SEMICOLON, 0);
                    return tokenScanned;
                    break;
                case '(':
                    errtoken = "(";
                    
                    tokenScanned->tokenname = "(";
                    tokenScanned->type = LPAREN;
                    
                    printf("(  (%d, %d)\n", LPAREN, 0);
                    return tokenScanned;
                    break;
                case ')':
                    errtoken = ")";
                    
                    tokenScanned->tokenname = ")";
                    tokenScanned->type = RPAREN;
                    
                    printf(")  (%d, %d)\n", RPAREN, 0);
                    return tokenScanned;
                    break;
                case '[':
                    errtoken = "[";
                    
                    tokenScanned->tokenname = "[";
                    tokenScanned->type = LBRAC;
                    
                    printf("[  (%d, %d)\n", LBRAC, 0);
                    return tokenScanned;
                    break;
                case ']':
                    errtoken = "]";
                    
                    tokenScanned->tokenname = "]";
                    tokenScanned->type = RBRAC;
                    
                    printf("]  (%d, %d)\n", RBRAC, 0);
                    return tokenScanned;
                    break;
                    
                default:
                    printf("ERROR!\n");
                    break;
            }
        }
    }
    fclose(pasfile);
    
    tokenScanned->tokenname = "ACCEPT";
    tokenScanned->type = ACCEPT;
    tokenScanned->pos = 0;
    tokenScanned->varname = 0;
    tokenScanned->val = -1;
    tokenScanned->floatval = 0;
    tokenScanned->intval = 0;
    tokenScanned->strval = "";
    
    printf("(End of file...)\n");
    return tokenScanned;
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
        }else if(!isDatatype(str)){
            idnodetemp = sttemp->SymbolTableBody[pos];
            while (idnodetemp->next_hash) {
                idnodetemp = idnodetemp->next_hash;
            }
            idnodetemp->next_hash = (Identifier *)malloc(sizeof(Identifier));
            idnodetemp->next_hash->name = str;
            idnodetemp->next_hash->type = IDENTIFIER;
            idnodetemp->next_hash->datatype = -1;
            idnodetemp->next_hash->intval = 0;
            idnodetemp->next_hash->floatval = 0;
            idnodetemp->next_hash->strval = "";
            idnodetemp->next_hash->datatypesize = -1;
            idnodetemp->next_hash->offset = -1;
            idnodetemp->next_hash->next_hash = NULL;
            idnodetemp->next_hash->next_ST = NULL;
        }
    }
    //To store those identifiers with the same datatype temperarily.
    return pos;
}

int install_int(char *str) {
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
}
