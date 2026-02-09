# Lexical Structure

## Source Text

Zen-C source code is encoded in UTF-8.

## Grammar Notation

The lexical grammar is defined using a notation similar to EBNF.
- `Rule ::= Production`: Defines a rule.
- `[ ... ]`: Character class.
- `*`: Zero or more repetitions.
- `+`: One or more repetitions.
- `?`: Zero or one occurrence.
- `|`: Alternation.
- `"..."` or `'...'`: Literal string/character.
- `~`: Negation (e.g., `~[\n]` means any character except newline).

## Whitespace and Comments

Whitespace separates tokens but is otherwise ignored. Comments are treated as whitespace.

```text
Whitespace ::= [ \t\n\r]+
Comment    ::= LineComment | BlockComment

LineComment  ::= "//" ~[\n]*
BlockComment ::= "/*" (BlockComment | ~("*/"))* "*/"
```

## Identifiers

Identifiers name entities such as variables, functions, and types.

```text
Identifier      ::= IdentifierStart IdentifierPart*
IdentifierStart ::= [a-zA-Z_]
IdentifierPart  ::= [a-zA-Z0-9_]
```

## Literals

### Integer Literals

Integers can be decimal, hexadecimal, or binary.

```text
IntegerLiteral ::= ( DecimalInt | HexInt | BinaryInt ) IntegerSuffix?

DecimalInt ::= [0-9]+
HexInt     ::= "0x" [0-9a-fA-F]+
BinaryInt  ::= "0b" [01]+

IntegerSuffix ::= "u" | "L" | "u64" | ... 
```
*Note: The lexer technically consumes any alphanumeric sequence following a number as a suffix.*

### Floating Point Literals

```text
FloatLiteral ::= [0-9]+ "." [0-9]* FloatSuffix?
               | [0-9]+ FloatSuffix

FloatSuffix ::= "f"
```

### String Literals

```text
StringLiteral ::= '"' StringChar* '"'
StringChar    ::= ~["\\] | EscapeSequence
EscapeSequence ::= "\\" ( ["\\/bfnrt] | "u" HexDigit{4} )
```

### F-Strings

```text
FStringLiteral ::= 'f"' StringChar* '"'
```


### Character Literals

```text
CharLiteral ::= "'" ( ~['\\] | EscapeSequence ) "'"
```

## Keywords

```text
Keyword ::= Declaration | Control | Special | BoolLiteral | NullLiteral | LogicOp

Declaration ::= "let" | "def" | "fn" | "struct" | "enum" | "union" | "alias"
              | "trait" | "impl" | "use" | "module" | "import" | "opaque"

Control     ::= "if" | "else" | "match" | "for" | "while" | "loop" 
              | "return" | "break" | "continue" | "guard" | "unless" 
              | "defer" | "async" | "await" | "try" | "catch" | "goto"

Special     ::= "asm" | "assert" | "test" | "sizeof" | "embed" | "comptime" 
              | "autofree" | "volatile" | "launch" | "ref" | "static" | "const"

BoolLiteral ::= "true" | "false"
NullLiteral ::= "null"

CReserved   ::= "auto" | "case" | "char" | "default" | "do" | "double" 
              | "extern" | "float" | "inline" | "int" | "long" | "register" 
              | "restrict" | "short" | "signed" | "switch" | "typedef" 
              | "unsigned" | "void" | "_Atomic" | "_Bool" | "_Complex" 
              | "_Generic" | "_Imaginary" | "_Noreturn" 
              | "_Static_assert" | "_Thread_local"

LogicOp     ::= "and" | "or"
```

## Operators and Punctuation

```text
Operator ::= "+"  | "-"  | "*"  | "/"  | "%"
           | "&&" | "||" | "!"  | "++" | "--"
           | "&"  | "|"  | "^"  | "~"  | "<<" | ">>"
           | "==" | "!=" | "<"  | ">"  | "<=" | ">="
           | "="  | "+=" | "-=" | "*=" | "/=" | "%="
           | "&=" | "|=" | "^=" | "<<=" | ">>="
           | ".." | "..=" | "..<" | "..."
           | "."  | "?." | "??" | "??=" | "->" | "=>" 
           | "::" | "|>" | "?"
           | "("  | ")"  | "{"  | "}"  | "["  | "]"
           | ","  | ":"  | ";"  | "@"
```
