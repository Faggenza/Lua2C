all:
	bison -d -v parser.y
	flex scanner.l
	gcc symtab.c semantic.c ast.c parser.tab.c lex.yy.c -lfl -o compier
