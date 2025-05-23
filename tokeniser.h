#ifndef TOKENISER_H
#define TOKENISER_H

enum TOKEN {
    FEOF,
    UNKNOWN,
    KEYWORD,
    NUMBER,
    ID,
    CHARCONST,
    RBRACKET,
    LBRACKET,
    RPARENT,
    LPARENT,
    COMMA,
    SEMICOLON,
    COLON,
    DOT,
    ADDOP,
    MULOP,
    RELOP,
    NOT,
    ASSIGN,
    VAR,
    BEGIN_TOKEN,
    END_TOKEN,
    IF,
    THEN,
    ELSE,
    WHILE,
    DO,
    FOR,
    TO,
    DISPLAY,
    TRUE_CONST,
    FALSE_CONST,
    DOUBLECONST
};




#endif
