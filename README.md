<div style="display:flex; justify-content: center; gap: 12%">
<img src="./img/lua_Logo.png" alt="Lua" width="150" />
<img src="./img/C_Logo.png" alt="C" width="138" />
</div>

# Lua2C
Transpiler written in C to convert a Lua-subset into C.
## Usage
To build:
```shell
    make all
```
or
```shell
    bison -d -v parser.y;
    flex scanner.l;
    gcc global.c translate.c symtab.c semantic.c pretty.c ast.c parser.tab.c lex.yy.c -lfl -o transpiler
```

On MacOS you may need to use -ll instead of -lfl:
```shell
    gcc global.c translate.c symtab.c semantic.c pretty.c ast.c parser.tab.c lex.yy.c -ll -o transpiler
```

To clean:
```shell
    make clean
```
To run:
```shell
    ./transpiler -options <src>
```
## Options:
```
-h  help
-t  print parse tree
-s  print symtable
```
## Test:
```shell
    make test
```
To test error:
```shell
    make error
```
## Requirements:
- Bison (version 3.8.2)
- Flex (version 2.6.4)
- GCC (version 4.8 or later)
- Make (version 4.2 or later)

