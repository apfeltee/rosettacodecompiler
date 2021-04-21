
#include "shared.h"


static TokenData_t tok;
static FILE *source_fp;
static FILE *dest_fp;


int analyzer_getEnum(const char *name);
TokenData_t analyzer_getToken();
void analyzer_expect(const char *msg, int s);
Tree_t *analyzer_doExpr(int p);
Tree_t *analyzer_doParenExpr();
Tree_t *analyzer_doStatement();
Tree_t *analyzer_parse();
int analyzer_printAst(Tree_t *t);
int analyzer_main(int argc, char *argv[]);


static void copy2vec(std::vector<char>& dv, const char* str, size_t len=0)
{
    size_t i;
    if(len == 0)
    {
        len = strlen(str);
    }
    fprintf(stderr, "copy2vec: str(%d)=%s\n", len, str);
    if(len > 0)
    {
        for(i=0; i<len; i++)
        {
            dv.push_back(str[i]);
        }
    }
}

Tree_t* analyzer_makeNode(int node_type, Tree_t* left, Tree_t* right)
{
    Tree_t* t;
    //t = (Tree_t*)calloc(sizeof(Tree_t), 1);
    t = new Tree_t;
    t->node_type = node_type;
    t->left = left;
    t->right = right;
    return t;
}

Tree_t* analyzer_makeLeaf(NodeType node_type, const std::vector<char>& cvec)
{
    Tree_t* t;
    {
        fprintf(stderr, "analyzer_makeLeaf:value(%d)=%.*s\n", int(cvec.size()), int(cvec.size()), cvec.data());
    }
    t = new Tree_t;
    t->node_type = node_type;
    t->svalue = std::string(cvec.data(), cvec.size());
    return t;
}

int analyzer_getEnum(const char* name)
{
    size_t i;
    for(i = 0; anattr_data[i].text != NULL; i++)
    {
        if(anattr_data[i].enum_text != NULL)
        {
            if(strcmp(anattr_data[i].enum_text, name) == 0)
            {
                return anattr_data[i].tok;
            }
        }
    }
    error(0, 0, "Unknown token %s\n", name);
    return 0;
}

TokenData_t analyzer_getToken()
{
    
    int len;
    TokenData_t tok;
    char* p;
    char* name;
    char* yytext;
    char inbuf[MAX_LINELENGTH + 1];

    len = read_line(source_fp, inbuf, MAX_LINELENGTH);
    yytext = inbuf;
    yytext = rtrim(yytext, &len);
    // [ ]*{lineno}[ ]+{colno}[ ]+token[ ]+optional
    // get line and column
    tok.err_ln = atoi(strtok(yytext, " "));
    tok.err_col = atoi(strtok(NULL, " "));

    // get the token name
    name = strtok(NULL, " ");
    tok.tok = analyzer_getEnum(name);

    // if there is extra data, get it
    p = name + strlen(name);
    if(p != &yytext[len])
    {
        for(++p; isspace((int)(*p)); ++p)
        {
        }
        copy2vec(tok.text, p);
    }
    return tok;
}
void analyzer_expect(const char* msg, int s)
{
    if(tok.tok == s)
    {
        tok = analyzer_getToken();
        return;
    }
    error(tok.err_ln, tok.err_col, "%s: Expecting '%s', found '%s'\n",
        msg,
        anattr_data[s].text,
        anattr_data[tok.tok].text
    );
}

Tree_t* analyzer_doExpr(int p)
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
                x = analyzer_doParenExpr();
            }
            break;
        case tk_Sub:
        case tk_Add:
            {
                op = tok.tok;
                tok = analyzer_getToken();
                node = analyzer_doExpr(anattr_data[tk_Negate].precedence);
                if(op == tk_Sub)
                {
                    x = analyzer_makeNode(nd_Negate, node, NULL);
                }
                else
                {
                    x = node;
                }
            }
            break;
        case tk_Not:
            {
                tok = analyzer_getToken();
                x = analyzer_makeNode(nd_Not, analyzer_doExpr(anattr_data[tk_Not].precedence), NULL);
            }
            break;
        case tk_Ident:
            {
                x = analyzer_makeLeaf(nd_Ident, tok.text);
                tok = analyzer_getToken();
            }
            break;
        case tk_Integer:
            {
                x = analyzer_makeLeaf(nd_Integer, tok.text);
                tok = analyzer_getToken();
            }
            break;
        default:
            error(tok.err_ln, tok.err_col, "Expecting a primary, found: %s\n", anattr_data[tok.tok].text);
    }

    while(anattr_data[tok.tok].is_binary && anattr_data[tok.tok].precedence >= p)
    {
        op = tok.tok;
        tok = analyzer_getToken();
        q = anattr_data[op].precedence;
        if(!anattr_data[op].right_associative)
        {
            q++;
        }
        node = analyzer_doExpr(q);
        x = analyzer_makeNode(anattr_data[op].node_type, x, node);
    }
    return x;
}

