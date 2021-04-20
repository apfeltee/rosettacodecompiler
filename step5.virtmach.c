
#include "shared.h"

da_dim(vm_object, code);


static FILE* source_fp;

static void run_vm(const code* obj, int32_t* data, int g_size, char **string_pool);


/*** Virtual Machine interpreter ***/
static void run_vm(const code* obj, int32_t* data, int g_size, char** string_pool)
{
    int32_t* sp;
    const code* pc;
    sp = &data[g_size + 1];
    pc = obj;

    if(pc == NULL)
    {
        fprintf(stderr, "run_vm: pc is null! obj=%p data=%p g_size=%d\n", obj, data, g_size);
        return;
    }
again:
    switch(*pc++)
    {
        case COD_FETCH:
            {
                *sp++ = data[*(int32_t*)pc];
                pc += sizeof(int32_t);
                goto again;
            }
            break;
        case COD_STORE:
            {
                data[*(int32_t*)pc] = *--sp;
                pc += sizeof(int32_t);
                goto again;
            }
            break;
        case COD_PUSH:
            {
                *sp++ = *(int32_t*)pc;
                pc += sizeof(int32_t);
                goto again;
            }
            break;
        case COD_ADD:
            {
                sp[-2] += sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_SUB:
            {
                sp[-2] -= sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_MUL:
            {
                sp[-2] *= sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_DIV:
            {
                sp[-2] /= sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_MOD:
            {
                sp[-2] %= sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_LT:
            {
                sp[-2] = sp[-2] < sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_GT:
            {
                sp[-2] = sp[-2] > sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_LE:
            {
                sp[-2] = sp[-2] <= sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_GE:
            {
                sp[-2] = sp[-2] >= sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_EQ:
            {
                sp[-2] = sp[-2] == sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_NE:
            {
                sp[-2] = sp[-2] != sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_AND:
            {
                sp[-2] = sp[-2] && sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_OR:
            {
                sp[-2] = sp[-2] || sp[-1];
                --sp;
                goto again;
            }
            break;
        case COD_NEG:
            {
                sp[-1] = -sp[-1];
                goto again;
            }
            break;
        case COD_NOT:
            {
                sp[-1] = !sp[-1];
                goto again;
            }
            break;
        case COD_JMP:
            {
                pc += *(int32_t*)pc;
                goto again;
            }
            break;
        case COD_JZ:
            {
                pc += (*--sp == 0) ? *(int32_t*)pc : (int32_t)sizeof(int32_t);
                goto again;
            }
            break;
        case COD_PRTC:
            {
                printf("%c", sp[-1]);
                --sp;
                goto again;
            }
            break;
        case COD_PRTS:
            {
                printf("%s", string_pool[sp[-1]]);
                --sp;
                goto again;
            }
            break;
        case COD_PRTI:
            {
                printf("%d", sp[-1]);
                --sp;
                goto again;
            }
            break;
        case COD_HALT:
            {
                break;
            }
            break;
        default:
            {
                error(0, 0, "Unknown opcode %d\n", *(pc - 1));
            }
            break;
    }
}

static char* translate(char* st)
{
    char *p, *q;
    if(st[0] == '"')// skip leading " if there
    {
        st++;
    }
    p = q = st;
    while((*p++ = *q++) != '\0')
    {
        if(q[-1] == '\\')
        {
            if(q[0] == 'n')
            {
                p[-1] = '\n';
                q++;
            }
            else if(q[0] == '\\')
            {
                q++;
            }
        }
        if(q[0] == '"' && q[1] == '\0')// skip trialing " if there
        {
            q++;
        }
    }
    return st;
}

/* convert an opcode string into its byte value */
static int findit(const char* text, int offset)
{
    size_t i;
    for(i = 0; i < sizeof(codemap_data) / sizeof(codemap_data[0]); i++)
    {
        if(strcmp(codemap_data[i].text, text) == 0)
        {
            return codemap_data[i].op;
        }
    }
    error(0, 0, "Unknown instruction %s at %d\n", text, offset);
    return -1;
}

static void emit_byte(int c)
{
    da_append(vm_object, (uchar)c, code);
}

static void emit_int(int32_t n)
{
    union
    {
        int32_t n;
        unsigned char c[sizeof(int32_t)];
    } x;

    x.n = n;

    for(size_t i = 0; i < sizeof(x.n); ++i)
    {
        emit_byte(x.c[i]);
    }
}

/*
Datasize: 5 Strings: 3
" is prime\n"
"Total primes found: "
"\n"
 154 jmp    (-73) 82
 164 jz     (32) 197
 175 push  0
 159 fetch [4]
 149 store [3]
 */

/* Load code into global array object, return the string pool and data size */
static char** load_code(int* ds)
{
    int i;
    int len;
    int line_len;
    int n_strings;
    int offset;
    int opcode;
    char* text;
    char* instr;
    char* operand;
    char** string_pool;
    char inbuf[MAX_LINELENGTH + 1];
    //text = read_line(source_fp, &line_len);
    line_len = read_line(source_fp, inbuf, MAX_LINELENGTH);
    text = inbuf;
    text = rtrim(text, &line_len);
    strtok(text, " ");// skip "Datasize:"
    *ds = atoi(strtok(NULL, " "));// get actual data_size
    strtok(NULL, " ");// skip "Strings:"
    n_strings = atoi(strtok(NULL, " "));// get number of strings
    string_pool = (char**)malloc(n_strings * sizeof(char*));
    for(i = 0; i < n_strings; ++i)
    {
        //text = read_line(source_fp, &line_len);
        line_len = read_line(source_fp, inbuf, MAX_LINELENGTH);
        text = inbuf;

        text = rtrim(text, &line_len);
        text = translate(text);
        string_pool[i] = strdup(text);
    }
    for(;;)
    {
        //text = read_line(source_fp, &line_len);
        line_len = read_line(source_fp, inbuf, MAX_LINELENGTH);
        text = inbuf;

        if(line_len == 0)
        {
            break;
        }
        fprintf(stderr, "vmachine: load_code:loop: line_len=%d, text=<<%.*s>>\n", line_len, line_len, inbuf);

        text = rtrim(text, &line_len);
        offset = atoi(strtok(text, " "));// get the offset
        instr = strtok(NULL, " ");// get the instruction
        opcode = findit(instr, offset);
        emit_byte(opcode);
        operand = strtok(NULL, " ");

        switch(opcode)
        {
            case COD_JMP:
            case COD_JZ:
                {
                    operand++;// skip the '('
                    len = strlen(operand);
                    operand[len - 1] = '\0';// remove the ')'
                    emit_int(atoi(operand));
                }
                break;
            case COD_PUSH:
                {
                    emit_int(atoi(operand));
                }
                break;
            case COD_FETCH:
            case COD_STORE:
                {
                    operand++;// skip the '['
                    len = strlen(operand);
                    operand[len - 1] = '\0';// remove the ']'
                    emit_int(atoi(operand));
                }
                break;
        }
    }
    return string_pool;
}

int vm_main(int argc, char* argv[])
{
    int data_size;
    int* data;
    char** string_pool;
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    string_pool = load_code(&data_size);
    data = (int*)malloc(1000 + data_size);
    run_vm(vm_object, data, data_size, string_pool);
    free(data);
    return 0;
}

