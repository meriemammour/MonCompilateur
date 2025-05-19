//  A compiler from a very simple Pascal-like structured language LL(k)
//  to 64-bit 80x86 Assembly langage
//  Copyright (C) 2019 Pierre Jourlin
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Build with "make compilateur"


#include <string>
#include <iostream>
#include <cstdlib>
#include <set>
#include <FlexLexer.h>
#include "tokeniser.h"
#include <cstring>

using namespace std;


enum OPREL {EQU, DIFF, INF, SUP, INFE, SUPE, WTFR};
enum OPADD {ADD, SUB, OR, WTFA};
enum OPMUL {MUL, DIV, MOD, AND ,WTFM};

// Prototypes anticipés pour éviter erreurs
void Factor(void);
void Term(void);
OPADD AdditiveOperator(void);
void SimpleExpression(void);
void Expression(void);
void Statement(void);

TOKEN current;				// Current token


FlexLexer* lexer = new yyFlexLexer; // This is the flex tokeniser

set<string> DeclaredVariables;
unsigned long TagNumber=0;

bool IsDeclared(const char *id){
	return DeclaredVariables.find(id)!=DeclaredVariables.end();
}


void Error(string s){
	cerr << "Ligne n°"<<lexer->lineno()<<", lu : '"<<lexer->YYText()<<"'("<<current<<"), mais ";
	cerr<< s << endl;
	exit(-1);
}

// Prototypes anticipés
void Expression(void);
void Statement(void);
void DeclarationPart(void);
void StatementPart(void);
void Program(void);

// -- Fonctions de base (Identifier, Number, Factor, etc) --
// (tes définitions de Identifier, Number, Factor, Term, etc ici...)
enum TYPES { UNSIGNED_INT, BOOLEAN };

void Identifier(void){
	cout << "\tpush "<<lexer->YYText()<<endl;
	current=(TOKEN) lexer->yylex();
    return UNSIGNED_INT; // Tous les identificateurs sont UNSIGNED_INT pour l’instant
}

void Number(void){
	cout <<"\tpush $"<<atoi(lexer->YYText())<<endl;
	current=(TOKEN) lexer->yylex();
    return UNSIGNED_INT;
}

TYPES Factor(void) {
    if(current == LPARENT) {
        current = (TOKEN) lexer->yylex();
        TYPES type = Expression();
        if(current != RPARENT)
            Error("')' était attendu");
        current = (TOKEN) lexer->yylex();
        return type;
    }
    else if(current == NUMBER)
        return Number();
    else if(current == ID)
        return Identifier();
    else
        Error("'(' ou chiffre ou lettre attendue");
    return UNSIGNED_INT; // pour éviter warning
}



OPADD AdditiveOperator(void) {
    OPADD opadd;
    if(strcmp(lexer->YYText(),"+") == 0)
        opadd = ADD;
    else if(strcmp(lexer->YYText(),"-") == 0)
        opadd = SUB;
    else if(strcmp(lexer->YYText(),"||") == 0)
        opadd = OR;
    else
        opadd = WTFA;
    current = (TOKEN) lexer->yylex();
    return opadd;
}

OPMUL MultiplicativeOperator(void) {
    OPMUL opmul;
    if(strcmp(lexer->YYText(), "*") == 0)
        opmul = MUL;
    else if(strcmp(lexer->YYText(), "/") == 0)
        opmul = DIV;
    else if(strcmp(lexer->YYText(), "%") == 0)
        opmul = MOD;
    else if(strcmp(lexer->YYText(), "&&") == 0)
        opmul = AND;
    else
        opmul = WTFM;
    current = (TOKEN) lexer->yylex();
    return opmul;
}



void Term(void) {
    OPMUL mulop;
    Factor();
    while(current == MULOP) {
        mulop = MultiplicativeOperator();
        Factor();
        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;
        switch(mulop) {
            case AND:
                cout << "\tmulq %rbx" << endl;
                cout << "\tpush %rax\t# AND" << endl;
                break;
            case MUL:
                cout << "\tmulq %rbx" << endl;
                cout << "\tpush %rax\t# MUL" << endl;
                break;
            case DIV:
                cout << "\tmovq $0, %rdx" << endl;
                cout << "\tdiv %rbx" << endl;
                cout << "\tpush %rax\t# DIV" << endl;
                break;
            case MOD:
                cout << "\tmovq $0, %rdx" << endl;
                cout << "\tdiv %rbx" << endl;
                cout << "\tpush %rdx\t# MOD" << endl;
                break;
            default:
                Error("opérateur multiplicatif attendu");
        }
    }
}

// (Le reste des fonctions comme MultiplicativeOperator, Term, AdditiveOperator, SimpleExpression, etc.)

// --- Fonctions déclaration et expressions (DeclarationPart, RelationalOperator, Expression) ---

