
#include "shared.h"

#define NELEMS(arr) (sizeof(arr) / sizeof(arr[0]))




static TokenData_t tok;
static FILE *source_fp;
static FILE *dest_fp;

/*
static const char* Display_nodes[]
= { "Identifier", "String",   "Integer",  "Sequence", "If",        "Prtc",     "Prts",
    "Prti",       "While",    "Assign",   "Negate",   "Not",       "Multiply", "Divide",
    "Mod",        "Add",      "Subtract", "Less",     "LessEqual", "Greater",  "GreaterEqual",
    "Equal",      "NotEqual", "And",      "Or" };
*/

static int get_enum(const char *name);
static TokenData_t gettok();
static void expect(const char *msg, int s);
static Tree_t *expr(int p);
static Tree_t *paren_expr();
static Tree_t *stmt();
static Tree_t *parse();
static void prt_ast(Tree_t *t);
int analyzer_main(int argc, char *argv[]);


static Tree_t* make_leaf(NodeType node_type, char* value)
{
    Tree_t* t;
    t = (Tree_t*)calloc(sizeof(Tree_t), 1);
    t->node_type = node_type;
    t->svalue = strdup(value);
    return t;
}


// return internal version of name
static int get_enum(const char* name)
{
    size_t i;
    for(i = 0; i < NELEMS(analyzer_atr); i++)
    {
        if(strcmp(analyzer_atr[i].enum_text, name) == 0)
        {
            return analyzer_atr[i].tok;
        }
    }
    error(0, 0, "Unknown token %s\n", name);
    return 0;
}

static TokenData_t gettok()
{
    
    int len;
    TokenData_t tok;
    char* p;
    char* name;
    char* yytext;
    char inbuf[MAX_LINELENGTH + 1];
    //yytext = read_line(source_fp, &len);
    len = read_line(source_fp, inbuf, MAX_LINELENGTH);
    yytext = inbuf;
    yytext = rtrim(yytext, &len);
    // [ ]*{lineno}[ ]+{colno}[ ]+token[ ]+optional
    // get line and column
    tok.err_ln = atoi(strtok(yytext, " "));
    tok.err_col = atoi(strtok(NULL, " "));

    // get the token name
    name = strtok(NULL, " ");
    tok.tok = get_enum(name);

    // if there is extra data, get it
    p = name + strlen(name);
    if(p != &yytext[len])
    {
        for(++p; isspace((int)(*p)); ++p)
        {
        }
        tok.text = strdup(p);
    }
    return tok;
}

static void expect(const char* msg, int s)
{
    if(tok.tok == s)
    {
        tok = gettok();
        return;
    }
    error(tok.err_ln, tok.err_col, "%s: Expecting '%s', found '%s'\n",
        msg,
        analyzer_atr[s].text,
        analyzer_atr[tok.tok].text
    );
}

static Tree_t* expr(int p)
{
    int q;
    int op;
    Tree_t *x;
    Tree_t *node;
    x = NULL;
    switch(tok.tok)
    {
        case tk_Lparen:
            {
                x = paren_expr();
            }
            break;
        case tk_Sub:
        case tk_Add:
            {
                op = tok.tok;
                tok = gettok();
                node = expr(analyzer_atr[tk_Negate].precedence);
                //x = (op == tk_Sub) ? make_node(nd_Negate, node, NULL) : node;
                if(op == tk_Sub)
                {
                    x = make_node(nd_Negate, node, NULL);
                }
                else
                {
                    x = node;
                }
            }
            break;
        case tk_Not:
            {
                tok = gettok();
                x = make_node(nd_Not, expr(analyzer_atr[tk_Not].precedence), NULL);
            }
            break;
        case tk_Ident:
            {
                x = make_leaf(nd_Ident, tok.text);
                tok = gettok();
            }
            break;
        case tk_Integer:
            {
                x = make_leaf(nd_Integer, tok.text);
                tok = gettok();
            }
            break;
        default:
            error(tok.err_ln, tok.err_col, "Expecting a primary, found: %s\n", analyzer_atr[tok.tok].text);
    }

    while(analyzer_atr[tok.tok].is_binary && analyzer_atr[tok.tok].precedence >= p)
    {
        op = tok.tok;
        tok = gettok();
        q = analyzer_atr[op].precedence;
        if(!analyzer_atr[op].right_associative)
        {
            q++;
        }
        node = expr(q);
        x = make_node(analyzer_atr[op].node_type, x, node);
    }
    return x;
}

