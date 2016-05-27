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
#define KEYWORDNUM 24
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
#define QUADNUM 1000
#define BACKPATCHNUM 100

typedef struct idnode {
    char *name;
    int type;
    int offset;
    int isarray;
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

Keyword *KeywordTable[KEYWORDNUM];

//Identifier *SymbolTableBody[BUCKETS];

struct ST SymbolTable;

//used in scan_token();
FILE *pasfile;
char ch;
typedef struct tokennode{
    char *tokenname;
    int pos;
    int type;
    int offset;
    int varname;
    int val;
    int intval;
    float floatval;
    char *strval;
    
    int quad;
    int *truelist;
    int *falselist;
    int *nextlist;
    int again;
}Token;

Token *tokenScanned;
char *token;

char *errtoken;

// used in parse();
Token *look_ahead;

//gencode;
typedef struct quadnode{
    Token op;
    Token arg1;
    Token arg2;
    Token result;
}Quad;

Quad quadList[QUADNUM];

char *KeyWord[KEYWORDNUM] = {
    "program", "begin", "end", "if", "then", "else",
    "array", "const", "do", "downto", "for",
    "nil", "repeat", "to", "until", "while", "var", "of", "label" , "goto",
    "procedure", "function", "or", "not"
};

int KeyWordCode[KEYWORDNUM] = {PROGRAM, PBEGIN, END, IF, THEN, ELSE,
    ARRAY, CONST, DO, DOWNTO, FOR, NIL, REPEAT, TO, UNTIL, WHILE, VAR, OF, LABEL, GOTO,
    PROCEDURE, FUNCTION, OR, NOT
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

Token relOperatorList[OPER];
Token *reloper = relOperatorList;
Token *reloperp = relOperatorList;

Token arrayList[OPER];
Token *arrlst = arrayList;
Token *arrlstp = arrayList;

Token mList[OPER];
Token *mlst = mList;
Token *mlstp = mList;

Token clsStatement[OPER];
Token *clsstmt = clsStatement;
Token *clsstmtp = clsStatement;

Token stmtSeq[OPER];
Token *stmtseq = stmtSeq;
Token *stmtseqp = stmtSeq;

//for string/int/real/array type variables;
int typeDenoter = 0;
int datatypeSize = 0;
int isArray = 0;

//for subprocesses;
Token currentSubProc;

//for array type variables;
int currentArraySize = 0;

//for gencode;
//FILE *intercode;
int codelabel = 0;
int currentTemp = 0;

Token nonTerminals[108];

//table pointer stack.
struct ST *tblptrstk[TABLES];
struct ST **tblptr = tblptrstk;
struct ST **tblptrp;

//offset stack.
int offsetstk[TABLES];
int *offset = offsetstk;
int *offsetp;

int hashpjw(char *str);
int isKeyWord(char *str);
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

void backpatch(int *booleanlist, int quad);
int *mergelist(int *b1list, int *b2list);

int *makeQuadList(int i);

void printQuad();

int main(int argc, const char * argv[]) {
    initialiseKeywordTable();
    pasfile = fopen("/Users/d_d/Desktop/CompilerPrinciple/hello.txt", "r");
//    intercode = fopen("/Users/d_d/Desktop/CompilerPrinciple/intercode.txt", "w");
    parse();
    printQuad();
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
        newTable->SymbolTableBody[i]->isarray = 0;
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
            if (isArray) {
                temp->isarray = 1;
            }
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

int *makeList(int i){
    int *newList;
    int j;
    newList = (int *)malloc(BACKPATCHNUM * sizeof(int));
    for (j = 0; j < BACKPATCHNUM; j++) {
        newList[j] = -1;
    }
    newList[0] = i;
    return newList;
}

void backpatch(int *booleanlist, int quad){
    int i;
    if(booleanlist == NULL){
        return;
    }
    for (i = 0; i < BACKPATCHNUM; i++) {
        if (booleanlist[i] != -1) {
            quadList[booleanlist[i]].result.quad = quad;
        }else{
            break;
        }
    }
}

int *mergelist(int *b1list, int *b2list){
    int *newList = makeList(-1);
    int i, j;
    if (b1list != NULL) {
        if (b2list != NULL) {
            for (i = 0; i < BACKPATCHNUM; i++) {
                if (b1list[i] != -1) {
                    newList[i] = b1list[i];
                }else{
                    break;
                }
            }
            for (j = 0; j < BACKPATCHNUM; j++) {
                if (b2list[j] != -1) {
                    newList[i + j] = b2list[j];
                }else{
                    break;
                }
            }
        }else{
            for (i = 0; i < BACKPATCHNUM; i++) {
                if (b1list[i] != -1) {
                    newList[i] = b1list[i];
                }else{
                    break;
                }
            }
        }
    }else{
        if (b2list != NULL) {
            for (j = 0; j < BACKPATCHNUM; j++) {
                if (b2list[j] != -1) {
                    newList[j] = b2list[j];
                }else{
                    break;
                }
            }
        }
    }
    return newList;
}

void printQuad(){
    int i;
    Quad temp;
    for (i = 0; i < BACKPATCHNUM; i++) {
        temp = quadList[i];
        if (temp.op.type != 0) {
            printf( "%4d: ", i);
            switch (temp.op.type) {
                case ARRAYADDR:
                    if (temp.result.varname == 0) {
                        printf( "%s := ", temp.result.tokenname);
                    }else{
                        printf( "t%d := ", temp.result.varname);
                    }
                    if (temp.arg1.varname == 0) {
                        printf( "%d", temp.arg1.intval);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    break;
                    
                case STAR:
                    printf( "t%d := ", temp.result.varname);
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " * ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    break;
                    
                case SLASH:
                    printf( "t%d := ", temp.result.varname);
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " / ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    break;
                    
                case ARRAYOFFSET:
                    printf( "t%d := ", temp.result.varname);
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " * %d", temp.arg2.intval);
                    break;
                    
                case ASSIGNMENT:
                    printf( "%s := ", temp.result.tokenname);
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    break;
                    
                case OFFSETASSIGN:
                    printf( "t%d[t%d] := ", temp.result.varname, temp.result.offset);
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    break;
                    
                case MINUSASSIGN:
                    printf( "t%d := minus %s", temp.result.varname, temp.arg1.tokenname);
                    break;
                    
                case PLUS:
                    if (temp.result.varname == 0) {
                        printf( "%s := ", temp.result.tokenname);
                    }else{
                        printf( "t%d := ", temp.result.varname);
                    }
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " + ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    break;
                    
                case MINUS:
                    if (temp.result.varname == 0) {
                        printf( "%s := ", temp.result.tokenname);
                    }else{
                        printf( "t%d := ", temp.result.varname);
                    }
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " - ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    break;
                    
                case LT:
                    printf( "if ");
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " < ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    printf( " goto %d", temp.result.quad);
                    break;
                    
                case GT:
                    printf( "if ");
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " > ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    printf( " goto %d", temp.result.quad);
                    break;
                    
                case GE:
                    printf( "if ");
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " >= ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    printf( " goto %d", temp.result.quad);
                    break;
                    
                case LE:
                    printf( "if ");
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " <= ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    printf( " goto %d", temp.result.quad);
                    break;
                    
                case EQUAL:
                    printf( "if ");
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " = ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    printf( " goto %d", temp.result.quad);
                    break;
                    
                case NOTEQUAL:
                    printf( "if ");
                    if (temp.arg1.varname == 0) {
                        printf( "%s", temp.arg1.tokenname);
                    }else{
                        printf( "t%d", temp.arg1.varname);
                    }
                    printf( " <> ");
                    if (temp.arg2.varname == 0) {
                        printf( "%s", temp.arg2.tokenname);
                    }else{
                        printf( "t%d", temp.arg2.varname);
                    }
                    printf( " goto %d", temp.result.quad);
                    break;
                    
                case GOTO:
                    printf( "goto %d", temp.result.quad);
                    break;
                    
                case OR:
                    printf( "t%d := t%d or t%d", temp.result.varname, temp.arg1.varname, temp.arg2.varname);
                    break;
                    
                default:
                    printf( "%d", temp.op.type);
                    break;
            }
            printf( "\n");
        }
    }
}

int parse() {
    int state; /* current state */
    int n;
    int i;
    int varname, varpos, varoffset;
    int notfound = 0;
    
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
    Token tokentemp;
    Token tokentemp1;
    int newtemptemp;
    int blisttemp[BACKPATCHNUM];
    
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
            
            //for dbg; declaration translation.
            temp = *tblptrp;
            if (temp) {
                for (k = 0; k < BUCKETS; k++) {
                    if (temp->SymbolTableBody[k]->offset != -1) {
                        printf("pos: %-3d ", k);
                        printf("name: %-10s ", temp->SymbolTableBody[k]->name);
                        printf("type: %-5d ", temp->SymbolTableBody[k]->type);
                        printf("datatypesize: %-5d ", temp->SymbolTableBody[k]->datatypesize);
                        printf("value: ");
                        switch (temp->SymbolTableBody[k]->datatype) {
                            case INTTYPE:
                                printf("%-10d ", temp->SymbolTableBody[k]->intval);
                                break;
                                
                            case REALTYPE:
                                printf("%-10f ", temp->SymbolTableBody[k]->floatval);
                                break;
                                
                            case STRTYPE:
                                printf("%-10s ", temp->SymbolTableBody[k]->strval);
                                break;
                                
                            default:
                                break;
                        }
                        printf("offset: %-5d\n", temp->SymbolTableBody[k]->offset);
                    }
                }
            }
        }else if (n < 0) {
            printf("Reduce using %d: %s\n",-n-1, ProductionTable[-n-1]);
            switch (-n-1) {
                case 2: // program_heading -> PROGRAM identifier
                    tblptrp ++;
                    *tblptrp = mktable(NULL);
                    offsetp ++;
                    *offsetp = 0;
                    idlstp = idlst;
                    break;
                    
                case 3: //identifier -> IDENTIFIER
                    nonTerminals[identifier] = *vsp;
                    break;
                    
                case 5: //program -> program_heading semicolon block DOT
                    addwidth(*tblptrp, *offsetp);
                    tblptrp --;
                    offsetp --;
                    break;
                    
                case 9: //identifier_list -> identifier
                    idlstp ++;
                    *idlstp = nonTerminals[identifier];
                    break;
                    
                case 14: //m1 -> EPSILON
                    tblptrp ++;
                    *tblptrp = mktable(NULL);
                    offsetp ++;
                    *offsetp = 0;
                    idlstp = idlst;
                    break;
                    
                case 20: //identifier_list -> identifier_list comma identifier
                    idlstp ++;
                    *idlstp = nonTerminals[identifier];
                    break;
                    
                case 34: //unsigned_constant -> unsigned_number
                    nonTerminals[unsigned_constant] = nonTerminals[unsigned_number];
                    break;
                    
                case 35: //unsigned_number -> unsigned_integer
                    nonTerminals[unsigned_number] = nonTerminals[unsigned_integer];
                    break;
                    
                case 36: //unsigned_number -> unsigned_real
                    nonTerminals[unsigned_number] = nonTerminals[unsigned_real];
                    break;
                    
                case 38: //unsigned_constant -> CHARACTER_STRING
                    nonTerminals[unsigned_constant] = *vsp;
                    
                case 39: //unsigned_integer -> DIGSEQ
                    nonTerminals[unsigned_integer] = *vsp;
                    break;
                    
                case 40: //sign -> MINUS
                    nonTerminals[sign] = *vsp;
                    break;
                    
                case 43: //unsigned_real -> REALNUMBER
                    nonTerminals[unsigned_real] = *vsp;
                    break;
                    
                case 48: //n1 -> EPSILON
                    currentSubProc = nonTerminals[identifier];
                    temp = mktable(*tblptrp);
                    tblptrp ++;
                    *tblptrp = temp;
                    
                    offsetp ++;
                    *offsetp = 0;
                    break;
                    
                case 61: //type_denoter -> identifier
                    temp = *tblptrp;
                    typeDenoter = temp->SymbolTableBody[vsp->pos]->type;
                    datatypeSize = TypeWidth[typeDenoter];
                    break;
                    
                case 66: //relop -> EQUAL;
                    nonTerminals[relop] = *vsp;
                    reloperp ++;
                    *reloperp = nonTerminals[relop];
                    break;
                    
                case 67: //relop -> GE
                    nonTerminals[relop] = *vsp;
                    reloperp ++;
                    *reloperp = nonTerminals[relop];
                    break;
                    
                case 68: //relop -> GT
                    nonTerminals[relop] = *vsp;
                    reloperp ++;
                    *reloperp = nonTerminals[relop];
                    break;
                    
                case 69: //relop -> LE
                    nonTerminals[relop] = *vsp;
                    reloperp ++;
                    *reloperp = nonTerminals[relop];
                    break;
                    
                case 70: //relop -> LT
                    nonTerminals[relop] = *vsp;
                    reloperp ++;
                    *reloperp = nonTerminals[relop];
                    break;
                    
                case 71: //addop -> MINUS
                    nonTerminals[addop] = *vsp;
                    addoperp ++;
                    *addoperp = nonTerminals[addop];
                    break;
                    
                case 72: //relop -> NOTEQUAL
                    nonTerminals[relop] = *vsp;
                    reloperp ++;
                    *reloperp = nonTerminals[relop];
                    break;
                    
                case 73: //m2 -> EPSILON;
                    nonTerminals[m2] = *vsp;
                    nonTerminals[m2].quad = codelabel;
                    break;
                    
                case 74: //addop -> PLUS
                    nonTerminals[addop] = *vsp;
                    addoperp ++;
                    *addoperp = nonTerminals[addop];
                    break;
                    
                case 75: //mulop -> SLASH
                    nonTerminals[mulop] = *vsp;
                    muloperp ++;
                    *muloperp = nonTerminals[mulop];
                    break;
                
                case 76: //mulop -> STAR
                    nonTerminals[mulop] = *vsp;
                    muloperp ++;
                    *muloperp = nonTerminals[mulop];
                    break;
                    
                case 79: //non_labeled_closed_statement -> compound_statement
                    nonTerminals[non_labeled_closed_statement] = nonTerminals[compound_statement];
                    break;
                    
                case 80: //statement_sequence -> statement
                    nonTerminals[statement_sequence].nextlist = nonTerminals[statement].nextlist;
                    break;
                    
                case 81: //statement -> closed_statement
                    nonTerminals[statement] = nonTerminals[closed_statement];
                    clsstmtp --;
                    break;
                    
                case 82: //closed_statement -> non_labeled_closed_statement
                    nonTerminals[closed_statement] = nonTerminals[non_labeled_closed_statement];
                    clsstmtp ++;
                    *clsstmtp = nonTerminals[closed_statement];
                    break;
                    
                case 83: //variable_access -> identifier
                    nonTerminals[variable_access] = nonTerminals[identifier];
                    temp = *tblptrp;
                    do {
                        if (temp->SymbolTableBody[nonTerminals[variable_access].pos]->datatypesize == -1) {
                            notfound = 1;
                            temp = temp->previous;
                        }else{
                            notfound = 0;
                            break;
                        }
                    } while (temp);
                    if (notfound) {
                        printf("\nERROR[UNDEFINED], variable %s not defined.\n", nonTerminals[variable_access].tokenname);
                        return 0;
                    }else{
                        nonTerminals[variable_access].val = temp->SymbolTableBody[nonTerminals[variable_access].pos]->datatype;
                        nonTerminals[variable_access].intval = temp->SymbolTableBody[nonTerminals[variable_access].pos]->intval;
                        nonTerminals[variable_access].floatval = temp->SymbolTableBody[nonTerminals[variable_access].pos]->floatval;
                        if (temp->SymbolTableBody[nonTerminals[variable_access].pos]->isarray) {
                            arrlstp ++;
                            *arrlstp = nonTerminals[variable_access];
                        }
                    }
                    varaccp ++;
                    *varaccp = nonTerminals[variable_access];
                    break;
                    
                case 85: //statement -> open_statement
                    nonTerminals[statement] = nonTerminals[open_statement];
                    break;
                    
                case 86: //open_statement -> non_labeled_open_statement
                    nonTerminals[open_statement] = nonTerminals[non_labeled_open_statement];
                    break;
                    
                case 87: //non_labeled_closed_statement -> assignment_statement
                    nonTerminals[non_labeled_closed_statement] = nonTerminals[assignment_statement];
                    break;
                    
                case 88: //non_labeled_closed_statement -> repeat_statement
                    nonTerminals[non_labeled_closed_statement] = nonTerminals[repeat_statement];
                    break;
                    
                case 90: //non_labeled_closed_statement -> closed_while_statement
                    nonTerminals[non_labeled_closed_statement] = nonTerminals[closed_while_statement];
                    break;
                    
                case 91: //non_labeled_closed_statement -> closed_for_statement
                    nonTerminals[non_labeled_closed_statement] = nonTerminals[closed_for_statement];
                    break;
                    
                case 92: //non_labeled_open_statement -> open_if_statement
                    nonTerminals[non_labeled_open_statement] = nonTerminals[open_if_statement];
                    break;
                    
                case 95: //variable_access -> indexed_variable
                    nonTerminals[variable_access] = nonTerminals[indexed_variable];
                    varaccp ++;
                    *varaccp = nonTerminals[variable_access];
                    break;
                    
                case 98: //m5 -> EPSILON
                    nonTerminals[m5].quad = codelabel;
                    break;
                    
                case 99: //m3 -> EPSILON
                    nonTerminals[m3].quad = codelabel;
                    mlstp ++;
                    *mlstp = nonTerminals[m3];
                    break;
                    
                case 115: //variable_declaration -> identifier_list COLON type_denoter
                    while (idlstp != idlst) {
                        enter(*tblptrp, *idlstp, typeDenoter, datatypeSize, *offsetp);
                        *offsetp += datatypeSize;
                        isArray = 0;
                        idlstp --;
                    }
                    break;
                    
                case 121: //addop -> OR m2
                    nonTerminals[addop] = nonTerminals[m2];
                    nonTerminals[addop].type = OR;
                    addoperp ++;
                    *addoperp = nonTerminals[addop];
                    break;
                    
                case 124: //compound_statement -> PBEGIN statement_sequence END
                    nonTerminals[compound_statement] = nonTerminals[statement_sequence];
                    break;
                    
                case 126: //indexed_variable -> index_expression_list RBRAC
                    nonTerminals[indexed_variable] = nonTerminals[index_expression_list];
                    varpos = newtemp();
                    nonTerminals[indexed_variable].varname = varpos;
                    temp = *tblptrp;
                    do {
                        if (temp->SymbolTableBody[nonTerminals[indexed_variable].pos]->datatypesize == -1) {
                            notfound = 1;
                            temp = temp->previous;
                        }else{
                            notfound = 0;
                            break;
                        }
                    } while (temp);
                    
                    printf("%4d: t%d := %d\n",codelabel , varpos, temp->SymbolTableBody[nonTerminals[indexed_variable].pos]->offset);
                    quadList[codelabel].op.type = ARRAYADDR;
                    quadList[codelabel].arg1.val = INTTYPE;
                    quadList[codelabel].arg1.intval = temp->SymbolTableBody[nonTerminals[indexed_variable].pos]->offset;
                    quadList[codelabel].result.varname = varpos;
                    
                    codelabel ++;
                    
                    varoffset = newtemp();
                    if (nonTerminals[expression].varname == 0) {
                        printf("%4d: t%d := %s * %d\n",codelabel, varoffset, nonTerminals[expression].tokenname, TypeWidth[temp->SymbolTableBody[nonTerminals[indexed_variable].pos]->datatype]);
                    }else{
                        printf("%4d: t%d := t%d * %d\n",codelabel, varoffset, nonTerminals[expression].varname, TypeWidth[temp->SymbolTableBody[nonTerminals[indexed_variable].pos]->datatype]);
                    }
                    nonTerminals[indexed_variable].offset = varoffset;
                    
                    quadList[codelabel].op.type = ARRAYOFFSET;
                    quadList[codelabel].arg1 = nonTerminals[expression];
                    quadList[codelabel].arg2.intval = TypeWidth[temp->SymbolTableBody[nonTerminals[indexed_variable].pos]->datatype];
                    quadList[codelabel].result.varname = varoffset; //????
                    
                    codelabel ++;
                    break;
                    
                case 127: //control_variable -> identifier
                    nonTerminals[control_variable] = nonTerminals[identifier];
                    break;
                    
                case 128: //primary -> unsigned_constant
                    nonTerminals[primary] = nonTerminals[unsigned_constant];
                    break;
                    
                case 129: //primary -> variable_access
                    nonTerminals[primary] = nonTerminals[variable_access];
                    varaccp --;
                    nonTerminals[variable_access] = *varaccp;
                    break;
                    
                case 130: //boolean_expression -> expression
                    nonTerminals[boolean_expression] = nonTerminals[expression];
                    break;
                    
                case 131: //expression -> simple_expression
                    nonTerminals[expression] = nonTerminals[simple_expression];
                    splexp --;
                    break;
                    
                case 132: //simple_expression -> term
                    nonTerminals[simple_expression] = nonTerminals[term];
                    splexp ++;
                    *splexp = nonTerminals[simple_expression];
                    
                    termlstp --;
                    break;
                    
                case 133: //term -> factor
                    nonTerminals[term] = nonTerminals[factor];
                    termlstp ++;
                    *termlstp = nonTerminals[term];
                    break;
                    
                case 134: //factor -> primary
                    nonTerminals[factor] = nonTerminals[primary];
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
                    
                case 149: //assignment_statement -> variable_access ASSIGNMENT expression
                    temp = *tblptrp;
                    do {
                        if (temp->SymbolTableBody[nonTerminals[variable_access].pos]->datatypesize == -1) {
                            notfound = 1;
                            temp = temp->previous;
                        }else{
                            notfound = 0;
                            break;
                        }
                    } while (temp);
                    if(temp->SymbolTableBody[nonTerminals[variable_access].pos]->datatype == nonTerminals[expression].val){
                        // gencode;
                        if (nonTerminals[variable_access].offset == -1) {
                            varname = nonTerminals[expression].varname;
                            printf("%4d: %s := ",codelabel, temp->SymbolTableBody[nonTerminals[variable_access].pos]->name);
                            
                            if (varname == 0) {
                                printf("%s\n", nonTerminals[expression].tokenname);
                            }else{
                                printf("t%d\n", nonTerminals[expression].varname);
                            }
                            quadList[codelabel].op.type = ASSIGNMENT;
                            quadList[codelabel].arg1 = nonTerminals[expression];
                            quadList[codelabel].result.tokenname = temp->SymbolTableBody[nonTerminals[variable_access].pos]->name;
                        }else{
                            varname = nonTerminals[expression].varname;
                            printf("%4d: t%d[t%d] := ",codelabel, nonTerminals[variable_access].varname, nonTerminals[variable_access].offset);
                            if (varname == 0) {
                                printf("%s\n", nonTerminals[expression].tokenname);
                            }else{
                                printf("t%d\n", nonTerminals[expression].varname);
                            }
                            quadList[codelabel].op.type = OFFSETASSIGN;
                            quadList[codelabel].arg1 = nonTerminals[expression];
                            quadList[codelabel].result = nonTerminals[variable_access];
                        }
                        
                        
                        codelabel ++; //gencode;
                        currentTemp = 0;
                        
                        switch (nonTerminals[expression].val) {
                            case INTTYPE:
                                temp->SymbolTableBody[nonTerminals[variable_access].pos]->intval = nonTerminals[expression].intval;
                                break;
                                
                            case REALTYPE:
                                temp->SymbolTableBody[nonTerminals[variable_access].pos]->floatval = nonTerminals[expression].floatval;
                                break;
                                
                            case STRTYPE:
                                temp->SymbolTableBody[nonTerminals[variable_access].pos]->strval = nonTerminals[expression].strval;
                                break;
                                
                            default:
                                break;
                        }
                        
                    }else{
                        printf("\nERROR[TYPE], assign type %s to type %s\n",TypeIdentifier[nonTerminals[expression].val], TypeIdentifier[temp->SymbolTableBody[nonTerminals[variable_access].pos]->datatype]);
                        return 0;
                    }
                    break;
                    
                case 150: //index_expression -> expression
                    nonTerminals[index_expression] = nonTerminals[expression];
                    break;
                    
                case 151: //index_expression_list -> variable_access LBRAC index_expression
                    nonTerminals[index_expression_list] = *arrlstp;
                    nonTerminals[index_expression_list].offset = nonTerminals[index_expression].intval;
                    nonTerminals[index_expression_list].varname = nonTerminals[index_expression].varname;
                    break;
                    
                case 153: //factor -> sign factor
                    if (nonTerminals[sign].type == MINUS) {
                        // gencode;
                        nonTerminals[factor].varname = newtemp();
                        printf("%4d: t%d := minus %s\n",codelabel, nonTerminals[factor].varname, nonTerminals[factor].tokenname);
                        quadList[codelabel].op.type = MINUSASSIGN;
                        quadList[codelabel].arg1 = nonTerminals[factor];
                        quadList[codelabel].result = nonTerminals[factor];
                        
                        codelabel ++;
                        
                        // calculation;
                        nonTerminals[factor].intval = -nonTerminals[factor].intval;
                        nonTerminals[factor].floatval = -nonTerminals[factor].floatval;
                    }
                    break;
                    
                case 155: //primary -> NOT primary
                    for (k = 0; k < BACKPATCHNUM; k++) {
                        blisttemp[k] = nonTerminals[primary].truelist[k];
                    }
                    for (k = 0; k < BACKPATCHNUM; k++) {
                        nonTerminals[primary].truelist[k] = nonTerminals[primary].falselist[k];
                    }
                    for (k = 0; k < BACKPATCHNUM; k++) {
                        nonTerminals[primary].falselist[k] = blisttemp[k];
                    }
                    
                    if (nonTerminals[primary].intval == 0) {
                        nonTerminals[primary].intval = 1;
                    }else{
                        nonTerminals[primary].intval = 0;
                    }
                    break;
                    
                case 159: //statement_sequence -> statement_sequence semicolon m3 statement
                    backpatch(nonTerminals[statement_sequence].nextlist, nonTerminals[m3].quad);
                    nonTerminals[statement_sequence].nextlist = nonTerminals[statement].nextlist;
                    mlstp --;
                    break;
                    
                case 161: //initial_value -> expression
                    nonTerminals[initial_value] = nonTerminals[expression];
                    break;
                    
                case 162: //simple_expression -> simple_expression addop term
                    nonTerminals[simple_expression] = *splexp;
                    nonTerminals[addop] = *addoperp;
                    nonTerminals[term] = *termlstp;
                    
                    // gencode;
                    varname = nonTerminals[simple_expression].varname;
                    quadList[codelabel].arg1 = nonTerminals[simple_expression];
                    
                    nonTerminals[simple_expression].varname = newtemp();
                    quadList[codelabel].result = nonTerminals[simple_expression];
                    
                    if (varname == 0) {
                        printf("%4d: t%d := %s %s",codelabel, nonTerminals[simple_expression].varname, nonTerminals[simple_expression].tokenname, nonTerminals[addop].tokenname);
                    }else{
                        printf("%4d: t%d := t%d %s",codelabel, nonTerminals[simple_expression].varname, varname, nonTerminals[addop].tokenname);
                    }
                    varname = nonTerminals[term].varname;
                    if (varname == 0) {
                        printf(" %s\n", nonTerminals[term].tokenname);
                    }else{
                        printf(" t%d\n", nonTerminals[term].varname);
                    }
                    quadList[codelabel].op = nonTerminals[addop];
                    quadList[codelabel].arg2 = nonTerminals[term];
                    
                    codelabel ++;
                        
                    if (nonTerminals[simple_expression].val == nonTerminals[term].val) {
                        if (nonTerminals[addop].type == PLUS) {
                            switch (nonTerminals[term].val) {
                                case INTTYPE:
                                    nonTerminals[simple_expression].intval = nonTerminals[simple_expression].intval + nonTerminals[term].intval;
                                    break;
                                    
                                case REALTYPE:
                                    nonTerminals[simple_expression].floatval = nonTerminals[simple_expression].floatval + nonTerminals[term].floatval;
                                    break;
                                    
                                default:
                                    break;
                            }
                        }else if (nonTerminals[addop].type == MINUS){
                            switch (nonTerminals[term].val) {
                                case INTTYPE:
                                    nonTerminals[simple_expression].intval = nonTerminals[simple_expression].intval - nonTerminals[term].intval;
                                    break;
                                    
                                case REALTYPE:
                                    nonTerminals[simple_expression].floatval = nonTerminals[simple_expression].floatval - nonTerminals[term].floatval;
                                    break;
                                    
                                default:
                                    break;
                            }
                        }else if (nonTerminals[addop].type == OR){
                            backpatch(nonTerminals[simple_expression].falselist, nonTerminals[m2].quad);
                            nonTerminals[simple_expression].truelist = mergelist(nonTerminals[simple_expression].truelist, nonTerminals[term].truelist);
                            nonTerminals[simple_expression].falselist = nonTerminals[term].falselist;
                        }
                    }else{
                        printf("\nERROR[TYPE].\n");
                        return 0;
                    }
                    *splexp = nonTerminals[simple_expression];
                    
                    termlstp --;
                    
                    addoperp --;
                    nonTerminals[addop] = *addoperp;
                    break;
                    
                case 163: //expression -> simple_expression relop simple_expression
                    tokentemp = nonTerminals[simple_expression];
                    nonTerminals[expression] = nonTerminals[simple_expression];
                    splexp --;
                    nonTerminals[simple_expression] = *splexp;
                    
                    nonTerminals[expression].truelist = makeList(codelabel);
                    nonTerminals[expression].falselist = makeList(codelabel);
                    
                    nonTerminals[expression].varname = newtemp();
                    
                    if (nonTerminals[simple_expression].varname == 0) {
                        if (tokentemp.varname == 0) {
                            printf("%4d: if %s %s %s goto %d\n", codelabel, tokentemp.tokenname, nonTerminals[relop].tokenname, nonTerminals[simple_expression].tokenname, codelabel + 3);
                        }else{
                            printf("%4d: if %s %s t%d goto %d\n", codelabel,tokentemp.tokenname, nonTerminals[relop].tokenname, nonTerminals[simple_expression].varname, codelabel + 3);
                        }
                    }else{
                        if (tokentemp.varname == 0) {
                            printf("%4d: if t%d %s %s goto %d\n", codelabel,tokentemp.varname, nonTerminals[relop].tokenname, nonTerminals[simple_expression].tokenname, codelabel + 3);
                        }else{
                            printf("%4d: if t%d %s t%d goto %d\n", codelabel, tokentemp.varname, nonTerminals[relop].tokenname, nonTerminals[simple_expression].varname, codelabel + 3);
                        }
                    }
                    quadList[codelabel].op = nonTerminals[relop];
                    quadList[codelabel].arg2 = nonTerminals[simple_expression];
                    quadList[codelabel].arg1 = tokentemp;
                    quadList[codelabel].result.quad = codelabel + 3;
                    codelabel ++;
                    
                    printf("%4d: t%d := 0\n", codelabel, nonTerminals[expression].varname);
                    quadList[codelabel].op.type = ARRAYADDR;
                    quadList[codelabel].arg1.varname = 0;
                    quadList[codelabel].arg1.intval = 0;
                    quadList[codelabel].result = nonTerminals[expression];
                    codelabel ++;
                    
                    
                    printf("%4d: goto %d\n", codelabel, codelabel + 2);
                    quadList[codelabel].op.type = GOTO;
                    quadList[codelabel].result.quad = codelabel + 2;
                    codelabel ++;
                    
                    printf("%4d: t%d := 1\n", codelabel, nonTerminals[expression].varname);
                    quadList[codelabel].op.type = ARRAYADDR;
                    quadList[codelabel].arg1.varname = 0;
                    quadList[codelabel].arg1.intval = 1;
                    quadList[codelabel].result = nonTerminals[expression];
                    codelabel ++;
                    
                    reloperp --;
                    splexp --;
                    break;
                    
                case 164: //term -> term mulop factor
                    nonTerminals[term] = *termlstp;
                    nonTerminals[mulop] = *muloperp;
                    
                    // gencode;
                    varname = nonTerminals[term].varname;
                    quadList[codelabel].arg1 = nonTerminals[term];
                    
                    nonTerminals[term].varname = newtemp();
                    quadList[codelabel].result = nonTerminals[term];
                    
                    if (varname == 0) {
                        printf("%4d: t%d := %s %s",codelabel, nonTerminals[term].varname, nonTerminals[term].tokenname, nonTerminals[mulop].tokenname);
                    }else{
                        printf("%4d: t%d := t%d %s",codelabel, nonTerminals[term].varname, varname, nonTerminals[mulop].tokenname);
                    }
                    varname = nonTerminals[factor].varname;
                    if (varname == 0) {
                        printf(" %s\n", nonTerminals[factor].tokenname);
                    }else{
                        printf(" t%d\n", nonTerminals[factor].varname);
                    }
                    quadList[codelabel].op = nonTerminals[mulop];
                    quadList[codelabel].arg2 = nonTerminals[factor];
                    
                    codelabel ++;
                    
                    if (nonTerminals[term].val == nonTerminals[factor].val) {
                        if (nonTerminals[mulop].type == STAR) {
                            switch (nonTerminals[factor].val) {
                                case INTTYPE:
                                    nonTerminals[term].intval = nonTerminals[term].intval * nonTerminals[factor].intval;
                                    break;
                                    
                                case REALTYPE:
                                    nonTerminals[term].floatval = nonTerminals[term].floatval * nonTerminals[factor].floatval;
                                    break;
                                    
                                default:
                                    break;
                            }
                        }else if (nonTerminals[mulop].type == SLASH){
                            switch (nonTerminals[factor].val) {
                                case INTTYPE:
                                    nonTerminals[term].intval = nonTerminals[term].intval / nonTerminals[factor].intval;
                                    break;
                                    
                                case REALTYPE:
                                    nonTerminals[term].floatval = nonTerminals[term].floatval / nonTerminals[factor].floatval;
                                    break;
                                    
                                default:
                                    break;
                            }
                        }
                    }else{
                        printf("\nERROR[TYPE].\n");
                    }
                    *termlstp = nonTerminals[term];
                    
                    muloperp --;
                    nonTerminals[mulop] = *muloperp;
                    break;
                    
                case 165: //primary -> LPAREN expression RPAREN
                    nonTerminals[primary] = nonTerminals[expression];
                    break;
                    
                case 166: //n3 -> EPSILON
                    backpatch(nonTerminals[statement_sequence].nextlist, codelabel);
                    break;
                    
                case 173: //open_if_statement -> IF boolean_expression THEN m3 statement
                    backpatch(nonTerminals[boolean_expression].truelist, nonTerminals[m3].quad);
                    nonTerminals[open_if_statement].nextlist = mergelist(nonTerminals[boolean_expression].falselist, nonTerminals[statement].nextlist);
                    
                    mlstp --;
                    break;
                    
                case 174: //n2 -> EPSILON
                    nonTerminals[n2].nextlist = makeList(codelabel);
                    printf("%4d: goto -", codelabel);
                    
                    quadList[codelabel].op.type = GOTO;
                    quadList[codelabel].result.quad = -1;
                    
                    codelabel ++;
                    break;
                    
                case 176: //array_type -> ARRAY LBRAC index_list RBRAC OF component_type
                    isArray = 1;
                    datatypeSize = TypeWidth[typeDenoter] * currentArraySize;
                    break;
                    
                case 177: //final_value -> expression
                    nonTerminals[final_value] = nonTerminals[expression];
                    break;
                    
                case 178: //repeat_statement -> REPEAT m5 statement_sequence UNTIL n3 boolean_expression
                    backpatch(nonTerminals[boolean_expression].falselist, nonTerminals[m5].quad);
                    nonTerminals[repeat_statement].nextlist = nonTerminals[boolean_expression].truelist;
                    break;
                    
                case 179: //closed_while_statement -> WHILE m3 boolean_expression DO m3 closed_statement
                    mlstp --;
                    tokentemp = *mlstp;
                    
                    backpatch(nonTerminals[closed_statement].nextlist, tokentemp.quad);
                    backpatch(nonTerminals[boolean_expression].truelist, nonTerminals[m3].quad + 1);
                    nonTerminals[closed_while_statement].nextlist = nonTerminals[boolean_expression].falselist;
                    
                    printf("%4d: goto %d %d\n", codelabel, tokentemp.quad, nonTerminals[m3].quad);
                    
                    quadList[codelabel].op.type = GOTO;
                    quadList[codelabel].result.quad = tokentemp.quad;
                    
                    codelabel ++;
                    
                    mlstp --;
                    break;
                    
                case 182: //m4 -> EPSILON
                    nonTerminals[m4] = nonTerminals[control_variable];
                    if (nonTerminals[initial_value].varname == 0) {
                        printf("%4d: %s := %s\n",codelabel, nonTerminals[m4].tokenname, nonTerminals[initial_value].tokenname);
                    }else{
                        printf("%4d: %s := t%d\n",codelabel, nonTerminals[m4].tokenname, nonTerminals[initial_value].varname);
                    }
                    
                    quadList[codelabel].op.type = ASSIGNMENT;
                    quadList[codelabel].arg1 = nonTerminals[initial_value];
                    quadList[codelabel].result = nonTerminals[m4];
                    
                    codelabel ++;
                    
                    newtemptemp = newtemp();
                    if (nonTerminals[final_value].varname == 0) {
                        printf("%4d: t%d := %s\n", codelabel, newtemptemp, nonTerminals[final_value].tokenname);
                    }else{
                        printf("%4d: t%d := t%d\n", codelabel, newtemptemp, nonTerminals[final_value].varname);
                    }
                    
                    quadList[codelabel].op.type = ARRAYADDR;
                    quadList[codelabel].arg1 = nonTerminals[final_value];
                    quadList[codelabel].result.varname = newtemptemp;
                    
                    codelabel ++;
                    
                    printf("%4d: goto %d\n", codelabel, codelabel + 2);
                    
                    quadList[codelabel].op.type = GOTO;
                    quadList[codelabel].result.quad = codelabel + 2;
                    
                    codelabel ++;
                    
                    nonTerminals[m4].again = codelabel;
                    
                    printf("%4d: %s := %s + 1\n", codelabel, nonTerminals[m4].tokenname, nonTerminals[m4].tokenname);
                    
                    quadList[codelabel].op.type = PLUS;
                    quadList[codelabel].arg1.varname = 0;
                    quadList[codelabel].arg1.tokenname = nonTerminals[m4].tokenname;
                    quadList[codelabel].arg2.varname = 0;
                    quadList[codelabel].arg2.tokenname = "1";
                    quadList[codelabel].result.varname = 0;
                    quadList[codelabel].result.tokenname = nonTerminals[m4].tokenname;
                    
                    codelabel ++;
                    
                    nonTerminals[m4].nextlist = makeList(codelabel);
                
                
                    printf("%4d: if %s > t%d goto -\n", codelabel, nonTerminals[m4].tokenname, newtemptemp);
                    
                    quadList[codelabel].op.type = GT;
                    quadList[codelabel].arg1.varname = 0;
                    quadList[codelabel].arg1.tokenname = nonTerminals[m4].tokenname;
                    quadList[codelabel].arg2.varname = newtemptemp;
                    quadList[codelabel].result.quad = -1;
                    
                    codelabel ++;
                    
                    break;
                    
                case 183: //closed_for_statement -> FOR control_variable ASSIGNMENT initial_value direction final_value DO m4 closed_statement
                    backpatch(nonTerminals[closed_statement].nextlist, nonTerminals[m4].again);
                    
                    printf("%4d: goto %d\n",codelabel, nonTerminals[m4].again);
                    quadList[codelabel].op.type = GOTO;
                    quadList[codelabel].result.quad = nonTerminals[m4].again;
                    
                    codelabel ++;
                    
                    nonTerminals[closed_for_statement].nextlist = nonTerminals[m4].nextlist;
                    
                    clsstmtp --;
                    mlstp --;
                    break;
                    
                case 185: //closed_if_statement -> IF boolean_expression THEN m3 closed_statement n2 ELSE m3 closed_statement
                    mlstp --;
                    tokentemp = *mlstp;
                    
                    clsstmtp --;
                    tokentemp1 = *clsstmtp;
                    
                    backpatch(nonTerminals[boolean_expression].truelist, tokentemp.quad);
                    backpatch(nonTerminals[boolean_expression].falselist, nonTerminals[m3].quad);
                    nonTerminals[closed_if_statement].nextlist = mergelist(tokentemp1.nextlist, mergelist(nonTerminals[n2].nextlist, nonTerminals[closed_statement].nextlist));
                    mlstp --;
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
            printf("\nError before %s where shouldn't exist a(an) %s!\n\n",errtoken, Terminals[look_ahead->type]);
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
            tokenScanned->offset = -1;
            tokenScanned->varname = 0;
            tokenScanned->val = -1;
            tokenScanned->floatval = 0;
            tokenScanned->intval = 0;
            tokenScanned->strval = "";
            tokenScanned->quad = -1;
            tokenScanned->truelist = NULL;
            tokenScanned->falselist = NULL;
            tokenScanned->nextlist = NULL;
            tokenScanned->again = -1;
            
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
                    tokenScanned->offset = -1;
                    tokenScanned->varname = 0;
                    tokenScanned->val = HEX;
                    tokenScanned->floatval = 0;
                    tokenScanned->intval = install_hex(token);
                    tokenScanned->strval = "";
                    tokenScanned->quad = -1;
                    tokenScanned->truelist = NULL;
                    tokenScanned->falselist = NULL;
                    tokenScanned->nextlist = NULL;
                    tokenScanned->again = -1;
                    
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
                    tokenScanned->offset = -1;
                    tokenScanned->varname = 0;
                    tokenScanned->val = OCT;
                    tokenScanned->floatval = 0;
                    tokenScanned->intval = install_oct(token);
                    tokenScanned->strval = "";
                    tokenScanned->quad = -1;
                    tokenScanned->truelist = NULL;
                    tokenScanned->falselist = NULL;
                    tokenScanned->nextlist = NULL;
                    tokenScanned->again = -1;
                    
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
                    tokenScanned->offset = -1;
                    tokenScanned->varname = 0;
                    tokenScanned->val = REALTYPE;
                    tokenScanned->floatval = install_real(token);
                    tokenScanned->intval = 0;
                    tokenScanned->strval = "";
                    tokenScanned->quad = -1;
                    tokenScanned->truelist = NULL;
                    tokenScanned->falselist = NULL;
                    tokenScanned->nextlist = NULL;
                    tokenScanned->again = -1;
                    
                    printf("(%d, %f)\n", REALNUMBER, tokenScanned->floatval);
                    return tokenScanned;
                }else{
                    fseek(pasfile, -1, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    
                    tokenScanned->tokenname = token;
                    tokenScanned->type = DIGSEQ;
                    tokenScanned->pos = 0;
                    tokenScanned->offset = -1;
                    tokenScanned->varname = 0;
                    tokenScanned->val = INTTYPE;
                    tokenScanned->floatval = install_int(token);
                    tokenScanned->intval = 0;
                    tokenScanned->strval = "";
                    tokenScanned->quad = -1;
                    tokenScanned->truelist = NULL;
                    tokenScanned->falselist = NULL;
                    tokenScanned->nextlist = NULL;
                    tokenScanned->again = -1;
                    
                    printf("(%d, %d)\n", DIGSEQ, tokenScanned->intval);
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
                    *temp++ = check;
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
                    tokenScanned->offset = -1;
                    tokenScanned->varname = 0;
                    tokenScanned->val = REALTYPE;
                    tokenScanned->floatval = install_real(token);
                    tokenScanned->intval = 0;
                    tokenScanned->strval = "";
                    tokenScanned->quad = -1;
                    tokenScanned->truelist = NULL;
                    tokenScanned->falselist = NULL;
                    tokenScanned->nextlist = NULL;
                    tokenScanned->again = -1;
                    
                    printf("(%d, %f)\n", REALNUMBER, tokenScanned->floatval);
                    return tokenScanned;
                }else if (ch == '.' &&  check == '.'){
                    fseek(pasfile, -2, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    
                    tokenScanned->tokenname = token;
                    tokenScanned->type = DIGSEQ;
                    tokenScanned->pos = 0;
                    tokenScanned->offset = -1;
                    tokenScanned->varname = 0;
                    tokenScanned->val = INTTYPE;
                    tokenScanned->floatval = 0;
                    tokenScanned->intval = install_int(token);
                    tokenScanned->strval = "";
                    tokenScanned->quad = -1;
                    tokenScanned->truelist = NULL;
                    tokenScanned->falselist = NULL;
                    tokenScanned->nextlist = NULL;
                    tokenScanned->again = -1;
                    
                    printf("(%d, %d)\n", DIGSEQ, tokenScanned->intval);
                    return tokenScanned;
                }else {
                    fseek(pasfile, -2, SEEK_CUR);
                    strcpy(errtoken, token);
                    printf("%s ",token);
                    
                    tokenScanned->tokenname = token;
                    tokenScanned->type = DIGSEQ;
                    tokenScanned->pos = 0;
                    tokenScanned->offset = -1;
                    tokenScanned->varname = 0;
                    tokenScanned->val = INTTYPE;
                    tokenScanned->floatval = 0;
                    tokenScanned->intval = install_int(token);
                    tokenScanned->strval = "";
                    tokenScanned->quad = -1;
                    tokenScanned->truelist = NULL;
                    tokenScanned->falselist = NULL;
                    tokenScanned->nextlist = NULL;
                    tokenScanned->again = -1;
                    
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
            tokenScanned->offset = -1;
            tokenScanned->varname = 0;
            tokenScanned->val = STRTYPE;
            tokenScanned->floatval = 0;
            tokenScanned->intval = 0;
            tokenScanned->strval = install_str(token);
            tokenScanned->quad = -1;
            tokenScanned->truelist = NULL;
            tokenScanned->falselist = NULL;
            tokenScanned->nextlist = NULL;
            tokenScanned->again = -1;
            
            printf("(%d, %s)\n", CHARACTER_STRING, tokenScanned->strval);
            
            ch = fgetc(pasfile);
            
            return tokenScanned;
        }else{
            tokenScanned->pos = 0;
            tokenScanned->offset = -1;
            tokenScanned->varname = 0;
            tokenScanned->val = -1;
            tokenScanned->floatval = 0;
            tokenScanned->intval = 0;
            tokenScanned->strval = "";
            tokenScanned->quad = -1;
            tokenScanned->truelist = NULL;
            tokenScanned->falselist = NULL;
            tokenScanned->nextlist = NULL;
            tokenScanned->again = -1;
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
    tokenScanned->offset = -1;
    tokenScanned->varname = 0;
    tokenScanned->val = -1;
    tokenScanned->floatval = 0;
    tokenScanned->intval = 0;
    tokenScanned->strval = "";
    tokenScanned->quad = -1;
    tokenScanned->truelist = NULL;
    tokenScanned->falselist = NULL;
    tokenScanned->nextlist = NULL;
    tokenScanned->again = -1;
    
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

int isKeyWord(char *str){
    int i = 0;
    for (i = 0; i < KEYWORDNUM; i++) {
        if (strcmp(str, KeyWord[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int gettoken(char *str) {
    int i = isKeyWord(str);
    if (i != -1) {
        return KeywordTable[i]->type;
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
            idnodetemp->next_hash->isarray = 0;
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
    int i;
    for (i = 0; i < KEYWORDNUM; i++) {
        KeywordTable[i] = (Keyword *)malloc(sizeof(Keyword));
        KeywordTable[i]->name = KeyWord[i];
        KeywordTable[i]->type = KeyWordCode[i];
    }
}
