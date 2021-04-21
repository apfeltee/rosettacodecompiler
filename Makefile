
exopts = -fno-optimize-sibling-calls -fno-omit-frame-pointer

srcfiles = step1.lex.cpp  step2.analyze.cpp  step3.astint.cpp  step4.codegen.cpp  step5.virtmach.cpp  dup.cpp  main.cpp
exename = backend.exe

all: runninja

runninja:
	ruby genmk.rb
	ninja -j1

normal:
	clang++ -std=c++20 -g3 -ggdb3 -Wall -Wextra $(exopts) $(srcfiles) -o $(exename)

gccsani:
	g++ -std=c++20 -g3 -ggdb3 -Wall -Wextra $(exopts) -finstrument-functions -fsanitize=address $(srcfiles) -o $(exename)

clangsani:
	clang++ -std=c++20 -g3 -ggdb3 -glldb -Wall -Wextra -Weffc++ $(exopts) -fsanitize=address $(srcfiles) -o $(exename)



