#!/usr/bin/ruby

=begin
this script turns this:

    case something:
        blah();
        foo();
        break;

into:

    case something:
        {
            blah();
            foo();
            break;
        }
        break;

yeah, double break. some manual work required, since there's no non-hacky way to
figure out whether or not a seen "break" is the last, or not.

=end

begin
  $stderr.printf("(waiting for input from stdin...)\n")
  space = ""
  extraspace = (" " * 4)
  closed = true
  $stdin.each_line do |ln|
    ln.scrub!
    if (m = ln.match(/^(?<space>\s*)\bcase\b\s+/)) != nil then
      space = m["space"]
      if closed == false then
        printf("%s}\n", space+extraspace)
        printf("%sbreak;\n", space+extraspace)
        closed = true
      end
      printf("%s", ln)
      printf("%s{\n", space+extraspace)
      closed = false
    else
      printf("%s%s", extraspace, ln)
    end
  end
  if not closed then
    printf("%s}\n", space+extraspace)
    printf("%sbreak;\n", space+extraspace)
  end
end