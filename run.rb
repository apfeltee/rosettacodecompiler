#!/usr/bin/ruby

require "open3"

EXEFILE_WINDOWS = File.join(".", "a.exe")
EXEFILE_LINUX   = File.join(".", "a.out")

EXEFILE = (
  if File.file?(EXEFILE_LINUX) then
    EXEFILE_LINUX
  else
    EXEFILE_WINDOWS
  end
)


def runstage(stnam, infile, outfile)
  retv = nil
  indata = File.read(infile)
  Open3.popen2(EXEFILE, stnam) do| input, output, wait_thr |
    input.sync = true
    output.sync = true

    input.puts(File.read(infile))
    input.close
    retb = output.read
    $stderr.printf("received %d bytes\n", retb.bytesize)
    File.write(outfile, retb)
    output.close
    retv = wait_thr.value
  end
  return retv
end

def check(stnam, infile, outfile)
  r = runstage(stnam, infile, outfile)
  if r != 0 then
    $stderr.printf("stage %p failed with status %d: inputfile (%s) exists=%p, outputfile (%s) exists=%p\n",
      stnam, r, infile, File.file?(infile), outfile, File.file?(outfile))
    $stderr.printf("command was: %s %s < %s > %s\n", EXEFILE, stnam, infile, outfile)
    if File.file?(outfile) then
      #$stderr.printf("contents of %p:\n")
      #File.foreach()
    end
    exit(1)
  end
end

begin
  o_input = "td.1.input.txt"
  o_lexer = "td.2.lexout.txt"
  o_analyzer = "td.3.anout.txt"
  o_codegen = "td.4.cgout.txt"
  o_final = "td.5.final.txt"
  infile = ARGV.shift
  if infile != nil then
    [o_input, o_lexer, o_analyzer, o_codegen, o_final].each do |f|
        File.delete(f) rescue nil
    end
    File.write(o_input, File.read(infile))
    check("lexer", o_input, o_lexer)
    check("analyzer", o_lexer, o_analyzer)
    check("codegen", o_analyzer, o_codegen)
    check("vmachine", o_codegen, o_final)
    if File.file?(o_final) then
      $stderr.printf("final output:\n")
      puts(File.read(o_final))
    end
  else
    $stderr.printf("usage: run.rb <inputfile>\n")
    exit(1)
  end
end



