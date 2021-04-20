
#include "shared.h"

void error(int err_line, int err_col, const char* fmt, ...)
{
    va_list ap;
    char buf[1000];
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    fprintf(stderr, "(%d, %d) error: %s\n", err_line, err_col, buf);
    exit(1);
}



// remove trailing spaces
char* rtrim(char* text, int* len)
{
    if(text != NULL)
    {
        for(; *len > 0 && isspace((int)(text[*len - 1])); --(*len))
        {
        }
        text[*len] = '\0';
    }
    return text;
}

void init_io(FILE** fp, FILE* std, const char mode[], const char fn[])
{
    if(fn[0] == '\0')
    {
        *fp = std;
    }
    else if((*fp = fopen(fn, mode)) == NULL)
    {
        error(0, 0, "Can't open %s\n", fn);
    }
    setvbuf(*fp, NULL, _IOFBF, 0);
}

char* copystrn(const char* s, unsigned int len)
{
    #if 1
        return strndup(s, len);
    #else
        unsigned int i;
        char* rt;
        rt = (char*)malloc(len+1);
        for(i=0; i<len; i++)
        {
            rt[i] = s[i];
        }
        rt[i+1] = 0;
        return rt;
    #endif
}

char* copystr(const char* s)
{
    #if 1
        return strdup(s);
    #else
        return copystrn(s, strlen(s));
    #endif
}

int read_line(FILE* source_fp, char* dbuf, int max)
{
    int ch;
    int len;
    len = 0;
    memset(dbuf, 0, max);
    while(true)
    {
        if((ch = fgetc(source_fp)) == EOF)
        {
            break;
        }
        if(ch == '\r')
        {
            continue;
        }
        if(ch == '\n')
        {
            break;
        }
        dbuf[len] = ch;
        len++;
        if(len >= max)
        {
            break;
        }
    }
    fprintf(stderr, "read_line:len=%d, str=<<%.*s>>\n", len, len, dbuf);
    //dbuf[len] = 0;
    return len;
}

Tree_t* make_node(int node_type, Tree_t* left, Tree_t* right)
{
    Tree_t* t;
    t = (Tree_t*)calloc(sizeof(Tree_t), 1);
    t->node_type = node_type;
    t->left = left;
    t->right = right;
    return t;
}

