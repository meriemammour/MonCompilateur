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
#include <unistd.h>
#include <map>
#include <vector>


using namespace std;


enum OPREL {EQU, DIFF, INF, SUP, INFE, SUPE, WTFR};
enum OPADD {ADD, SUB, OR, WTFA};
enum OPMUL {MUL, DIV, MOD, AND ,WTFM};

// Prototypes anticipés pour éviter erreurs
enum TYPES { UNSIGNED_INT, TYPE_BOOLEAN };


// Prototypes anticipés avec le bon type de retour
TYPES Factor(void);
TYPES Term(void);
OPADD AdditiveOperator(void);
TYPES SimpleExpression(void);
TYPES Expression(void);
void Statement(void);
void DeclarationPart(void);
void StatementPart(void);
void Program(void);


TOKEN current;				// Current token


FlexLexer* lexer = new yyFlexLexer; // This is the flex tokeniser

map<string, TYPES> DeclaredVariables;

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
TYPES Expression(void);
void Statement(void);
void DeclarationPart(void);
void StatementPart(void);
void Program(void);

// -- Fonctions de base (Identifier, Number, Factor, etc) --
// (tes définitions de Identifier, Number, Factor, Term, etc ici...)

//modificatiion tp44
TYPES Identifier(void){
    string var = lexer->YYText();
    if(!IsDeclared(var.c_str()))
        Error("Variable non déclarée");
    cout << "\tpush " << var << endl;
    current = (TOKEN)lexer->yylex();
    return DeclaredVariables[var];  // retourner son vrai type
}

// modification tp4
TYPES Number(void){
    cout << "\tpush $" << atoi(lexer->YYText()) << endl;
    current = (TOKEN) lexer->yylex();
    return UNSIGNED_INT;
}

