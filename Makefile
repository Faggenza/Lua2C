all:
	bison -d -v parser.y
	flex scanner.l
	gcc global.c translate.c symtab.c semantic.c pretty.c ast.c parser.tab.c lex.yy.c -lfl -o transpiler

clean:
	rm -rf parser.tab.c parser.tab.h lex.yy.c parser.output transpiler test/**/*.c test/**/*.h test/**/*.out test/**/**/*.c test/**/**/*.h test/**/**/*.out

test: clean all
	find test/*/valid -type f -name "*.lua" | while read lua_file; do \
		base_name=$$(basename $$lua_file .lua); \
		dir_name=$$(dirname $$lua_file); \
		./transpiler $$lua_file; \
		gcc $$dir_name/$$base_name.c -o $$dir_name/$$base_name.out; \
	done

error: clean all
	find test/*/error -type f -name "*.lua" | while read lua_file; do \
		base_name=$$(basename $$lua_file .lua); \
		./transpiler $$lua_file; \
	done