
#include "shared.h"

std::vector<std::string> ast_string_pool;
std::vector<std::string> ast_global_names;
std::vector<int> ast_global_values;

static FILE *source_fp;
static FILE *dest_fp;


int ast_interp(Tree_t *x);
int ast_getEnumValue(const char name[]);
Tree_t *ast_loadAst(FILE *fp);
int astint_main(int argc, char *argv[]);

Tree_t* ast_makeLeaf(int node_type, int value)
{
    Tree_t* t;
    t = new Tree_t;
    t->node_type = node_type;
    t->ivalue = value;
    return t;
}


int ast_fetchVarOffset(const std::string& name)
{
    int i;
    int n;
    for(i = 0; i < ast_global_names.size(); ++i)
    {
        if(name == ast_global_names[i])
        {
            return i;
        }
    }
    ast_global_names.push_back(name);
    ast_global_values.push_back(0);
    return ast_global_names.size();
}


int ast_fetchStringOffset(char* st)
{
    int i;
    int n;
    int len;
    char* p;
    char* q;
    n = 0;
    len = strlen(st);
    st[len - 1] = '\0';
    st++;
    p = q = st;
    while((*p++ = *q++) != '\0')
    {
        if(q[-1] == '\\')
        {
            if(q[0] == 'n')
            {
                p[-1] = '\n';
                ++q;
            }
            else if(q[0] == '\\')
            {
                ++q;
            }
        }
    }
    for(i = 0; i < ast_string_pool.size(); ++i)
    {
        if(ast_string_pool[i] == st)
        {
            return i;
        }
    }
    ast_string_pool.push_back(st);
    return ast_string_pool.size();
}




int ast_getEnumValue(const char* name)
{
    size_t i;
    for(i = 0; anattr_data[i].enum_text != NULL; i++)
    {
        if(strcmp(anattr_data[i].enum_text, name) == 0)
        {
            return anattr_data[i].node_type;
        }
    }
    error(0, 0, "Unknown token %s\n", name);
    return -1;
}

Tree_t* ast_loadAst(FILE* fp)
{
    int n;
    int len;
    int node_type;
    char* p;
    char* tok;
    char* yytext;
    char inbuf[MAX_LINELENGTH + 1];
    Tree_t* left;
    Tree_t* right;
    n = 0;
    len = read_line(fp, inbuf, MAX_LINELENGTH);
    yytext = inbuf;
    yytext = rtrim(yytext, &len);

    // get first token
    tok = strtok(yytext, " ");

    if(tok[0] == ';')
    {
        return NULL;
    }
    node_type = ast_getEnumValue(tok);
    // if there is extra data, get it
    p = tok + strlen(tok);
    if(p != &yytext[len])
    {
        for(++p; isspace((int)(*p)); ++p)
        {
        }
        switch(node_type)
        {
            case nd_Ident:
                {
                    n = ast_fetchVarOffset(p);
                }
                break;
            case nd_Integer:
                {
                    n = strtol(p, NULL, 0);
                }
                break;
            case nd_String:
                {
                    n = ast_fetchStringOffset(p);
                }
                break;
            default:
                error(0, 0, "Unknown node type: %s\n", p);
                break;
        }
        return ast_makeLeaf(node_type, n);
    }
    left = ast_loadAst(fp);
    right = ast_loadAst(fp);
    return analyzer_makeNode(node_type, left, right);
}

int ast_interp(Tree_t* x)
{
    /* interpret the parse tree */
    if(!x)
    {
        return 0;
    }
    /*
    * the returns make the break unnecessary, but not
    * every compiler is smart enough to know that...
    */
    switch(x->node_type)
    {
        case nd_Integer:
            {
                return x->ivalue;
            }
            break;
        case nd_Ident:
            {
                return ast_global_values[x->ivalue];
            }
            break;
        case nd_String:
            {
                return x->ivalue;
            }
            break;
        case nd_Assign:
            {
                return ast_global_values[x->left->ivalue] = ast_interp(x->right);
            }
            break;
        case nd_Add:
            {
                return ast_interp(x->left) + ast_interp(x->right);
            }
            break;
        case nd_Sub:
            {
                return ast_interp(x->left) - ast_interp(x->right);
            }
            break;
        case nd_Mul:
            {
                return ast_interp(x->left) * ast_interp(x->right);
            }
            break;
        case nd_Div:
            {
                return ast_interp(x->left) / ast_interp(x->right);
            }
            break;
        case nd_Mod:
            {
                return ast_interp(x->left) % ast_interp(x->right);
            }
            break;
        case nd_Lss:
            {
                return ast_interp(x->left) < ast_interp(x->right);
            }
            break;
        case nd_Gtr:
            {
                return ast_interp(x->left) > ast_interp(x->right);
            }
            break;
        case nd_Leq:
            {
                return ast_interp(x->left) <= ast_interp(x->right);
            }
            break;
        case nd_Eql:
            {
                return ast_interp(x->left) == ast_interp(x->right);
            }
            break;
        case nd_Neq:
            {
                return ast_interp(x->left) != ast_interp(x->right);
            }
            break;
        case nd_And:
            {
                return ast_interp(x->left) && ast_interp(x->right);
            }
            break;
        case nd_Or:
            {
                return ast_interp(x->left) || ast_interp(x->right);
            }
            break;
        case nd_Negate:
            {
                return -ast_interp(x->left);
            }
            break;
        case nd_Not:
            {
                return !ast_interp(x->left);
            }
            break;
        case nd_If:
            {
                if(ast_interp(x->left))
                {
                    ast_interp(x->right->left);
                }
                else
                {
                    ast_interp(x->right->right);
                }
                return 0;
            }
            break;
        case nd_While:
            {
                while(ast_interp(x->left))
                {
                    ast_interp(x->right);
                }
                return 0;
            }
            break;
        case nd_Prtc:
            {
                fprintf(dest_fp, "%c", ast_interp(x->left));
                return 0;
            }
            break;
        case nd_Prti:
            {
                fprintf(dest_fp, "%d", ast_interp(x->left));
                return 0;
            }
            break;
        case nd_Prts:
            {
                fprintf(dest_fp, "%s", ast_string_pool[ast_interp(x->left)].data());
                return 0;
            }
            break;
        case nd_Sequence:
            {
                ast_interp(x->left);
                ast_interp(x->right);
                return 0;
            }
            break;
        default:
            error(0, 0, "ast_interp: unknown tree type %d\n", x->node_type);
            return 1;
            break;
    }
    return 0;
}

int astint_main(int argc, char* argv[])
{
    Tree_t* x;
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");
    x = ast_loadAst(source_fp);
    return ast_interp(x);
}