Tree_t* analyzer_doParenExpr()
{
    Tree_t* t;
    analyzer_expect("analyzer_doParenExpr", tk_Lparen);
    t = analyzer_doExpr(0);
    analyzer_expect("analyzer_doParenExpr", tk_Rparen);
    return t;
}

Tree_t* analyzer_doStatement()
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
                tok = analyzer_getToken();
                e = analyzer_doParenExpr();
                s = analyzer_doStatement();
                s2 = NULL;
                if(tok.tok == tk_Else)
                {
                    tok = analyzer_getToken();
                    s2 = analyzer_doStatement();
                }
                t = analyzer_makeNode(nd_If, e, analyzer_makeNode(nd_If, s, s2));
            }
            break;
        case tk_Putc:
            {
                tok = analyzer_getToken();
                e = analyzer_doParenExpr();
                t = analyzer_makeNode(nd_Prtc, e, NULL);
                analyzer_expect("Putc", tk_Semi);
            }
            break;
        /* print '(' analyzer_doExpr {',' analyzer_doExpr} ')' */
        case tk_Print:
            {
                tok = analyzer_getToken();
                for(analyzer_expect("Print", tk_Lparen);; analyzer_expect("Print", tk_Comma))
                {
                    if(tok.tok == tk_String)
                    {
                        e = analyzer_makeNode(nd_Prts, analyzer_makeLeaf(nd_String, tok.text), NULL);
                        tok = analyzer_getToken();
                    }
                    else
                        e = analyzer_makeNode(nd_Prti, analyzer_doExpr(0), NULL);

                    t = analyzer_makeNode(nd_Sequence, t, e);

                    if(tok.tok != tk_Comma)
                        break;
                }
                analyzer_expect("Print", tk_Rparen);
                analyzer_expect("Print", tk_Semi);
            }
            break;
        case tk_Semi:
            {
                tok = analyzer_getToken();
            }
            break;
        case tk_Ident:
            {
                v = analyzer_makeLeaf(nd_Ident, tok.text);
                tok = analyzer_getToken();
                analyzer_expect("assign", tk_Assign);
                e = analyzer_doExpr(0);
                t = analyzer_makeNode(nd_Assign, v, e);
                analyzer_expect("assign", tk_Semi);
            }
            break;
        case tk_While:
            {
                tok = analyzer_getToken();
                e = analyzer_doParenExpr();
                s = analyzer_doStatement();
                t = analyzer_makeNode(nd_While, e, s);
            }
            break;
        /* {analyzer_doStatement} */
        case tk_Lbrace:
            {
                for(analyzer_expect("Lbrace", tk_Lbrace); (tok.tok != tk_Rbrace) && (tok.tok != tk_EOI);)
                {
                    t = analyzer_makeNode(nd_Sequence, t, analyzer_doStatement());
                }
                analyzer_expect("Lbrace", tk_Rbrace);
            }
            break;
        case tk_EOI:
            {
                /* nop */
            }
            break;
        default:
            error(tok.err_ln, tok.err_col, "expecting start of statement, found '%s'\n",
                  anattr_data[tok.tok].text);
    }
    return t;
}

Tree_t* analyzer_parse()
{
    Tree_t* t;
    t = NULL;
    tok = analyzer_getToken();
    do
    {
        t = analyzer_makeNode(nd_Sequence, t, analyzer_doStatement());
    } while(t != NULL && tok.tok != tk_EOI);
    return t;
}

int analyzer_printAst(Tree_t* t)
{
    int i;
    bool found;
    found = false;
    if(t == NULL)
    {
        fprintf(dest_fp, ";\n");
    }
    else
    {
        for(i=0; displaynodes_data[i].str != NULL; i++)
        {
            if(displaynodes_data[i].tok == t->node_type)
            {
                found = true;
                fprintf(dest_fp, "%-14s ", displaynodes_data[i].str);
            }
        }
        if(!found)
        {
            fprintf(stderr, "analyzer: analyzer_printAst: did not found node for type %d!\n", t->node_type);
            return 1;
        }
        if(t->node_type == nd_Ident || t->node_type == nd_Integer || t->node_type == nd_String)
        {
            fprintf(dest_fp, "%s\n", t->svalue.data());
        }
        else
        {
            fprintf(dest_fp, "\n");
            analyzer_printAst(t->left);
            analyzer_printAst(t->right);
        }
    }
    return 0;
}


int analyzer_main(int argc, char* argv[])
{
    Tree_t* t;
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");
    t = analyzer_parse();
    if(t != NULL)
    {
        return analyzer_printAst(t);
    }
    fprintf(stderr, "analyzer: analyzer_parse() failed\n");
    return 1;
}
