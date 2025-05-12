all:
	bison -d -v parser.y
	flex scanner.l
	gcc symtab.c semantic.c pretty.c ast.c parser.tab.c lex.yy.c -lfl -o compiler

clean:
	rm -f parser.tab.c parser.tab.h lex.yy.c parser.output compiler
