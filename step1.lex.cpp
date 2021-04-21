
#include "shared.h"

typedef struct keywordinfo_tag_t keywordinfo_t;
struct keywordinfo_tag_t
{
    const char* s;
    int sym;
};

std::vector<char> lexer_text;

static const keywordinfo_t kwds[] =
{
    {"else",  tk_Else},
    {"if",    tk_If},
    {"print", tk_Print},
    {"putc",  tk_Putc},
    {"while", tk_While},
    {NULL, 0}
};

static FILE* source_fp;
static FILE* dest_fp;
static int line = 1;
static int col = 0;
static int the_ch = ' ';


TokenData_t lexer_getToken();

/* get next char from input */
int lexer_nextChar()
{
    the_ch = getc(source_fp);
    col++;
    if(the_ch == '\n')
    {
        ++line;
        col = 0;
    }
    return the_ch;
}

TokenData_t lexer_parseCharLiteral(int n, int err_line, int err_col)
{
    if(the_ch == '\'')
    {
        error(err_line, err_col, "lexer_getToken: empty character constant");
    }
    if(the_ch == '\\')
    {
        lexer_nextChar();
        if(the_ch == 'n')
        {
            n = 10;
        }
        else if(the_ch == '\\')
        {
            n = '\\';
        }
        else
        {
            error(err_line, err_col, "lexer_getToken: unknown escape sequence \\%c", the_ch);
        }
    }
    if(lexer_nextChar() != '\'')
    {
        error(err_line, err_col, "multi-character constant");
    }
    lexer_nextChar();
    return (TokenData_t){ tk_Integer, err_line, err_col, n, {}};
}

TokenData_t lexer_parseDivOrComment(int err_line, int err_col)
{
    /* process divide or comments */
    if(the_ch != '*')
    {
        return (TokenData_t){ tk_Div, err_line, err_col, 0, {}};
    }
    /* comment found */
    lexer_nextChar();
    for(;;)
    {
        if(the_ch == '*')
        {
            if(lexer_nextChar() == '/')
            {
                lexer_nextChar();
                return lexer_getToken();
            }
        }
        else if(the_ch == EOF)
        {
            error(err_line, err_col, "EOF in comment");
        }
        else
        {
            lexer_nextChar();
        }
    }
}

TokenData_t lexer_parseStringLiteral(int start, int err_line, int err_col)
{
    lexer_text.clear();
    while(lexer_nextChar() != start)
    {
        if(the_ch == '\n')
        {
            error(err_line, err_col, "EOL in string");
        }
        if(the_ch == EOF)
        {
            error(err_line, err_col, "EOF in string");
        }
        lexer_text.push_back(the_ch);
    }
    lexer_text.push_back('\0');
    lexer_nextChar();
    return (TokenData_t){ tk_String, err_line, err_col, 0, lexer_text};
}

int kwd_cmp(const void* p1, const void* p2)
{
    return strcmp(*(char**)p1, *(char**)p2);
}

int lexer_getIdentType(const char* ident)
{
    size_t i;
    keywordinfo_t *kwp;

    for(i=0; kwds[i].s != NULL; i++)
    {
        if(strcmp(ident, kwds[i].s) == 0)
        {
            return kwds[i].sym;
        }
    }
    return tk_Ident;

    #if 0
    kwp = (keywordinfo_t*)bsearch((void*)&ident, kwds, NELEMS(kwds)-1, sizeof(kwds[0]), kwd_cmp);
    if(kwp == NULL)
    {
        return tk_Ident;
    }
    return kwp->sym;
    #endif
}

TokenData_t lexer_identOrNumber(int err_line, int err_col)
{
    long long n;
    bool is_number;
    is_number = true;
    lexer_text.clear();
    while(isalnum(the_ch) || the_ch == '_')
    {
        lexer_text.push_back(the_ch);
        if(!isdigit(the_ch))
        {
            is_number = false;
        }
        lexer_nextChar();
    }
    if(lexer_text.size() == 0)
    {
        error(err_line, err_col, "lexer_getToken: unrecognized character (%d) '%c'\n", the_ch, the_ch);
    }
    lexer_text.push_back('\0');
    if(isdigit((int)(lexer_text[0])))
    {
        if(!is_number)
        {
            error(err_line, err_col, "invalid number: %s\n", lexer_text.data());
        }
        n = strtol(lexer_text.data(), NULL, 0);
        if((n == (LONG_MAX - 1)) || (errno == ERANGE))
        {
            error(err_line, err_col, "Number exceeds maximum value");
        }
        return (TokenData_t){ tk_Integer, err_line, err_col, n, {} };
    }
    return (TokenData_t){ lexer_getIdentType(lexer_text.data()), err_line, err_col, 0, lexer_text};
}