//modification tp4
TYPES Factor(void){
    TYPES type;
    if(current == LPARENT){  // '('
        current = (TOKEN) lexer->yylex();
        type = Expression();
        if(current != RPARENT)
            Error("')' était attendu");
        else
            current = (TOKEN) lexer->yylex();
    }
    else if(current == NUMBER)
        type = Number();
    else if(current == TRUE_CONST || current == FALSE_CONST) { 
        // Si le lexer retourne TRUE_CONST et FALSE_CONST pour true/false
        cout << "\tpush $" << (current == TRUE_CONST ? 1 : 0) << endl;
        current = (TOKEN) lexer->yylex();
        type = TYPE_BOOLEAN;
    }
    else if(current == ID)
        type = Identifier();
    else
        Error("'(' ou chiffre ou lettre attendue");
    return type;
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


//modification tp4
TYPES Term(void){
    TYPES type1, type2;
    OPMUL mulop;
    type1 = Factor();
    while(current == MULOP){
        mulop = MultiplicativeOperator();
        type2 = Factor();
        if(type2 != type1)
            Error("types incompatibles dans Term");

        // Génération de code assembleur selon mulop
        cout << "\tpop %rbx" << endl;  // deuxième opérande
        cout << "\tpop %rax" << endl;  // premier opérande
        switch(mulop) {
            case AND:
                if(type1 != TYPE_BOOLEAN)
                    Error("type BOOLEAN attendu pour AND");
                cout << "\tandq %rbx, %rax\t# AND" << endl;
                break;
            case MUL:
                if(type1 != UNSIGNED_INT)
                    Error("type UNSIGNED_INT attendu pour MUL");
                cout << "\timulq %rbx, %rax\t# MUL" << endl;
                break;
            case DIV:
                if(type1 != UNSIGNED_INT)
                    Error("type UNSIGNED_INT attendu pour DIV");
                cout << "\tmovq $0, %rdx" << endl;
                cout << "\tidivq %rbx\t# DIV" << endl;
                break;
            case MOD:
                if(type1 != UNSIGNED_INT)
                    Error("type UNSIGNED_INT attendu pour MOD");
                cout << "\tmovq $0, %rdx" << endl;
                cout << "\tidivq %rbx\t# MOD" << endl;
                cout << "\tmovq %rdx, %rax" << endl;
                break;
            default:
                Error("opérateur multiplicatif inconnu");
        }
        cout << "\tpush %rax" << endl;
    }
    return type1;
}



// (Le reste des fonctions comme MultiplicativeOperator, Term, AdditiveOperator, SimpleExpression, etc.)

// --- Fonctions déclaration et expressions (DeclarationPart, RelationalOperator, Expression) ---
//modification en tp4 / tp5 / tp6
void DeclarationPart() {
    if(current != VAR)
        Error("VAR attendu en début de déclaration");

    current = (TOKEN) lexer->yylex();  // consomme VAR

    while(current == ID) {
        vector<string> vars;
        vars.push_back(lexer->YYText());

        current = (TOKEN) lexer->yylex();

        while(current == COMMA) {
            current = (TOKEN) lexer->yylex();
            if(current != ID)
                Error("Identificateur attendu après ','");
            vars.push_back(lexer->YYText());
            current = (TOKEN) lexer->yylex();
        }

        if(current != COLON)
            Error("':' attendu après liste de variables");

        current = (TOKEN) lexer->yylex();

        if(current != BOOLEAN && current != INTEGER)
            Error("Type BOOLEAN ou INTEGER attendu après ':'");

        TYPES type;
        if(current == BOOLEAN)
            type = TYPE_BOOLEAN;
        else
            type = UNSIGNED_INT;

        for(const auto &v : vars) {
            if(IsDeclared(v.c_str()))
                Error("Variable déjà déclarée : " + v);
            DeclaredVariables[v] = type;
            cout << v << ":\t.quad 0" << endl;
        }

        current = (TOKEN) lexer->yylex();

        if(current == SEMICOLON) {
            current = (TOKEN) lexer->yylex();
        }
        else if(current == BEGIN_TOKEN) {
            break;
        }
        else {
            Error("';' ou 'BEGIN' attendu après déclaration");
        }
    }
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

//modification tp4
TYPES SimpleExpression(void){
    TYPES type1, type2;
    OPADD adop;
    type1 = Term();
    while(current == ADDOP){
        adop = AdditiveOperator();
        type2 = Term();
        if(type2 != type1)
            Error("types incompatibles dans SimpleExpression");

        // Génération de code assembleur selon adop
        cout << "\tpop %rbx" << endl;
        cout << "\tpop %rax" << endl;
        switch(adop) {
            case OR:
                if(type1 != TYPE_BOOLEAN)
                    Error("type BOOLEAN attendu pour OR");
                cout << "\torq %rbx, %rax\t# OR" << endl;
                break;
            case ADD:
                if(type1 != UNSIGNED_INT)
                    Error("type UNSIGNED_INT attendu pour ADD");
                cout << "\taddq %rbx, %rax\t# ADD" << endl;
                break;
            case SUB:
                if(type1 != UNSIGNED_INT)
                    Error("type UNSIGNED_INT attendu pour SUB");
                cout << "\tsubq %rbx, %rax\t# SUB" << endl;
                break;
            default:
                Error("opérateur additif inconnu");
        }
        cout << "\tpush %rax" << endl;
    }
    return type1;
}


//modiification tp4
TYPES Expression(void){
    TYPES type1, type2;
    OPREL oprel;
    type1 = SimpleExpression();

    if(current == RELOP){
        oprel = RelationalOperator();
        type2 = SimpleExpression();
        if(type2 != type1)
            Error("types incompatibles pour la comparaison");

        // Génération de code assembleur pour comparaison
        cout << "\tpop %rax" << endl;
        cout << "\tpop %rbx" << endl;
        cout << "\tcmpq %rax, %rbx" << endl;

        unsigned long tag = ++TagNumber;
        switch(oprel){
            case EQU:
                cout << "\tje Vrai" << tag << "\t# If equal" << endl;
                break;
            case DIFF:
                cout << "\tjne Vrai" << tag << "\t# If different" << endl;
                break;
            case SUPE:
                cout << "\tjae Vrai" << tag << "\t# If above or equal" << endl;
                break;
            case INFE:
                cout << "\tjbe Vrai" << tag << "\t# If below or equal" << endl;
                break;
            case INF:
                cout << "\tjb Vrai" << tag << "\t# If below" << endl;
                break;
            case SUP:
                cout << "\tja Vrai" << tag << "\t# If above" << endl;
                break;
            default:
                Error("Opérateur de comparaison inconnu");
        }

        cout << "\tpush $0\t\t# False" << endl;
        cout << "\tjmp Suite" << tag << endl;
        cout << "Vrai" << tag << ":\tpush $0xFFFFFFFFFFFFFFFF\t\t# True" << endl;
        cout << "Suite" << tag << ":" << endl;

        return TYPE_BOOLEAN;
    }
    return type1;
}



// --- Structures de contrôle (AssignementStatement, IfStatement, WhileStatement, ForStatement, BlockStatement) ---
//modification en tp4
void AssignementStatement(void){
    if(current != ID)
        Error("Identificateur attendu");
    string variable = lexer->YYText();

    // Vérifier que la variable est déclarée
    if(DeclaredVariables.find(variable) == DeclaredVariables.end())
        Error("Variable non déclarée");

    TYPES varType = DeclaredVariables[variable];  // Récupérer type réel de la variable

    current = (TOKEN) lexer->yylex();
    if(current != ASSIGN)
        Error("caractères ':=' attendus");

    current = (TOKEN) lexer->yylex();

    // Analyser l'expression à droite et récupérer son type
    TYPES exprType = Expression();

    // Vérifier compatibilité des types
    if(exprType != varType)
        Error("types incompatibles dans l'affectation");

    // Générer le code assembleur pour stocker la valeur dans la variable
    cout << "\tpop %rax" << endl;
cout << "\tmovq %rax, " << variable << "(%rip)" << endl;
cout << "// AssignementStatement: stocke dans " << variable << endl;


}

//modification en tp4
void IfStatement(void){
    current = (TOKEN) lexer->yylex();  // Consommer IF

    // Analyse de la condition
    TYPES typeCond = Expression();

    // Vérifier que la condition est BOOLEAN
    if(typeCond != TYPE_BOOLEAN)
        Error("Condition IF doit être de type BOOLEAN");

    if(current != THEN)
        Error("THEN attendu");

    current = (TOKEN) lexer->yylex();

    // Génération des labels
    unsigned long tag = ++TagNumber;

    // Génération code assembleur test condition
    cout << "\tpop %rax" << endl;
    cout << "\tcmpq $0, %rax" << endl;
    cout << "\tje Sinon" << tag << endl;

    // Bloc THEN
    Statement();

    cout << "\tjmp FinSi" << tag << endl;
    cout << "Sinon" << tag << ":" << endl;

    // Bloc ELSE optionnel
    if(current == ELSE){
        current = (TOKEN) lexer->yylex();
        Statement();
    }

    cout << "FinSi" << tag << ":" << endl;
}


// modification en tp4
void WhileStatement(void){
    unsigned long tagStart = ++TagNumber;
    unsigned long tagEnd = ++TagNumber;

    current = (TOKEN) lexer->yylex();  // Consommer WHILE

    cout << "Boucle" << tagStart << ":" << endl;

    // Analyse de la condition
    TYPES typeCond = Expression();

    // Vérifier que la condition est BOOLEAN
    if(typeCond != TYPE_BOOLEAN)
        Error("Condition WHILE doit être de type BOOLEAN");

    // Génération code assembleur test condition
    cout << "\tpop %rax" << endl;
    cout << "\tcmpq $0, %rax" << endl;
    cout << "\tje FinBoucle" << tagEnd << endl;

    if(current != DO)
        Error("DO attendu");

    current = (TOKEN) lexer->yylex();

    // Corps de la boucle
    Statement();

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
    current = (TOKEN) lexer->yylex();

    Statement();

    while(current == SEMICOLON){
        current = (TOKEN) lexer->yylex();
        if(current == END_TOKEN)
            break;
        Statement();
    }

    if(current != END_TOKEN)
        Error("END attendu");
    current = (TOKEN) lexer->yylex();

    // Ne pas gérer le DOT ici, c'est géré dans StatementPart()
}



//tp 5 
void DisplayStatement(void) {
    current = (TOKEN) lexer->yylex();  // consommer DISPLAY
    TYPES typeExpr = Expression();
    if(typeExpr != UNSIGNED_INT)
        Error("DISPLAY ne fonctionne que pour les nombres entiers non signés");
    cout << "\tpop %rsi" << endl;
    cout << "\tleaq FormatString1(%rip), %rdi" << endl;
    cout << "\tmovq $0, %rax" << endl;
    cout << "\tcall printf@PLT" << endl;
    cout << "// DisplayStatement called" << endl;

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
        case DISPLAY:      // <-- ici la gestion de DISPLAY
            DisplayStatement();
            break;
        case END_TOKEN:
            Error("Instruction attendue (pas END)");
            break;
        default:
            Error("Instruction attendue");
    }
}





// -- AJOUT de la fonction Program() ---

void StatementPart(void) {
   cout << "\t.text" << endl;
    cout << "\t.globl main" << endl;
    cout << "main:" << endl;
    cout << "\tpush %rbp" << endl;           // <<-- IMPORTANT, à rajouter
    cout << "\tmovq %rsp, %rbp" << endl;

    Statement();

    while(current == SEMICOLON){
        current = (TOKEN) lexer->yylex();
        if(current == END_TOKEN)
            Error("Instruction attendue (pas END)");
        if(current == DOT)
            break;
        Statement();
    }

    if(current != DOT)
        Error("caractère '.' attendu");
    current = (TOKEN) lexer->yylex();


 cout << "\tmovq %rbp, %rsp\t# Restore stack pointer" << endl;
cout << "\tpop %rbp\t# Restore base pointer" << endl;
cout << "\tret\t# Return from main" << endl;

    // jai commenter les cout pour le tp5 
//cout << "\tmovq a(%rip), %rsi" << endl;
//cout << "\tleaq FormatString1(%rip), %rdi" << endl;
//cout << "\tmovq $0, %rax" << endl;
//cout << "\tcall printf@PLT" << endl;
}



//modification tp6
void Program(void){
    cout << "\t.data" << endl;
    cout << "\t.align 8" << endl;

    // Déclare la chaîne de format pour printf (toujours présente)
    cout << "FormatString1:\t.string \"%llu\\n\"\t# used by printf to display 64-bit unsigned integers" << endl;

    // Ensuite, si on a des variables, on les déclare ici
    if(current == VAR)
        DeclarationPart();  // ça imprime des lignes de variables ex: a: .quad 0

    cout << "\t.text" << endl;
    cout << "\t.globl main" << endl;
    cout << "main:" << endl;
    cout << "\tpush %rbp" << endl;
    cout << "\tmovq %rsp, %rbp" << endl;

    if(current == BEGIN_TOKEN){
        BlockStatement();
        if(current != DOT)
            Error("caractère '.' attendu après END");
        current = (TOKEN) lexer->yylex();
    }
    else {
        StatementPart();
    }

    cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
    cout << "\tpop %rbp\t\t\t# Restore old base pointer" << endl;
    cout << "\tret\t\t\t# Return from main function" << endl;
}






int main(int argc, char* argv[]) {
    if(argc < 2) {
        cerr << "Usage: " << argv[0] << " <fichier_source>" << endl;
        return 1;
    }
    // Ouvre le fichier et redirige stdin dessus
    FILE* file = fopen(argv[1], "r");
    if(!file) {
        cerr << "Erreur ouverture fichier : " << argv[1] << endl;
        return 1;
    }
    // Rediriger stdin vers ce fichier (pour que Flex lise dessus)
    if(dup2(fileno(file), fileno(stdin)) == -1) {
        perror("dup2");
        return 1;
    }
    fclose(file);

    cout << "\t\t\t# This code was produced by the CERI Compiler" << endl;
    current = (TOKEN) lexer->yylex();

    DeclaredVariables["TRUE"] = TYPE_BOOLEAN;
DeclaredVariables["FALSE"] = TYPE_BOOLEAN;

    Program();
    cout << "\tmovq %rbp, %rsp\t\t# Restore the position of the stack's top" << endl;
    cout << "\tret\t\t\t# Return from main function" << endl;
    if(current != FEOF) {
        cerr << "Caractères en trop à la fin du programme : [" << current << "]";
        Error(".");
    }
    return 0;
}
