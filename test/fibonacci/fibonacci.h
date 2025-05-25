#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
char* c_lua_io_read_line(){
     char *buff;
    scanf("%ms", &buff);
    return buff;
}

float c_lua_io_read_number(){
     float ret;
    scanf("%f", &ret);
    return ret;
}

char *c_lua_io_read_bytes(int n)
{
    char *buff = malloc(sizeof(char) * (n + 1));
    scanf("%ms", &buff);
    buff[n] = '\0';
    return buff;
}

typedef struct
{
    char *key;
    union value
        {
            int int_value;
            double float_value;
            char *string_value;
            bool bool_value;
        } value;
} lua_field;

int fibonacci(int n);