void DeclarationPart(void){
	if(current!=RBRACKET)
		Error("caractère '[' attendu");
	cout << "\t.data"<<endl;
	cout << "\t.align 8"<<endl;
	
	current=(TOKEN) lexer->yylex();
	if(current!=ID)
		Error("Un identificateur était attendu");
	cout << lexer->YYText() << ":\t.quad 0"<<endl;
	DeclaredVariables.insert(lexer->YYText());
	current=(TOKEN) lexer->yylex();
	while(current==COMMA){
		current=(TOKEN) lexer->yylex();
		if(current!=ID)
			Error("Un identificateur était attendu");
		cout << lexer->YYText() << ":\t.quad 0"<<endl;
		DeclaredVariables.insert(lexer->YYText());
		current=(TOKEN) lexer->yylex();
	}
	if(current!=LBRACKET)
		Error("caractère ']' attendu");
	current=(TOKEN) lexer->yylex();
}

OPREL RelationalOperator(void){
	OPREL oprel;
	if(strcmp(lexer->YYText(),"==")==0)
		oprel=EQU;
	else if(strcmp(lexer->YYText(),"!=")==0)
		oprel=DIFF;
	else if(strcmp(lexer->YYText(),"<")==0)
		oprel=INF;
	else if(strcmp(lexer->YYText(),">")==0)
		oprel=SUP;
	else if(strcmp(lexer->YYText(),"<=")==0)
		oprel=INFE;
	else if(strcmp(lexer->YYText(),">=")==0)
		oprel=SUPE;
	else oprel=WTFR;
	current=(TOKEN) lexer->yylex();
	return oprel;
}
void SimpleExpression(void){
    OPADD adop;
    Term();
    while(current==ADDOP){
        adop=AdditiveOperator();
        Term();
        cout << "\tpop %rbx"<<endl;
        cout << "\tpop %rax"<<endl;
        switch(adop){
            case OR:
                cout << "\taddq\t%rbx, %rax\t# OR"<<endl;
                break;            
            case ADD:
                cout << "\taddq\t%rbx, %rax\t# ADD"<<endl;
                break;            
            case SUB:    
                cout << "\tsubq\t%rbx, %rax\t# SUB"<<endl;
                break;
            default:
                Error("opérateur additif inconnu");
        }
        cout << "\tpush %rax"<<endl;
    }
}


void Expression(void){
    OPREL oprel;
    SimpleExpression();    // On commence par analyser une SimpleExpression

    if(current == RELOP){  // Si on a un opérateur relationnel, on analyse la suite
        oprel = RelationalOperator();
        SimpleExpression();

        // On génère le code assembleur pour la comparaison
        cout << "\tpop %rax" << endl;
        cout << "\tpop %rbx" << endl;
        cout << "\tcmpq %rax, %rbx" << endl;

        // Selon l'opérateur, on saute à une étiquette "Vrai"
        switch(oprel){
            case EQU:
                cout << "\tje Vrai" << ++TagNumber << "\t# If equal" << endl;
                break;
            case DIFF:
                cout << "\tjne Vrai" << ++TagNumber << "\t# If different" << endl;
                break;
            case SUPE:
                cout << "\tjae Vrai" << ++TagNumber << "\t# If above or equal" << endl;
                break;
            case INFE:
                cout << "\tjbe Vrai" << ++TagNumber << "\t# If below or equal" << endl;
                break;
            case INF:
                cout << "\tjb Vrai" << ++TagNumber << "\t# If below" << endl;
                break;
            case SUP:
                cout << "\tja Vrai" << ++TagNumber << "\t# If above" << endl;
                break;
            default:
                Error("Opérateur de comparaison inconnu");
        }

        // Si la comparaison est fausse, on pousse 0 (False)
        cout << "\tpush $0\t\t# False" << endl;
        cout << "\tjmp Suite" << TagNumber << endl;

        // Sinon on pousse 0xFFFFFFFFFFFFFFFF (True)
        cout << "Vrai" << TagNumber << ":\tpush $0xFFFFFFFFFFFFFFFF\t\t# True" << endl;

        cout << "Suite" << TagNumber << ":" << endl;
    }
}


// --- Structures de contrôle (AssignementStatement, IfStatement, WhileStatement, ForStatement, BlockStatement) ---

void AssignementStatement(void){
	string variable;
	if(current!=ID)
		Error("Identificateur attendu");
	if(!IsDeclared(lexer->YYText())){
		cerr << "Erreur : Variable '"<<lexer->YYText()<<"' non déclarée"<<endl;
		exit(-1);
	}
	variable=lexer->YYText();
	current=(TOKEN) lexer->yylex();
	if(current!=ASSIGN)
		Error("caractères ':=' attendus");
	current=(TOKEN) lexer->yylex();
	Expression();
	cout << "\tpop "<<variable<<endl;
}

void IfStatement(void){
	current = (TOKEN) lexer->yylex(); // Consommer IF
	Expression();
	if(current != THEN)
		Error("THEN attendu");
	current = (TOKEN) lexer->yylex();
	Statement();
	if(current == ELSE){
		current = (TOKEN) lexer->yylex();
		Statement();
	}
}

