

#include "shared.h"

da_dim(cgen_object, code);
da_dim(cgen_globals, char*);
da_dim(cgen_string_pool, char*);


static FILE* source_fp;
static FILE* dest_fp;
static int here = 0;


/* codegen.c */
static int fetch_var_offset(const char *id);
static int fetch_string_offset(const char *st);
static int hole(void);
static void fix(int src, int dst);
static void code_gen(Tree_t *x);
static void code_finish(void);
static void list_code(void);
static Tree_t *load_ast(FILE *fp);
int cgen_main(int argc, char *argv[]);

static Tree_t* make_leaf(int node_type, char* value)
{
    Tree_t* t;
    t = (Tree_t*)calloc(sizeof(Tree_t), 1);
    t->node_type = node_type;
    t->svalue = strdup(value);
    return t;
}


static int fetch_var_offset(const char* id)
{
    int i;
    int n;
    for(i = 0; i < da_len(cgen_globals); ++i)
    {
        if(strcmp(id, cgen_globals[i]) == 0)
        {
            return i;
        }
    }
    da_add(cgen_globals, char*);
    n = da_len(cgen_globals) - 1;
    cgen_globals[n] = strdup(id);
    return n;
}

static int fetch_string_offset(const char* st)
{
    int i;
    int n;
    for(i = 0; i < da_len(cgen_string_pool); ++i)
    {
        if(strcmp(st, cgen_string_pool[i]) == 0)
        {
            return i;
        }
    }
    da_add(cgen_string_pool, char*);
    n = da_len(cgen_string_pool) - 1;
    cgen_string_pool[n] = strdup(st);
    return n;
}

static int type_to_op(int type)
{
    return codegen_atr[type].opcode;
}

static void fix(int src, int dst)
{
    *(int32_t*)(cgen_object + src) = dst - src;
}

static void emit_byte(int c)
{
    da_append(cgen_object, (uchar)c, code);
    ++here;
}

static void emit_int(int32_t n)
{
    size_t i;
    union
    {
        int32_t n;
        unsigned char c[sizeof(int32_t)];
    } x;
    x.n = n;

    for(i = 0; i < sizeof(x.n); ++i)
    {
        emit_byte(x.c[i]);
    }
}



static int hole()
{
    int t = here;
    emit_int(0);
    return t;
}


void code_gen(Tree_t* x)
{
    int p1, p2, n;

    if(x == NULL)
        return;
    switch(x->node_type)
    {
        case nd_Ident:
            emit_byte(COD_FETCH);
            n = fetch_var_offset(x->svalue);
            emit_int(n);
            break;
        case nd_Integer:
            emit_byte(COD_PUSH);
            emit_int(atoi(x->svalue));
            break;
        case nd_String:
            emit_byte(COD_PUSH);
            n = fetch_string_offset(x->svalue);
            emit_int(n);
            break;
        case nd_Assign:
            n = fetch_var_offset(x->left->svalue);
            code_gen(x->right);
            emit_byte(COD_STORE);
            emit_int(n);
            break;
        case nd_If:
            code_gen(x->left);// if expr
            emit_byte(COD_JZ);// if false, jump
            p1 = hole();// make room for jump dest
            code_gen(x->right->left);// if true statements
            if(x->right->right != NULL)
            {
                emit_byte(COD_JMP);
                p2 = hole();
            }
            fix(p1, here);
            if(x->right->right != NULL)
            {
                code_gen(x->right->right);
                fix(p2, here);
            }
            break;
        case nd_While:
            p1 = here;
            code_gen(x->left);// while expr
            emit_byte(COD_JZ);// if false, jump
            p2 = hole();// make room for jump dest
            code_gen(x->right);// statements
            emit_byte(COD_JMP);// back to the top
            fix(hole(), p1);// plug the top
            fix(p2, here);// plug the 'if false, jump'
            break;
        case nd_Sequence:
            code_gen(x->left);
            code_gen(x->right);
            break;
        case nd_Prtc:
            code_gen(x->left);
            emit_byte(COD_PRTC);
            break;
        case nd_Prti:
            code_gen(x->left);
            emit_byte(COD_PRTI);
            break;
        case nd_Prts:
            code_gen(x->left);
            emit_byte(COD_PRTS);
            break;
        case nd_Lss:
        case nd_Gtr:
        case nd_Leq:
        case nd_Geq:
        case nd_Eql:
        case nd_Neq:
        case nd_And:
        case nd_Or:
        case nd_Sub:
        case nd_Add:
        case nd_Div:
        case nd_Mul:
        case nd_Mod:
            code_gen(x->left);
            code_gen(x->right);
            emit_byte(type_to_op(x->node_type));
            break;
        case nd_Negate:
        case nd_Not:
            code_gen(x->left);
            emit_byte(type_to_op(x->node_type));
            break;
        default:
            error(0, 0, "error in code generator - found %d, expecting operator\n", x->node_type);
    }
}

