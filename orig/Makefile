
CC = gcc -Wall -Wextra -g3 -ggdb3

default: all

all: lexer.exe analyzer.exe astint.exe codegen.exe vmachine.exe

lexer.exe: lexer.c
	$(CC) lexer.c -o lexer.exe

analyzer.exe: analyzer.c
	$(CC) analyzer.c -o analyzer.exe

astint.exe: astint.c
	$(CC) astint.c -o astint.exe

codegen.exe: codegen.c
	$(CC) codegen.c -o codegen.exe

vmachine.exe: vmachine.c
	$(CC) vmachine.c -o vmachine.exe

clean:
	rm -f *.o *.obj *.stackdump

distclean: clean
	rm -f *.exe

rebuild: distclean all

