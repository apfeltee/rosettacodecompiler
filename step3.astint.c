
#include "shared.h"

// dependency: Ordered by NodeType, must remain in same order as NodeType enum

da_dim(ast_string_pool, char*);
da_dim(ast_global_names, char*);
da_dim(ast_global_values, int);

static FILE *source_fp;
static FILE *dest_fp;

static int fetch_var_offset(const char *name);
static int fetch_string_offset(char *st);
static int interp(Tree_t *x);
static int get_enum_value(const char name[]);
static Tree_t *load_ast(FILE *fp);
int astint_main(int argc, char *argv[]);

static Tree_t* make_leaf(int node_type, int value)
{
    Tree_t* t;
    t = (Tree_t*)calloc(sizeof(Tree_t), 1);
    t->node_type = node_type;
    t->ivalue = value;
    return t;
}


static int fetch_var_offset(const char* name)
{
    int i;
    int n;
    for(i = 0; i < da_len(ast_global_names); ++i)
    {
        if(strcmp(name, ast_global_names[i]) == 0)
        {
            return i;
        }
    }
    da_add(ast_global_names, char*);
    n = da_len(ast_global_names) - 1;
    ast_global_names[n] = copystr(name);
    da_append(ast_global_values, 0, int);
    return n;
}


static int fetch_string_offset(char* st)
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
    for(i = 0; i < da_len(ast_string_pool); ++i)
    {
        if(strcmp(st, ast_string_pool[i]) == 0)
        {
            return i;
        }
    }
    da_add(ast_string_pool, char*);
    n = da_len(ast_string_pool) - 1;
    ast_string_pool[n] = copystr(st);
    return da_len(ast_string_pool) - 1;
}




static int get_enum_value(const char* name)
{
    size_t i;
    for(i = 0; i < sizeof(anattr_data) / sizeof(anattr_data[0]); i++)
    {
        if(strcmp(anattr_data[i].enum_text, name) == 0)
        {
            return anattr_data[i].node_type;
        }
    }
    error(0, 0, "Unknown token %s\n", name);
    return -1;
}

static Tree_t* load_ast(FILE* fp)
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
    //yytext = read_line(fp, &len);
    len = read_line(fp, inbuf, MAX_LINELENGTH);
    yytext = inbuf;
    yytext = rtrim(yytext, &len);

    // get first token
    tok = strtok(yytext, " ");

    if(tok[0] == ';')
    {
        return NULL;
    }
    node_type = get_enum_value(tok);
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
                    n = fetch_var_offset(p);
                }
                break;
            case nd_Integer:
                {
                    n = strtol(p, NULL, 0);
                }
                break;
            case nd_String:
                {
                    n = fetch_string_offset(p);
                }
                break;
            default:
                error(0, 0, "Unknown node type: %s\n", p);
                break;
        }
        return make_leaf(node_type, n);
    }
    left = load_ast(fp);
    right = load_ast(fp);
    return make_node(node_type, left, right);
}

static int interp(Tree_t* x)
{ /* interpret the parse tree */
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
                return ast_global_values[x->left->ivalue] = interp(x->right);
            }
            break;
        case nd_Add:
            {
                return interp(x->left) + interp(x->right);
            }
            break;
        case nd_Sub:
            {
                return interp(x->left) - interp(x->right);
            }
            break;
        case nd_Mul:
            {
                return interp(x->left) * interp(x->right);
            }
            break;
        case nd_Div:
            {
                return interp(x->left) / interp(x->right);
            }
            break;
        case nd_Mod:
            {
                return interp(x->left) % interp(x->right);
            }
            break;
        case nd_Lss:
            {
                return interp(x->left) < interp(x->right);
            }
            break;
        case nd_Gtr:
            {
                return interp(x->left) > interp(x->right);
            }
            break;
        case nd_Leq:
            {
                return interp(x->left) <= interp(x->right);
            }
            break;
        case nd_Eql:
            {
                return interp(x->left) == interp(x->right);
            }
            break;
        case nd_Neq:
            {
                return interp(x->left) != interp(x->right);
            }
            break;
        case nd_And:
            {
                return interp(x->left) && interp(x->right);
            }
            break;
        case nd_Or:
            {
                return interp(x->left) || interp(x->right);
            }
            break;
        case nd_Negate:
            {
                return -interp(x->left);
            }
            break;
        case nd_Not:
            {
                return !interp(x->left);
            }
            break;
        case nd_If:
            {
                if(interp(x->left))
                {
                    interp(x->right->left);
                }
                else
                {
                    interp(x->right->right);
                }
                return 0;
            }
            break;
        case nd_While:
            {
                while(interp(x->left))
                {
                    interp(x->right);
                }
                return 0;
            }
            break;
        case nd_Prtc:
            {
                fprintf(dest_fp, "%c", interp(x->left));
                return 0;
            }
            break;
        case nd_Prti:
            {
                fprintf(dest_fp, "%d", interp(x->left));
                return 0;
            }
            break;
        case nd_Prts:
            {
                fprintf(dest_fp, "%s", ast_string_pool[interp(x->left)]);
                return 0;
            }
            break;
        case nd_Sequence:
            {
                interp(x->left);
                interp(x->right);
                return 0;
            }
            break;
        default:
            error(0, 0, "interp: unknown tree type %d\n", x->node_type);
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
    x = load_ast(source_fp);
    return interp(x);
}
