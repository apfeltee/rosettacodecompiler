#!/usr/bin/ruby

require "open3"

EXEFILE = File.join(__dir__, "backend.exe")


def runstage_old(stnam, infile, outfile)
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

def runstage(stnam, infile, outfile)
  insize = File.size(infile)
  # do addr checks, but discard leak checks - the backend
  # does not free anything (yet!)
  ENV["ASAN_OPTIONS"] = "detect_leaks=0"
  rt = system(EXEFILE, stnam, infile, outfile)
  ecode = $?
  if rt then
    if File.file?(outfile) then
      outsize = File.size(outfile)
      $stderr.printf("stage %p: wrote %d bytes from %d bytes input\n", stnam, outsize, insize)
    else
      $stderr.printf("stage %p: no outputfile written?\n", stnam)
    end
  end
  return [rt, ecode]
end

def check(stnam, infile, outfile)
  if File.file?(outfile) then
    File.delete(outfile)
  end
  rt, code = runstage(stnam, infile, outfile)
  if not rt then
    $stderr.printf("stage %p failed with status %d: inputfile (%s) exists=%p, outputfile (%s) exists=%p\n",
      stnam, code, infile, File.file?(infile), outfile, File.file?(outfile))
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



