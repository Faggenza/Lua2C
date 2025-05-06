all:
	bison -d -v parser.y
	lexer scanner.l
	gcc -o parser.out parser.tab.c lex.yy.c -lfl
