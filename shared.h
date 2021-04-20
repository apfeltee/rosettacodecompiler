
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>

#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))


#define da_dim(name, type)  \
    type* name = NULL;      \
    int _qy_##name##_p = 0; \
    int _qy_##name##_max = 0

#define da_redim(name, type) \
    do \
    { \
        if(_qy_##name##_p >= _qy_##name##_max) \
        { \
            name = (type*)realloc(name, (_qy_##name##_max += 32) * sizeof(name[0])); \
        } \
    } while(0)

#define da_rewind(name) _qy_##name##_p = 0

#define da_append(name, x, type)          \
    do                              \
    {                               \
        da_redim(name, type);             \
        name[_qy_##name##_p++] = x; \
    } while(0)
#define da_len(name) _qy_##name##_p

#define da_add(name, type)      \
    do                    \
    {                     \
        da_redim(name, type);   \
        _qy_##name##_p++; \
    } while(0)



/*
lexer -> analyzer -> codegen -> vmachine
*/

enum
{
    MAX_LINELENGTH = 1024,
};

enum TokenType
{
    tk_EOI,
    tk_Mul,
    tk_Div,
    tk_Mod,
    tk_Add,
    tk_Sub,
    tk_Negate,
    tk_Not,
    tk_Lss,
    tk_Leq,
    tk_Gtr,
    tk_Geq,
    tk_Eql,
    tk_Neq,
    tk_Assign,
    tk_And,
    tk_Or,
    tk_If,
    tk_Else,
    tk_While,
    tk_Print,
    tk_Putc,
    tk_Lparen,
    tk_Rparen,
    tk_Lbrace,
    tk_Rbrace,
    tk_Semi,
    tk_Comma,
    tk_Ident,
    tk_Integer,
    tk_String
};


enum NodeType
{
    nd_Ident,
    nd_String,
    nd_Integer,
    nd_Sequence,
    nd_If,
    nd_Prtc,
    nd_Prts,
    nd_Prti,
    nd_While,
    nd_Assign,
    nd_Negate,
    nd_Not,
    nd_Mul,
    nd_Div,
    nd_Mod,
    nd_Add,
    nd_Sub,
    nd_Lss,
    nd_Leq,
    nd_Gtr,
    nd_Geq,
    nd_Eql,
    nd_Neq,
    nd_And,
    nd_Or
};


enum Code_t
{
    COD_FETCH,
    COD_STORE,
    COD_PUSH,
    COD_ADD,
    COD_SUB,
    COD_MUL,
    COD_DIV,
    COD_MOD,
    COD_LT,
    COD_GT,
    COD_LE,
    COD_GE,
    COD_EQ,
    COD_NE,
    COD_AND,
    COD_OR,
    COD_NEG,
    COD_NOT,
    COD_JMP,
    COD_JZ,
    COD_PRTC,
    COD_PRTS,
    COD_PRTI,
    COD_HALT
};


typedef unsigned char uchar;
typedef uchar code;
typedef enum TokenType TokenType;
typedef enum NodeType NodeType;
typedef enum Code_t Code_t;
typedef struct Tree_tag_t Tree_t;
typedef struct TokenData_tag_t TokenData_t;
typedef struct DependencyItem_tag_t DependencyItem_t;
typedef struct AttributeItem_tag_t AttributeItem_t;
typedef struct CodeItem_tag_t CodeItem_t;
typedef struct LexItem_tag_t LexItem_t;
typedef struct NodeItem_tag_t NodeItem_t;


struct Tree_tag_t
{
    int node_type;
    Tree_t* left;
    Tree_t* right;
    int ivalue;
    char* svalue;

};

struct TokenData_tag_t
{
    int tok;
    int err_ln, err_col;
    //union
    //{
        long long n; /* value for constants */
        char* text; /* text for idents */
    //};
};

struct DependencyItem_tag_t
{
    const char *text;
    const char* enum_text;
    int tok;
    bool right_associative;
    bool is_binary;
    bool is_unary;
    int precedence;
    int node_type;
};

struct AttributeItem_tag_t
{
    const char* enum_text;
    int node_type;
    int opcode;
};

struct LexItem_tag_t
{
    const char* str;
    int tok;
};

struct NodeItem_tag_t
{
    const char* str;
    int tok;
};

struct CodeItem_tag_t
{
    const char* text;
    int op;
};


static const NodeItem_t displaynodes_data[]=
{
    {"Identifier", nd_Ident},
    {"String", nd_String},
    {"Integer", nd_Integer},
    {"Sequence", nd_Sequence},
    {"If", nd_If},
    {"Prtc", nd_Prtc},
    {"Prts", nd_Prts},
    {"Prti", nd_Prti},
    {"While", nd_While},
    {"Assign", nd_Assign},
    {"Negate", nd_Negate},
    {"Not", nd_Not},
    {"Multiply", nd_Mul},
    {"Divide", nd_Div},
    {"Mod", nd_Mod},
    {"Add", nd_Add},
    {"Subtract", nd_Sub},
    {"Less", nd_Lss},
    {"LessEqual", nd_Leq},
    {"Greater", nd_Gtr},
    {"GreaterEqual", nd_Geq},
    {"Equal", nd_Eql},
    {"NotEqual", nd_Neq},
    {"And", nd_And},
    {"Or", nd_Or},
    {NULL, 0},
};


static const LexItem_t lextokens_data[] =
{
    {"End_of_input", tk_EOI},
    {"Op_multiply", tk_Mul},
    {"Op_divide", tk_Div},
    {"Op_mod", tk_Mod},
    {"Op_add", tk_Add},
    {"Op_subtract", tk_Sub},
    {"Op_negate", tk_Negate},
    {"Op_not", tk_Not},
    {"Op_less", tk_Lss},
    {"Op_lessequal", tk_Leq},
    {"Op_greater", tk_Gtr},
    {"Op_greaterequal", tk_Geq},
    {"Op_equal", tk_Eql},
    {"Op_notequal", tk_Neq},
    {"Op_assign", tk_Assign},
    {"Op_and", tk_And},
    {"Op_or", tk_Or},
    {"kwdIF", tk_If},
    {"kwdELSE", tk_Else},
    {"kwdWHILE", tk_While},
    {"kwdPRINT", tk_Print},
    {"kwdPUTC", tk_Putc},
    {"LeftParen", tk_Lparen},
    {"RightParen", tk_Rparen},
    {"LeftBrace", tk_Lbrace},
    {"RightBrace", tk_Rbrace},
    {"Semicolon", tk_Semi},
    {"Comma", tk_Comma},
    {"Identifier", tk_Ident},
    {"Integer", tk_Integer},
    {"String", tk_String},
    {NULL, 0}
};

static const CodeItem_t codemap_data[] =
{
    { "fetch", COD_FETCH },
    { "store", COD_STORE },
    { "push", COD_PUSH },
    { "add", COD_ADD },
    { "sub", COD_SUB },
    { "mul", COD_MUL },
    { "div", COD_DIV },
    { "mod", COD_MOD },
    { "lt", COD_LT },
    { "gt", COD_GT },
    { "le", COD_LE },
    { "ge", COD_GE },
    { "eq", COD_EQ },
    { "ne", COD_NE },
    { "and", COD_AND },
    { "or", COD_OR },
    { "neg", COD_NEG },
    { "not", COD_NOT },
    { "jmp", COD_JMP },
    { "jz", COD_JZ },
    { "prtc", COD_PRTC },
    { "prts", COD_PRTS },
    { "prti", COD_PRTI },
    { "halt", COD_HALT },
    {NULL, 0},
};

// dependency: Ordered by NodeType, must remain in same order as NodeType enum

static const AttributeItem_t codeattr_data[] =
{
    { "Identifier", nd_Ident, -1 },
    { "String", nd_String, -1 },
    { "Integer", nd_Integer, -1 },
    { "Sequence", nd_Sequence, -1 },
    { "If", nd_If, -1 },
    { "Prtc", nd_Prtc, -1 },
    { "Prts", nd_Prts, -1 },
    { "Prti", nd_Prti, -1 },
    { "While", nd_While, -1 },
    { "Assign", nd_Assign, -1 },
    { "Negate", nd_Negate, COD_NEG },
    { "Not", nd_Not, COD_NOT },
    { "Multiply", nd_Mul, COD_MUL },
    { "Divide", nd_Div, COD_DIV },
    { "Mod", nd_Mod, COD_MOD },
    { "Add", nd_Add, COD_ADD },
    { "Subtract", nd_Sub, COD_SUB },
    { "Less", nd_Lss, COD_LT },
    { "LessEqual", nd_Leq, COD_LE },
    { "Greater", nd_Gtr, COD_GT },
    { "GreaterEqual", nd_Geq, COD_GE },
    { "Equal", nd_Eql, COD_EQ },
    { "NotEqual", nd_Neq, COD_NE },
    { "And", nd_And, COD_AND },
    { "Or", nd_Or, COD_OR },
    {NULL, 0, 0}
};

// dependency: Ordered by tok, must remain in same order as TokenType enum
static const DependencyItem_t  anattr_data[] =
{
    { "EOI",    "End_of_input", tk_EOI,     false, false, false, -1, -1 },
    { "*",      "Op_multiply",  tk_Mul,     false, true, false, 13, nd_Mul },
    { "/",      "Op_divide",    tk_Div,     false, true, false, 13, nd_Div },
    { "%",      "Op_mod",       tk_Mod,     false, true, false, 13, nd_Mod },
    { "+",      "Op_add",       tk_Add,     false, true, false, 12, nd_Add },
    { "-",      "Op_subtract",  tk_Sub,     false, true, false, 12, nd_Sub },
    { "-",      "Op_negate",    tk_Negate,  false, false, true, 14, nd_Negate },
    { "!",      "Op_not",       tk_Not,     false, false, true, 14, nd_Not },
    { "<",      "Op_less",      tk_Lss,     false, true, false, 10, nd_Lss },
    { "<=",     "Op_lessequal", tk_Leq,     false, true, false, 10, nd_Leq },
    { ">",      "Op_greater",   tk_Gtr,     false, true, false, 10, nd_Gtr },
    { ">=",     "Op_grequal",   tk_Geq,     false, true, false, 10, nd_Geq },
    { "==",     "Op_equal",     tk_Eql,     false, true, false, 9, nd_Eql },
    { "!=",     "Op_notequal",  tk_Neq,     false, true, false, 9, nd_Neq },
    { "=",      "Op_assign",    tk_Assign,  false, false, false, -1, nd_Assign },
    { "&&",     "Op_and",       tk_And,     false, true, false, 5, nd_And },
    { "||",     "Op_or",        tk_Or,      false, true, false, 4, nd_Or },
    { "if",     "kwdIF",        tk_If,      false, false, false, -1, nd_If },
    { "else",   "kwdELSE",      tk_Else,    false, false, false, -1, -1 },
    { "while",  "kwdWHILE",     tk_While,   false, false, false, -1, nd_While },
    { "print",  "kwdPRINT",     tk_Print,   false, false, false, -1, -1 },
    { "putc",   "kwdPUTC",      tk_Putc,    false, false, false, -1, -1 },
    { "(",      "LeftParen",    tk_Lparen,  false, false, false, -1, -1 },
    { ")",      "RightParen",   tk_Rparen,  false, false, false, -1, -1 },
    { "{",      "LeftBrace",    tk_Lbrace,  false, false, false, -1, -1 },
    { "}",      "RightBrace",   tk_Rbrace,  false, false, false, -1, -1 },
    { ";",      "Semicolon",    tk_Semi,    false, false, false, -1, -1 },
    { ",",      "Comma",        tk_Comma,   false, false, false, -1, -1 },
    { "Ident",  "Identifier",   tk_Ident,   false, false, false, -1, nd_Ident },
    { "IntLit", "Integer",      tk_Integer, false, false, false, -1, nd_Integer },
    { "StrLit", "String",       tk_String,  false, false, false, -1, nd_String },
    {NULL,      NULL,           0,          false, false, false, -1, 0}
};


void vm_emit_int(int32_t n);
void vm_emit_byte(int c);
void error(int err_line, int err_col, const char *fmt, ...);
Tree_t *make_node(int node_type, Tree_t *left, Tree_t *right);
char *rtrim(char *text, int *len);
char* copystrn(const char* s, unsigned int len);
char* copystr(const char* s);
void init_io(FILE **fp, FILE *std, const char mode[], const char fn[]);
int read_line(FILE *source_fp, char* dest, int maxlen);

