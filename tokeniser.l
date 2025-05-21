%{
// This is our Lexical tokeniser 
// It should be compiled into cpp with :
// flex++ -d -otokeniser.cpp tokeniser.l 
// And then compiled into object with
// g++ -c tokeniser.cpp
// tokens can be read using lexer->yylex()
// lexer->yylex() returns the type of the lexicon entry (see enum TOKEN in tokeniser.h)
// and lexer->YYText() returns the lexicon entry as a string

#include "tokeniser.h"
#include <iostream>

using namespace std;

%}

%option noyywrap
%option c++
%option yylineno

stringconst  \"[^\n"]+\"
ws      [ \t\n\r]+
alpha   [A-Za-z]
digit   [0-9]
number  {digit}+
id	{alpha}({alpha}|{digit})*
addop	(\+|\-|\|\|)
mulop	(\*|\/|%|\&\&)
relop	(\<|\>|"=="|\<=|\>=|!=)
unknown [^\"A-Za-z0-9 \n\r\t\(\)\<\>\=\!\%\&\|\}\-\;\.]+

%%

{addop}		return ADDOP;
{mulop}		return MULOP;
{relop}		return RELOP;
{number}	return NUMBER;


"if"		return IF;
"then"		return THEN;
"else"		return ELSE;
"while"		return WHILE;
"do"		return DO;
"for"		return FOR;
"to"		return TO;
[Bb][Ee][Gg][Ii][Nn]     return BEGIN_TOKEN;
[Ee][Nn][Dd]             return END_TOKEN;
[Dd][Ii][Ss][Pp][Ll][Aa][Yy]  return DISPLAY;
[Vv][Aa][Rr]             return VAR;
[Bb][Oo][Oo][Ll][Ee][Aa][Nn]  return BOOLEAN;
[Ii][Nn][Tt][Ee][Gg][Ee][Rr]  return INTEGER;
":"         return COLON;
[Tt][Rr][Uu][Ee]    return TRUE_CONST;
[Ff][Aa][Ll][Ss][Ee] return FALSE_CONST;



{id}		return ID;
{stringconst}	return STRINGCONST;

"["		return RBRACKET;
"]"		return LBRACKET;
","		return COMMA;
";"		return SEMICOLON;
"."		return DOT;
":="		return ASSIGN;
"("		return RPARENT;
")"		return LPARENT;
"!"		return NOT;

<<EOF>>		return FEOF;

{ws}    {/* skip blanks and tabs */};

"(*"    { /* Skip comments between '(*' and '*)' */
		int c;
		while((c = yyinput()) != 0){
     			if(c == '*'){
     	    			if((c = yyinput()) == ')')
    	        			break;
    	     			else
  	          			unput(c);
  	   		}	
		}
	};

{unknown}	return UNKNOWN;

%%
