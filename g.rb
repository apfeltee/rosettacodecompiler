
$src = <<'__eol__'
    FETCH,
    STORE,
    PUSH,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    LT,
    GT,
    LE,
    GE,
    EQ,
    NE,
    AND,
    OR,
    NEG,
    NOT,
    JMP,
    JZ,
    PRTC,
    PRTS,
    PRTI,
    HALT
__eol__


$src.split(",").map{|s| s.strip! ; printf("%s=COD_%s ", s,s ) }