TokenData_t lexer_follow(int expect, TokenType ifyes, TokenType ifno, int err_line, int err_col)
{
    /* look ahead for '>=', etc. */
    if(the_ch == expect)
    {
        lexer_nextChar();
        return (TokenData_t){ ifyes, err_line, err_col, 0, {}};
    }
    if(ifno == tk_EOI)
    {
        error(err_line, err_col, "lexer_follow: unrecognized character '%c' (%d)\n", the_ch, the_ch);
    }
    return (TokenData_t){ ifno, err_line, err_col, 0 , {}};
}

TokenData_t lexer_getToken()
{
    int err_line;
    int err_col;
    /* skip white space */
    while(isspace(the_ch))
    {
        lexer_nextChar();
    }
    err_line = line;
    err_col = col;
    switch(the_ch)
    {
        case '{':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Lbrace, err_line, err_col, 0 , {}};
            }
            break;
        case '}':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Rbrace, err_line, err_col, 0 , {}};
            }
            break;
        case '(':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Lparen, err_line, err_col, 0, {}};
            }
        case ')':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Rparen, err_line, err_col,  0, {}};
            }
            break;
        case '+':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Add, err_line, err_col, 0 , {}};
            }
        case '-':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Sub, err_line, err_col,  0 , {}};
            }
            break;
        case '*':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Mul, err_line, err_col,  0 , {}};
            }
            break;
        case '%':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Mod, err_line, err_col,  0 , {}};
            }
            break;
        case ';':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Semi, err_line, err_col, 0, {}};
            }
            break;
        case ',':
            {
                lexer_nextChar();
                return (TokenData_t){ tk_Comma, err_line, err_col, 0, {}};
            }
            break;
        case '/':
            {
                lexer_nextChar();
                return lexer_parseDivOrComment(err_line, err_col);
            }
            break;
        case '\'':
            {
                lexer_nextChar();
                return lexer_parseCharLiteral(the_ch, err_line, err_col);
            }
            break;
        case '<':
            {
                lexer_nextChar();
                return lexer_follow('=', tk_Leq, tk_Lss, err_line, err_col);
            }
            break;
        case '>':
            {
                lexer_nextChar();
                return lexer_follow('=', tk_Geq, tk_Gtr, err_line, err_col);
            }
            break;
        case '=':
            {
                lexer_nextChar();
                return lexer_follow('=', tk_Eql, tk_Assign, err_line, err_col);
            }
            break;
        case '!':
            {
                lexer_nextChar();
                return lexer_follow('=', tk_Neq, tk_Not, err_line, err_col);
            }
            break;
        case '&':
            {
                lexer_nextChar();
                return lexer_follow('&', tk_And, tk_EOI, err_line, err_col);
            }
            break;
        case '|':
            {
                lexer_nextChar();
                return lexer_follow('|', tk_Or, tk_EOI, err_line, err_col);
            }
            break;
        case '"':
            {
                return lexer_parseStringLiteral(the_ch, err_line, err_col);
            }
            break;
        case EOF:
            {
                return (TokenData_t){ tk_EOI, err_line, err_col, 0 , {}};
            }
            break;
        default:
            {
                return lexer_identOrNumber(err_line, err_col);
            }
            break;
    }
}

int lexer_run()
{
    int i;
    bool found;
    const char* sp;
    TokenData_t tok;
    do
    {
        tok = lexer_getToken();
        found = false;
        fprintf(dest_fp, "%5d %5d ", tok.err_ln, tok.err_col);
        for(i=0; lextokens_data[i].str != NULL; i++)
        {
            if(tok.tok == lextokens_data[i].tok)
            {
                fprintf(dest_fp, "%.15s", lextokens_data[i].str);
                found = true;
            }
        }
        if(found == false)
        {
            fprintf(stderr, "failed to find token id %d in list!\n", tok.tok);
            return 1;
        }
        if(tok.tok == tk_Integer)
        {
            fprintf(dest_fp, "  %4lld", tok.n);
        }
        else if(tok.tok == tk_Ident)
        {
            fprintf(dest_fp, " %s", tok.text.data());
        }
        else if(tok.tok == tk_String)
        {
            auto& vec = tok.text;
            fprintf(dest_fp, " \"%.*s\"", vec.size(), vec.data());
        }
        fprintf(dest_fp, "\n");
    } while(tok.tok != tk_EOI);
    if(dest_fp != stdout)
    {
        fclose(dest_fp);
    }
    return 0;
}

int lexer_main(int argc, char* argv[])
{
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");
    return lexer_run();
}

