
#include "shared.h"

typedef struct keywordinfo_tag_t keywordinfo_t;
struct keywordinfo_tag_t
{
    const char* s;
    int sym;
};


da_dim(lexer_text, char);


static const keywordinfo_t kwds[] =
{
    {"else",  tk_Else},
    {"if",    tk_If},
    {"print", tk_Print},
    {"putc",  tk_Putc},
    {"while", tk_While},
};

static FILE* source_fp;
static FILE* dest_fp;
static int line = 1;
static int col = 0;
static int the_ch = ' ';


static TokenData_t gettok();

/* get next char from input */
static int next_ch()
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

static TokenData_t char_lit(int n, int err_line, int err_col)
{
    if(the_ch == '\'')
    {
        error(err_line, err_col, "gettok: empty character constant");
    }
    if(the_ch == '\\')
    {
        next_ch();
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
            error(err_line, err_col, "gettok: unknown escape sequence \\%c", the_ch);
        }
    }
    if(next_ch() != '\'')
    {
        error(err_line, err_col, "multi-character constant");
    }
    next_ch();
    return (TokenData_t){ tk_Integer, err_line, err_col, n, NULL };
}

static TokenData_t div_or_cmt(int err_line, int err_col)
{ /* process divide or comments */
    if(the_ch != '*')
    {
        return (TokenData_t){ tk_Div, err_line, err_col, 0, NULL};
    }
    /* comment found */
    next_ch();
    for(;;)
    {
        if(the_ch == '*')
        {
            if(next_ch() == '/')
            {
                next_ch();
                return gettok();
            }
        }
        else if(the_ch == EOF)
        {
            error(err_line, err_col, "EOF in comment");
        }
        else
        {
            next_ch();
        }
    }
}

static TokenData_t string_lit(int start, int err_line, int err_col)
{
    /* "st" */
    da_rewind(lexer_text);
    while(next_ch() != start)
    {
        if(the_ch == '\n')
        {
            error(err_line, err_col, "EOL in string");
        }
        if(the_ch == EOF)
        {
            error(err_line, err_col, "EOF in string");
        }
        da_append(lexer_text, (char)the_ch, char);
    }
    da_append(lexer_text, '\0', char);
    next_ch();
    return (TokenData_t){ tk_String, err_line, err_col, 0, lexer_text};
}

static int kwd_cmp(const void* p1, const void* p2)
{
    return strcmp(*(char**)p1, *(char**)p2);
}

static int get_ident_type(const char* ident)
{
    keywordinfo_t *kwp;
    //return (kwp = bsearch(&ident, kwds, NELEMS(kwds), sizeof(kwds[0]), kwd_cmp)) == NULL ? tk_Ident : kwp->sym;
    kwp = (keywordinfo_t*)bsearch((void*)&ident, kwds, NELEMS(kwds), sizeof(kwds[0]), kwd_cmp);
    if(kwp == NULL)
    {
        return tk_Ident;
    }
    return kwp->sym;
}

static TokenData_t ident_or_int(int err_line, int err_col)
{
    long long n;
    bool is_number;
    is_number = true;
    da_rewind(lexer_text);
    while(isalnum(the_ch) || the_ch == '_')
    {
        da_append(lexer_text, (char)the_ch, char);
        if(!isdigit(the_ch))
        {
            is_number = false;
        }
        next_ch();
    }
    if(da_len(lexer_text) == 0)
    {
        error(err_line, err_col, "gettok: unrecognized character (%d) '%c'\n", the_ch, the_ch);
    }
    da_append(lexer_text, '\0', char);
    if(isdigit((int)(lexer_text[0])))
    {
        if(!is_number)
        {
            error(err_line, err_col, "invalid number: %s\n", lexer_text);
        }
        n = strtol(lexer_text, NULL, 0);
        if((n == (LONG_MAX - 1)) || (errno == ERANGE))
        {
            error(err_line, err_col, "Number exceeds maximum value");
        }
        return (TokenData_t){ tk_Integer, err_line, err_col, n, NULL };
    }
    return (TokenData_t){ get_ident_type(lexer_text), err_line, err_col, 0, lexer_text};
}