static void code_finish()
{
    emit_byte(COD_HALT);
}

static void list_code()
{
    fprintf(dest_fp, "Datasize: %d Strings: %d\n", da_len(cgen_globals), da_len(cgen_string_pool));
    for(int i = 0; i < da_len(cgen_string_pool); ++i)
    {
        fprintf(dest_fp, "%s\n", cgen_string_pool[i]);
    }
    code* pc = cgen_object;

again:
    fprintf(dest_fp, "%5d ", (int)(pc - cgen_object));
    switch(*pc++)
    {
        case COD_FETCH:
            fprintf(dest_fp, "fetch [%d]\n", *(int32_t*)pc);
            pc += sizeof(int32_t);
            goto again;
        case COD_STORE:
            fprintf(dest_fp, "store [%d]\n", *(int32_t*)pc);
            pc += sizeof(int32_t);
            goto again;
        case COD_PUSH:
            fprintf(dest_fp, "push  %d\n", *(int32_t*)pc);
            pc += sizeof(int32_t);
            goto again;
        case COD_ADD:
            fprintf(dest_fp, "add\n");
            goto again;
        case COD_SUB:
            fprintf(dest_fp, "sub\n");
            goto again;
        case COD_MUL:
            fprintf(dest_fp, "mul\n");
            goto again;
        case COD_DIV:
            fprintf(dest_fp, "div\n");
            goto again;
        case COD_MOD:
            fprintf(dest_fp, "mod\n");
            goto again;
        case COD_LT:
            fprintf(dest_fp, "lt\n");
            goto again;
        case COD_GT:
            fprintf(dest_fp, "gt\n");
            goto again;
        case COD_LE:
            fprintf(dest_fp, "le\n");
            goto again;
        case COD_GE:
            fprintf(dest_fp, "ge\n");
            goto again;
        case COD_EQ:
            fprintf(dest_fp, "eq\n");
            goto again;
        case COD_NE:
            fprintf(dest_fp, "ne\n");
            goto again;
        case COD_AND:
            fprintf(dest_fp, "and\n");
            goto again;
        case COD_OR:
            fprintf(dest_fp, "or\n");
            goto again;
        case COD_NOT:
            fprintf(dest_fp, "not\n");
            goto again;
        case COD_NEG:
            fprintf(dest_fp, "neg\n");
            goto again;
        case COD_JMP:
            fprintf(dest_fp, "jmp    (%d) %d\n", *(int32_t*)pc, (int32_t)(pc + *(int32_t*)pc - cgen_object));
            pc += sizeof(int32_t);
            goto again;
        case COD_JZ:
            fprintf(dest_fp, "jz     (%d) %d\n", *(int32_t*)pc, (int32_t)(pc + *(int32_t*)pc - cgen_object));
            pc += sizeof(int32_t);
            goto again;
        case COD_PRTC:
            fprintf(dest_fp, "prtc\n");
            goto again;
        case COD_PRTI:
            fprintf(dest_fp, "prti\n");
            goto again;
        case COD_PRTS:
            fprintf(dest_fp, "prts\n");
            goto again;
        case COD_HALT:
            fprintf(dest_fp, "halt\n");
            break;
        default:
            error(0, 0, "listcode:Unknown opcode %d\n", *(pc - 1));
    }
}


static int get_enum_value(const char name[])
{
    for(size_t i = 0; i < sizeof(codegen_atr) / sizeof(codegen_atr[0]); i++)
    {
        if(strcmp(codegen_atr[i].enum_text, name) == 0)
        {
            return codegen_atr[i].node_type;
        }
    }
    error(0, 0, "Unknown token %s\n", name);
    return -1;
}

static Tree_t* load_ast(FILE* fp)
{
    int len;
    int node_type;
    char* p;
    char* tok;
    char* yytext;
    char inbuf[MAX_LINELENGTH + 1];
    Tree_t* left;
    Tree_t* right;
    //yytext = read_line(fp, &len);
    len = read_line(fp, inbuf, MAX_LINELENGTH);
    yytext = inbuf;
    if(len == 0)
    {
        fprintf(stderr, "codegen: load_ast: NULL line?\n");
    }
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
        return make_leaf(node_type, p);
    }
    left = load_ast(fp);
    right = load_ast(fp);
    return make_node(node_type, left, right);
}


int cgen_main(int argc, char* argv[])
{
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");
    code_gen(load_ast(source_fp));
    code_finish();
    list_code();
    return 0;
}

