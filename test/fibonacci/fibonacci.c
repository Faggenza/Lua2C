#include "fibonacci.h"

int fibonacci(int n) {
    if (n <= 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    } else {
        int a = 0;
        int b = 1;
        int result = 0;
        for (int i = 2; i <= n; i++) {
            result = a + b;
            a = b;
            b = result;
        }
        return result;
    }
}
int main() {
    printf("Inserisci il numero di termini della serie di Fibonacci da calcolare:\n");
    float input = c_lua_io_read_number();
    if (input) {
        printf("Serie di Fibonacci:\n");
        for (int i = 0; i <= input - 1; i++) {
            printf("Termine %d : %d\n", i, fibonacci(i));
        }
    } else {
        printf("Per favore, inserisci un numero valido.\n");
    }
    return 0;
}
