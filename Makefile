all:
	bison -d parser.y
	lexer scanner.l
	gcc -o parser.out parser.tab.c lex.yy.c -lfl
