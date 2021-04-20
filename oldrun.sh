#!/bin/zsh

# case 'l':  return lexer_main(argc, argv);
# case 'a':  return analyzer_main(argc, argv);
# case 'i':  return astint_main(argc, argv);
# case 'c':  return cgen_main(argc, argv);
# case 'v':  return vm_main(argc, argv);

function check
{
  local lastst="$1"
  local ofile="$2"
  echo "***error occured*** last status=$lastst output=\"$ofile\""
  if [[ "$lastst" != 0 ]]; then
    exit "$lastst"
  fi
  return $lastst
}


here="$PWD/a.exe"
linexe="$PWD/a.out"

# prefer linux exe if it exists; .exe is cygwin
if [[ -x "$linexe" ]]; then
  here="$linexe"
fi

inputfile="$1"

o_input="td.1.input.txt"
o_lexer="td.2.lexout.txt"
o_analyzer="td.3.anout.txt"
o_codegen="td.4.cgout.txt"
o_final="td.5.final.txt"

if [[ -f "$inputfile" ]]; then
  cat "$inputfile" > "$o_input"
  cat "$o_input" | "$here" lexer > "$o_lexer" || check "$?" "$o_lexer"
  cat "$o_lexer" | "$here" analyzer > "$o_analyzer" || check "$?" "$o_analyzer"
  cat "$o_analyzer" | "$here" codegen > "$o_codegen" || check "$?" "$o_codegen"
  cat "$o_codegen" | "$here" vmachine > "$o_final" || check "$?" "$o_final"
  cat "$o_final"
else
  echo "either no argument passed, or file does not exist"
  exit 1
fi


