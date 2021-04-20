#!/bin/sh

here="."

if [[ $1 ]]; then
  inputfile="$1"
  shift
  set -x
  #"$here/lexer.exe" "$inputfile" "$@" | "$here/analyzer.exe" | "$here/astint.exe"
  "$here/lexer.exe" "$inputfile" "$@" | "$here/analyzer.exe" | "$here/codegen.exe" | "$here/vmachine.exe"
else
  echo "usage: $0 <file> [<args...>]"
fi