void WhileStatement(void){
	unsigned long tagStart = ++TagNumber;
	unsigned long tagEnd = ++TagNumber;

	current = (TOKEN) lexer->yylex(); // consommer WHILE

	cout << "Boucle" << tagStart << ":" << endl;

	Expression();  // Condition
	cout << "\tpop %rax" << endl;
	cout << "\tcmpq $0, %rax" << endl;
	cout << "\tje FinBoucle" << tagEnd << endl;

	if(current != DO)
		Error("DO attendu");
	current = (TOKEN) lexer->yylex();

	Statement();   // Corps de la boucle

	cout << "\tjmp Boucle" << tagStart << endl;
	cout << "FinBoucle" << tagEnd << ":" << endl;
}

void ForStatement(void){
	unsigned long tagStart = ++TagNumber;
	unsigned long tagEnd = ++TagNumber;

	current = (TOKEN) lexer->yylex(); // consommer FOR

	// Récupérer la variable assignée dans l'initialisation
	if(current != ID)
		Error("Identificateur attendu dans FOR");
	string variable = lexer->YYText();

	if(!IsDeclared(variable.c_str())){
		cerr << "Erreur : Variable '" << variable << "' non déclarée" << endl;
		exit(-1);
	}

	AssignementStatement();           // Initialisation (ex: i := 0)

	if(current != TO)
		Error("TO attendu");
	current = (TOKEN) lexer->yylex();

	Expression();                     // Valeur limite
	cout << "\tpop %rbx" << endl;    // Stocker limite dans %rbx

	if(current != DO)
		Error("DO attendu");
	current = (TOKEN) lexer->yylex();

	cout << "Boucle" << tagStart << ":" << endl;

	// Comparer variable courante à limite
	cout << "\tpush " << variable << endl;
	cout << "\tpop %rax" << endl;
	cout << "\tcmpq %rbx, %rax" << endl;
	cout << "\tjg FinBoucle" << tagEnd << endl;

	Statement();                     // Corps de la boucle

	// Incrémenter la variable
	cout << "\tpush " << variable << endl;
	cout << "\tpop %rax" << endl;
	cout << "\taddq $1, %rax" << endl;
	cout << "\tpush %rax" << endl;
	cout << "\tpop " << variable << endl;

	cout << "\tjmp Boucle" << tagStart << endl;
	cout << "FinBoucle" << tagEnd << ":" << endl;
}

void BlockStatement(void) {
    if(current != BEGIN_TOKEN)
        Error("BEGIN attendu");
    current = (TOKEN) lexer->yylex();  // consommer BEGIN

    //cout << "DEBUG après BEGIN: current = " << current << endl;

    Statement();

    while(current == SEMICOLON){
        current = (TOKEN) lexer->yylex();
       // cout << "DEBUG dans boucle BlockStatement: current = " << current << endl;

        if(current == END_TOKEN) {
          //   cout << "DEBUG fin block : END_TOKEN trouvé" << endl;
            break;  // Ne pas appeler Statement() si END_TOKEN
        }

        Statement();
    }

   // cout << "DEBUG avant fin BlockStatement: current = " << current << endl;

    if(current != END_TOKEN)
        Error("END attendu");
    current = (TOKEN) lexer->yylex();  // consommer END
}



void Statement(void) {
    switch(current) {
        case ID:
            AssignementStatement();
            break;
        case IF:
            IfStatement();
            break;
        case WHILE:
            WhileStatement();
            break;
        case FOR:
            ForStatement();
            break;
        case BEGIN_TOKEN:
            BlockStatement();
            break;
        case END_TOKEN:  // <-- IMPORTANT
            // Ne rien faire, ne pas appeler Statement(), 
            // car END marque la fin du block.
            // Si on entre ici, c'est une erreur, on peut retourner ou faire une erreur.
            Error("Instruction attendue (pas END)");
            break;
        default:
            Error("Instruction attendue");
    }
}



// -- AJOUT de la fonction Program() ---

void StatementPart(void){
    cout << "\t.text\t\t# The following lines contain the program"<<endl;
    cout << "\t.globl main\t# The main function must be visible from outside"<<endl;
    cout << "main:\t\t\t# The main function body :"<<endl;
    cout << "\tmovq %rsp, %rbp\t# Save the position of the stack's top"<<endl;

    Statement();

    while(current == SEMICOLON){
        current = (TOKEN) lexer->yylex();
        // Ne pas appeler Statement si current vaut END_TOKEN ou DOT
        if(current == END_TOKEN || current == DOT)
            break;
        Statement();
    }

    if(current != DOT)
        Error("caractère '.' attendu");
    current = (TOKEN) lexer->yylex();
}


void Program(void){
	if(current==RBRACKET)
		DeclarationPart();
	StatementPart();
}

int main(void){
	cout << "\t\t\t# This code was produced by the CERI Compiler"<<endl;
	current = (TOKEN) lexer->yylex();
	Program();
	cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top"<<endl;
	cout << "\tret\t\t\t# Return from main function"<<endl;
	if(current != FEOF){
		cerr << "Caractères en trop à la fin du programme : [" << current << "]";
		Error(".");
	}
	return 0;
}
