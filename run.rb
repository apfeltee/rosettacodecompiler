#!/usr/bin/ruby -w

require "ostruct"
require "optparse"
require "open3"

EXEFILE = File.join(__dir__, "backend.exe")

class Run
  def initialize(opts, infile)
    @opts = opts
    @infile = infile
  end

  def runstage(stnam, infile, outfile)
    insize = File.size(infile)
    if @opts.checkleaks == false then
      ENV["ASAN_OPTIONS"] = "detect_leaks=0"
    end
    rt = system(EXEFILE, stnam, infile, outfile)
    ecode = $?
    if rt then
      if File.file?(outfile) then
        outsize = File.size(outfile)
        $stderr.printf("stage %p: wrote %d bytes from %d bytes input\n", stnam, outsize, insize)
      else
        $stderr.printf("stage %p: no outputfile written?\n", stnam)
      end
    else
      if File.file?(outfile) then
        File.delete(outfile)
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
      $stderr.printf("command was: ./%s %s %p %p\n", File.basename(EXEFILE), stnam, infile, outfile)
      if File.file?(outfile) then
        #$stderr.printf("contents of %p:\n")
        #File.foreach()
      end
      exit(1)
    end
  end

  def launch()
    o_input = "td.1.input.txt"
    o_lexer = "td.2.lexout.txt"
    o_analyzer = "td.3.anout.txt"
    o_codegen = "td.4.cgout.txt"
    o_final = "td.5.final.txt"
    [o_input, o_lexer, o_analyzer, o_codegen, o_final].each do |f|
        File.delete(f) rescue nil
    end
    File.write(o_input, File.read(@infile))
    check("lexer", o_input, o_lexer)
    check("analyzer", o_lexer, o_analyzer)
    check("codegen", o_analyzer, o_codegen)
    check("vmachine", o_codegen, o_final)
    if File.file?(o_final) then
      $stderr.printf("final output:\n")
      puts(File.read(o_final))
    end
  end

end

begin
  opts = OpenStruct.new({
    checkleaks: false,
  })
  OptionParser.new{|prs|
    prs.on("-h", "--help", "show this help and exit"){
      puts(prs.help)
      exit(0)
    }
    prs.on("-l", "--leaks", "if compiled with ASAN, display leaks"){
      opts.checkleaks = true
    }
  }.parse!
  infile = ARGV.shift
  if infile != nil then
    Run.new(opts, infile).launch
  else
    $stderr.printf("usage: run.rb <inputfile>\n")
    exit(1)
  end
end



