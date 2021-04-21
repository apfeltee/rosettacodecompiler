
#include "shared.h"

std::vector<code> vm_object;

static FILE* source_fp;
static FILE* dest_fp;

static void vm_runCode(const code* obj, int32_t* data, int g_size, char **string_pool);


static char* vm_translateString(char* st)
{
    char *p, *q;
    // skip leading " if there
    if(st[0] == '"')
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
        // skip trialing " if there
        if(q[0] == '"' && q[1] == '\0')
        {
            q++;
        }
    }
    return st;
}

/* convert an opcode string into its byte value */
static int vm_findIt(const char* text, int offset)
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

static void vm_emitByte(int c)
{

    vm_object.push_back(c);
}

static void vm_emitInteger(int32_t n)
{
    union
    {
        int32_t n;
        unsigned char c[sizeof(int32_t)];
    } x;

    x.n = n;

    for(size_t i = 0; i < sizeof(x.n); ++i)
    {
        vm_emitByte(x.c[i]);
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
static std::vector<std::string> vm_loadCode(int* ds)
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
    std::vector<std::string> string_pool;
    char inbuf[MAX_LINELENGTH + 1];
    line_len = read_line(source_fp, inbuf, MAX_LINELENGTH);
    text = inbuf;
    text = rtrim(text, &line_len);
    // skip "Datasize:"
    strtok(text, " ");
    // get actual data_size
    *ds = atoi(strtok(NULL, " "));
    // skip "Strings:"
    strtok(NULL, " ");
    // get number of strings
    n_strings = atoi(strtok(NULL, " "));
    string_pool.reserve(n_strings + 1);
    for(i = 0; i < n_strings; ++i)
    {
        line_len = read_line(source_fp, inbuf, MAX_LINELENGTH);
        text = inbuf;

        text = rtrim(text, &line_len);
        text = vm_translateString(text);
        string_pool.push_back(text);
    }
    for(;;)
    {
        line_len = read_line(source_fp, inbuf, MAX_LINELENGTH);
        text = inbuf;
        if(line_len == 0)
        {
            break;
        }
        text = rtrim(text, &line_len);
        // get the offset
        offset = atoi(strtok(text, " "));
        // get the instruction
        instr = strtok(NULL, " ");
        opcode = vm_findIt(instr, offset);
        vm_emitByte(opcode);
        operand = strtok(NULL, " ");

        switch(opcode)
        {
            case COD_JMP:
            case COD_JZ:
                {
                    // skip the '('
                    operand++;
                    len = strlen(operand);
                    // remove the ')'
                    operand[len - 1] = '\0';
                    vm_emitInteger(atoi(operand));
                }
                break;
            case COD_PUSH:
                {
                    vm_emitInteger(atoi(operand));
                }
                break;
            case COD_FETCH:
            case COD_STORE:
                {
                    // skip the '['
                    operand++;
                    len = strlen(operand);
                    // remove the ']'
                    operand[len - 1] = '\0';
                    vm_emitInteger(atoi(operand));
                }
                break;
        }
    }
    return string_pool;
}


/*** Virtual Machine interpreter ***/
static void vm_runCode(const code* obj, int32_t* data, int g_size, const std::vector<std::string>& string_pool)
{
    int32_t* sp;
    const code* pc;
    sp = &data[g_size + 1];
    pc = obj;
    fprintf(stderr, "vm_runCode: obj=%p, data=%p, g_size=%d, string_pool.size=%p\n", obj, data, g_size, int(string_pool.size()));
    if(pc == NULL)
    {
        fprintf(stderr, "vm_runCode: pc is null!\n");
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
                fprintf(dest_fp, "%c", sp[-1]);
                --sp;
                goto again;
            }
            break;
        case COD_PRTS:
            {
                fprintf(dest_fp, "%s", string_pool[sp[-1]].data());
                --sp;
                goto again;
            }
            break;
        case COD_PRTI:
            {
                fprintf(dest_fp, "%d", sp[-1]);
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

int vm_main(int argc, char* argv[])
{
    int data_size;
    int* data;
    std::vector<std::string> string_pool;
    init_io(&source_fp, stdin, "r", argc > 1 ? argv[1] : "");
    init_io(&dest_fp, stdout, "wb", argc > 2 ? argv[2] : "");
    string_pool = vm_loadCode(&data_size);
    data = new int[vm_object.size() + 1];
    vm_runCode(vm_object.data(), data, data_size, string_pool);
    delete[] data;
    return 0;
}

