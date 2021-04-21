#!/usr/bin/ruby

require "tempfile"

def do_cproto(file, tmpfile)
  # cproto uses a sort of string template to
  # modify how function prototypes are printed out. in this
  # case, the result wraps everything except the symbol in comments;
  # "int" is the placeholder for the return type,
  # "f" is the placeholder for the symbol name
  # "(a, b)" is the placeholder for the arguments.
  pattern = "/* int */ f; // (a, b);"
  return system("cproto", "-s", "-P", pattern, file, "-o", tmpfile)
end

def do_preproc(tmpfile)
  anothertmp = Tempfile.new
  begin
    anothertmp.close
    system("gcc", "-xc++", "-E", "-P", tmpfile, "-o", anothertmp.path)
    return File.read(anothertmp.path)
  ensure
    anothertmp.unlink
  end
end

def getsyms(file)
  tmpfile = Tempfile.new
  tmpfile.close
  begin
    do_cproto(file, tmpfile.path)
    data = do_preproc(tmpfile.path)
    return data.strip.split(";").map(&:strip).reject(&:empty?)
  ensure
    tmpfile.unlink
  end
end

begin
  file = ARGV.shift
  syms = getsyms(file)
  
  syms.each do |s|
    printf("%s=%s ", s, s)
  end
end

