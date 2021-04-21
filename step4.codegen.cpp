

#include "shared.h"

std::vector<code> cgen_objects;
std::vector<std::string> cgen_globals;
std::vector<std::string> cgen_string_pool;

static FILE* source_fp;
static FILE* dest_fp;
static int here = 0;


/* codegen.c */
int cgen_fetchVarOffset(const char *id);
int cgen_fetchStringOffset(const char *st);
int cgen_plugHole(void);
void cgen_fixByte(int src, int dst);
bool cgen_generateCode(Tree_t *x);
void cgen_finishCode(void);
int list_code(void);
Tree_t *cgen_loadAst(FILE *fp);
int cgen_main(int argc, char *argv[]);

Tree_t* cgen_makeLeaf(int node_type, char* value)
{
    Tree_t* t;
    t = new Tree_t;
    t->node_type = node_type;
    t->svalue = std::string(value, strlen(value));
    return t;
}


int cgen_fetchVarOffset(const std::string& id)
{
    size_t i;
    for(i = 0; i < cgen_globals.size(); ++i)
    {
        if(id == cgen_globals[i])
        {
            return i;
        }
    }
    cgen_globals.push_back(id);
    return cgen_globals.size() - 1;
}

int cgen_fetchStringOffset(const std::string& st)
{
    size_t i;
    for(i = 0; i < cgen_string_pool.size(); ++i)
    {
        if(st == cgen_string_pool[i])
        {
            return i;
        }
    }
    cgen_string_pool.push_back(st);
    return cgen_string_pool.size() - 1;
}

int cgen_typeToOp(int type)
{
    return codeattr_data[type].opcode;
}

void cgen_fixByte(int src, int dst)
{
    fprintf(stderr, "fixing: src=%d, dst=%d\n", src, dst);

    *(int32_t*)(cgen_objects.data() + src) = dst - src;
    
}

void cgen_emitByte(int c)
{
    cgen_objects.push_back(c);
    ++here;
}

void cgen_emitInt(int32_t n)
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
        cgen_emitByte(x.c[i]);
    }
}

int cgen_plugHole()
{
    int t = here;
    cgen_emitInt(0);
    return t;
}

int cgen_getEnumValue(const char* name)
{
    size_t i;
    for(i=0; codeattr_data[i].enum_text != NULL; i++)
    {
        if(strcmp(codeattr_data[i].enum_text, name) == 0)
        {
            return codeattr_data[i].node_type;
        }
    }
    error(0, 0, "Unknown token %s\n", name);
    return -1;
}

Tree_t* cgen_loadAst(FILE* fp)
{
    int len;
    int node_type;
    char* p;
    char* tok;
    char* yytext;
    char inbuf[MAX_LINELENGTH + 1];
    Tree_t* left;
    Tree_t* right;
    len = read_line(fp, inbuf, MAX_LINELENGTH);
    yytext = inbuf;
    if(len == 0)
    {
        fprintf(stderr, "codegen: cgen_loadAst: NULL line?\n");
        return NULL;
    }
    yytext = rtrim(yytext, &len);
    // get first token
    tok = strtok(yytext, " ");
    if(tok[0] == ';')
    {
        return NULL;
    }
    node_type = cgen_getEnumValue(tok);
    // if there is extra data, get it
    p = tok + strlen(tok);
    if(p != &yytext[len])
    {
        for(++p; isspace((int)(*p)); ++p)
        {
        }
        return cgen_makeLeaf(node_type, p);
    }
    left = cgen_loadAst(fp);
    right = cgen_loadAst(fp);
    return analyzer_makeNode(node_type, left, right);
}


bool cgen_generateCode(Tree_t* x)
{
    int n;
    int p1;
    int p2;
    if(x == NULL)
    {
        return 0;
    }
    switch(x->node_type)
    {
        case nd_Ident:
            {
                cgen_emitByte(COD_FETCH);
                n = cgen_fetchVarOffset(x->svalue);
                cgen_emitInt(n);
            }
            break;
        case nd_Integer:
            {
                cgen_emitByte(COD_PUSH);
                cgen_emitInt(atoi(x->svalue.c_str()));
            }
            break;
        case nd_String:
            {
                cgen_emitByte(COD_PUSH);
                n = cgen_fetchStringOffset(x->svalue);
                cgen_emitInt(n);
            }
            break;
        case nd_Assign:
            {
                n = cgen_fetchVarOffset(x->left->svalue);
                cgen_generateCode(x->right);
                cgen_emitByte(COD_STORE);
                cgen_emitInt(n);
            }
            break;
        case nd_If:
            {
                // if expr
                cgen_generateCode(x->left);
                // if false, jump
                cgen_emitByte(COD_JZ);
                // make room for jump dest
                p1 = cgen_plugHole();
                // if true statements
                cgen_generateCode(x->right->left);
                if(x->right->right != NULL)
                {
                    cgen_emitByte(COD_JMP);
                    p2 = cgen_plugHole();
                }
                cgen_fixByte(p1, here);
                if(x->right->right != NULL)
                {
                    cgen_generateCode(x->right->right);
                    cgen_fixByte(p2, here);
                }
            }
            break;
        case nd_While:
            {
                p1 = here;
                // while expr
                cgen_generateCode(x->left);
                // if false, jump
                cgen_emitByte(COD_JZ);
                // make room for jump dest
                p2 = cgen_plugHole();
                // statements
                cgen_generateCode(x->right);
                // back to the top
                cgen_emitByte(COD_JMP);
                // plug the top
                cgen_fixByte(cgen_plugHole(), p1);
                // plug the 'if false, jump'
                cgen_fixByte(p2, here);
            }
            break;
        case nd_Sequence:
            {
                cgen_generateCode(x->left);
                cgen_generateCode(x->right);
            }
            break;
        case nd_Prtc:
            {
                cgen_generateCode(x->left);
                cgen_emitByte(COD_PRTC);
            }
            break;
        case nd_Prti:
            {
                cgen_generateCode(x->left);
                cgen_emitByte(COD_PRTI);
            }
            break;
        case nd_Prts:
            {
                cgen_generateCode(x->left);
                cgen_emitByte(COD_PRTS);
            }
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
            {
                cgen_generateCode(x->left);
                cgen_generateCode(x->right);
                cgen_emitByte(cgen_typeToOp(x->node_type));
            }
            break;
        case nd_Negate:
        case nd_Not:
            {
                cgen_generateCode(x->left);
                cgen_emitByte(cgen_typeToOp(x->node_type));
            }
            break;
        default:
            {
                error(0, 0, "error in code generator - found %d, expecting operator\n", x->node_type);
                return false;
            }
            break;
    }
    return true;
}

