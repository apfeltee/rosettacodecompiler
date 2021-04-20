
#include "shared.h"

int analyzer_main(int argc, char* argv[]);
int astint_main(int argc, char* argv[]);
int cgen_main(int argc, char* argv[]);
int lexer_main(int argc, char* argv[]);
int vm_main(int argc, char* argv[]);

bool streq(const char* src, const char* fnd)
{
    return (strcmp(src, fnd) == 0);
}

int main(int argc, char* argv[])
{
    int c;
    const char* a;
    if(argc > 1)
    {
        a = argv[1];
        argv++;
        argc--;
        c = tolower(a[0]);
        if((c == 'l') || streq(a, "lex") || streq(a, "lexer"))
        {
            fprintf(stderr, "-- running lexer\n");
            return lexer_main(argc, argv);
        }
        else if((c == 'a') || streq(a, "analyze") || streq(a, "analyzer"))
        {
            fprintf(stderr, "-- running analyzer\n");
            return analyzer_main(argc, argv);
        }
        else if((c == 'i') || streq(a, "interp") || streq(a, "astint") || streq(a, "astinterpret"))
        {
            fprintf(stderr, "-- running astint\n");
            return astint_main(argc, argv);
        }
        else if((c == 'c') || streq(a, "cgen") || streq(a, "codegen"))
        {
            fprintf(stderr, "-- running codegen\n");
            return cgen_main(argc, argv);
        }
        else if((c == 'v') || streq(a, "vm") || streq(a, "virtualmachine"))
        {
            fprintf(stderr, "-- running virtualmachine\n");
            return vm_main(argc, argv);
        }
        else
        {
            fprintf(stderr, "unknown cmd '%s'\n", a);
        }
        return 1;
    }
    else
    {
        fprintf(stderr, "usage: %s <l(ex) | a(nalyzer) | i(nterp) | c(odegen) | v(irtualmachine) >\n", argv[0]);
        return 1;
    }
    return 0;
}

