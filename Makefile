
exopts = -fno-optimize-sibling-calls -fno-omit-frame-pointer

srcfiles = step1.lex.c  step2.analyze.c  step3.astint.c  step4.codegen.c  step5.virtmach.c  dup.c  main.c

all: normal

normal:
	g++ -xc++ -std=c++20 -g3 -ggdb3 -Wall -Wextra $(exopts) $(srcfiles)

gccsani:
	g++ -std=c++20 -g3 -ggdb3 -Wall -Wextra $(exopts) -finstrument-functions -fsanitize=address $(srcfiles)

clangsani:
	clang++ -std=c++20 -g3 -ggdb3 -glldb -Wall -Wextra -Weffc++ $(exopts) -fsanitize=address $(srcfiles)



