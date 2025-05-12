# Lua2C
Transpiler written in C to convert a Lua-subset into C.
To build:
```shell
    make all
```
or
```shell
    bison -d -v parser.y
    flex scanner.l
    gcc global.c symtab.c semantic.c pretty.c ast.c parser.tab.c lex.yy.c -lfl -o transpiler
```
To clean:
```shell
    make clean
```
To run:
```shell
    ./transpiler -options <src>
```
Options:
```shell
    -h  help
    -t  print parse tree
    -s  print symtable
```
# TODO 
**Fix 3 conflits shift/reduce**