static Tree_t* paren_expr()
{
    Tree_t* t;
    expect("paren_expr", tk_Lparen);
    t = expr(0);
    expect("paren_expr", tk_Rparen);
    return t;
}

static Tree_t* stmt()
{
    Tree_t *t;
    Tree_t* v;
    Tree_t* e;
    Tree_t* s;
    Tree_t* s2;
    t = NULL;
    switch(tok.tok)
    {
        case tk_If:
            {
                tok = gettok();
                e = paren_expr();
                s = stmt();
                s2 = NULL;
                if(tok.tok == tk_Else)
                {
                    tok = gettok();
                    s2 = stmt();
                }
                t = make_node(nd_If, e, make_node(nd_If, s, s2));
            }
            break;
        case tk_Putc:
            {
                tok = gettok();
                e = paren_expr();
                t = make_node(nd_Prtc, e, NULL);
                expect("Putc", tk_Semi);
            }
            break;
        case tk_Print: /* print '(' expr {',' expr} ')' */
            {
                tok = gettok();
                for(expect("Print", tk_Lparen);; expect("Print", tk_Comma))
                {
                    if(tok.tok == tk_String)
                    {
                        e = make_node(nd_Prts, make_leaf(nd_String, tok.text), NULL);
                        tok = gettok();
                    }
                    else
                        e = make_node(nd_Prti, expr(0), NULL);

                    t = make_node(nd_Sequence, t, e);

                    if(tok.tok != tk_Comma)
                        break;
                }
                expect("Print", tk_Rparen);
                expect("Print", tk_Semi);
            }
            break;
        case tk_Semi:
            {
                tok = gettok();
            }
            break;
        case tk_Ident:
            {
                v = make_leaf(nd_Ident, tok.text);
                tok = gettok();
                expect("assign", tk_Assign);
                e = expr(0);
                t = make_node(nd_Assign, v, e);
                expect("assign", tk_Semi);
            }
            break;
        case tk_While:
            {
                tok = gettok();
                e = paren_expr();
                s = stmt();
                t = make_node(nd_While, e, s);
            }
            break;
        case tk_Lbrace: /* {stmt} */
            {
                for(expect("Lbrace", tk_Lbrace); (tok.tok != tk_Rbrace) && (tok.tok != tk_EOI);)
                {
                    t = make_node(nd_Sequence, t, stmt());
                }
                expect("Lbrace", tk_Rbrace);
            }
            break;
        case tk_EOI:
            {
                /* nop */
            }
            break;
        default:
            error(tok.err_ln, tok.err_col, "expecting start of statement, found '%s'\n",
                  analyzer_atr[tok.tok].text);
    }
    return t;
}

static Tree_t* parse()
{
    Tree_t* t;
    t = NULL;
    tok = gettok();
    do
    {
        t = make_node(nd_Sequence, t, stmt());
    } while(t != NULL && tok.tok != tk_EOI);
    return t;
}

static void prt_ast(Tree_t* t)
{
    int i;
    bool found;
    found = false;
    if(t == NULL)
    {
        printf(";\n");
    }
    else
    {
        //printf("%-14s ", Display_nodes[t->node_type]);
        for(i=0; displaynodes_data[i].str != NULL; i++)
        {
            if(displaynodes_data[i].tok == t->node_type)
            {
                found = true;
                printf("%-14s ", displaynodes_data[i].str);
            }
        }
        if(!found)
        {
            fprintf(stderr, "analyzer: prt_ast: did not found node for type %d!\n", t->node_type);
        }
        if(t->node_type == nd_Ident || t->node_type == nd_Integer || t->node_type == nd_String)
        {
            printf("%s\n", t->svalue);
        }
        else
        {
            printf("\n");
            prt_ast(t->left);
            prt_ast(t->right);
        }
    }
}


int analyzer_main(int argc, char* argv[])
{
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");
    prt_ast(parse());
    return 0;
}