void cgen_finishCode()
{
    cgen_emitByte(COD_HALT);
}

int cgen_printCode()
{
    size_t i;
    int32_t addrto;
    int32_t addrfrom;
    code* cgob;
    code* pc;
    std::vector<CodeInfo> rt;
    fprintf(dest_fp, "Datasize: %d Strings: %d\n", int(cgen_globals.size()), int(cgen_string_pool.size()));
    for(i = 0; i < cgen_string_pool.size(); i++)
    {
        fprintf(dest_fp, "%s\n", cgen_string_pool[i].data());
    }
    cgob = cgen_objects.data();
    pc = cgob;

again:
    fprintf(dest_fp, "%5d ", (int)(pc - cgob));
    switch(*pc++)
    {
        case COD_FETCH:
            {
                fprintf(dest_fp, "fetch [%d]\n", *(int32_t*)pc);
                pc += sizeof(int32_t);
                goto again;
            }
            break;
        case COD_STORE:
            {
                fprintf(dest_fp, "store [%d]\n", *(int32_t*)pc);
                pc += sizeof(int32_t);
                goto again;
            }
            break;
        case COD_PUSH:
            {
                fprintf(dest_fp, "push  %d\n", *(int32_t*)pc);
                pc += sizeof(int32_t);
                goto again;
            }
            break;
        case COD_ADD:
            {
                fprintf(dest_fp, "add\n");
                goto again;
            }
            break;
        case COD_SUB:
            {
                fprintf(dest_fp, "sub\n");
                goto again;
            }
            break;
        case COD_MUL:
            {
                fprintf(dest_fp, "mul\n");
                goto again;
            }
            break;
        case COD_DIV:
            {
                fprintf(dest_fp, "div\n");
                goto again;
            }
            break;
        case COD_MOD:
            {
                fprintf(dest_fp, "mod\n");
                goto again;
            }
            break;
        case COD_LT:
            {
                fprintf(dest_fp, "lt\n");
                goto again;
            }
            break;
        case COD_GT:
            {
                fprintf(dest_fp, "gt\n");
                goto again;
            }
            break;
        case COD_LE:
            {
                fprintf(dest_fp, "le\n");
                goto again;
            }
            break;
        case COD_GE:
            {
                fprintf(dest_fp, "ge\n");
                goto again;
            }
            break;
        case COD_EQ:
            {
                fprintf(dest_fp, "eq\n");
                goto again;
            }
            break;
        case COD_NE:
            {
                fprintf(dest_fp, "ne\n");
                goto again;
            }
            break;
        case COD_AND:
            {
                fprintf(dest_fp, "and\n");
                goto again;
            }
            break;
        case COD_OR:
            {
                fprintf(dest_fp, "or\n");
                goto again;
            }
            break;
        case COD_NOT:
            {
                fprintf(dest_fp, "not\n");
                goto again;
            }
            break;
        case COD_NEG:
            {
                fprintf(dest_fp, "neg\n");
                goto again;
            }
            break;
        case COD_JMP:
            {
                addrfrom = *(int32_t*)pc; 
                addrto = (int32_t)(pc + *(int32_t*)pc - cgob);
                fprintf(dest_fp, "jmp    (%d) %d\n", addrfrom, addrto);
                pc += sizeof(int32_t);
                goto again;
            }
            break;
        case COD_JZ:
            {
                addrfrom = *(int32_t*)pc;
                addrto = (int32_t)(pc + *(int32_t*)pc - cgob);
                fprintf(dest_fp, "jz     (%d) %d\n", addrfrom, addrto);
                pc += sizeof(int32_t);
                goto again;
            }
            break;
        case COD_PRTC:
            {
                fprintf(dest_fp, "prtc\n");
                goto again;
            }
            break;
        case COD_PRTI:
            {
                fprintf(dest_fp, "prti\n");
                goto again;
            }
            break;
        case COD_PRTS:
            {
                fprintf(dest_fp, "prts\n");
                goto again;
            }
            break;
        case COD_HALT:
            {
                fprintf(dest_fp, "halt\n");
                break;
            }
            break;

        default:
            {
                error(0, 0, "listcode:Unknown opcode %d\n", *(pc - 1));
                return 1;
            }
            break;
    }
    return 0;
}


int cgen_main(int argc, char* argv[])
{
    Tree_t* t;
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");
    t = cgen_loadAst(source_fp);
    if(t != NULL)
    {
        if(cgen_generateCode(t))
        {
            cgen_finishCode();
            return cgen_printCode();
        }
        else
        {
            fprintf(stderr, "codegen: failed to generate code\n");
        }
    }
    else
    {
        fprintf(stderr, "codegen: failed to load ast\n");
    }
    return 1;
}