static TokenData_t follow(int expect, TokenType ifyes, TokenType ifno, int err_line, int err_col)
{ /* look ahead for '>=', etc. */
    if(the_ch == expect)
    {
        next_ch();
        return (TokenData_t){ ifyes, err_line, err_col, 0, NULL };
    }
    if(ifno == tk_EOI)
    {
        error(err_line, err_col, "follow: unrecognized character '%c' (%d)\n", the_ch, the_ch);
    }
    return (TokenData_t){ ifno, err_line, err_col, 0 , NULL};
}

static TokenData_t gettok()
{
    int err_line;
    int err_col;
    /* return the token type */
    /* skip white space */
    while(isspace(the_ch))
    {
        next_ch();
    }
    err_line = line;
    err_col = col;
    switch(the_ch)
    {
        case '{':
            {
                next_ch();
                return (TokenData_t){ tk_Lbrace, err_line, err_col, 0 , NULL };
            }
            break;
        case '}':
            {
                next_ch();
                return (TokenData_t){ tk_Rbrace, err_line, err_col, 0 , NULL};
            }
            break;
        case '(':
            {
                next_ch();
                return (TokenData_t){ tk_Lparen, err_line, err_col, 0, NULL };
            }
        case ')':
            {
                next_ch();
                return (TokenData_t){ tk_Rparen, err_line, err_col,  0, NULL };
            }
            break;
        case '+':
            {
                next_ch();
                return (TokenData_t){ tk_Add, err_line, err_col, 0 , NULL};
            }
        case '-':
            {
                next_ch();
                return (TokenData_t){ tk_Sub, err_line, err_col,  0 , NULL};
            }
            break;
        case '*':
            {
                next_ch();
                return (TokenData_t){ tk_Mul, err_line, err_col,  0 , NULL};
            }
            break;
        case '%':
            {
                next_ch();
                return (TokenData_t){ tk_Mod, err_line, err_col,  0 , NULL };
            }
            break;
        case ';':
            {
                next_ch();
                return (TokenData_t){ tk_Semi, err_line, err_col, 0, NULL };
            }
            break;
        case ',':
            {
                next_ch();
                return (TokenData_t){ tk_Comma, err_line, err_col, 0, NULL};
            }
            break;
        case '/':
            {
                next_ch();
                return div_or_cmt(err_line, err_col);
            }
            break;
        case '\'':
            {
                next_ch();
                return char_lit(the_ch, err_line, err_col);
            }
            break;
        case '<':
            {
                next_ch();
                return follow('=', tk_Leq, tk_Lss, err_line, err_col);
            }
            break;
        case '>':
            {
                next_ch();
                return follow('=', tk_Geq, tk_Gtr, err_line, err_col);
            }
            break;
        case '=':
            {
                next_ch();
                return follow('=', tk_Eql, tk_Assign, err_line, err_col);
            }
            break;
        case '!':
            {
                next_ch();
                return follow('=', tk_Neq, tk_Not, err_line, err_col);
            }
            break;
        case '&':
            {
                next_ch();
                return follow('&', tk_And, tk_EOI, err_line, err_col);
            }
            break;
        case '|':
            {
                next_ch();
                return follow('|', tk_Or, tk_EOI, err_line, err_col);
            }
            break;
        case '"':
            {
                return string_lit(the_ch, err_line, err_col);
            }
            break;
        case EOF:
            {
                return (TokenData_t){ tk_EOI, err_line, err_col, 0 , NULL};
            }
            break;
        default:
            {
                return ident_or_int(err_line, err_col);
            }
            break;
    }
}

static void run()
{
    int i;
    bool found;
    TokenData_t tok;



    do
    {
        tok = gettok();
        found = false;
        //fprintf(dest_fp, "%5d  %5d %.15s", tok.err_ln, tok.err_col, toks[tok.tok]);
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
        }
        if(tok.tok == tk_Integer)
        {
            fprintf(dest_fp, "  %4lld", tok.n);
        }
        else if(tok.tok == tk_Ident)
        {
            fprintf(dest_fp, " %s", tok.text);
        }
        else if(tok.tok == tk_String)
        {
            fprintf(dest_fp, " \"%s\"", tok.text);
        }
        fprintf(dest_fp, "\n");
    } while(tok.tok != tk_EOI);
    if(dest_fp != stdout)
    {
        fclose(dest_fp);
    }
}

int lexer_main(int argc, char* argv[])
{
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");
    run();
    return 0;
}

