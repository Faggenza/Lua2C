-- Test per la traduzione di print e tipi di dati

-- 1. Variabili di diversi tipi
num_int = 10
num_float = 3.14
str_var = "Mondo"
bool_true_var = true
bool_false_var = false
nil_var = nil

-- 2. Chiamate a print semplici con un solo argomento
print("Ciao") -- Letterale stringa diretto
print(num_int)
print(num_float)
print(str_var)
print(bool_true_var)
print(bool_false_var)
print(nil_var)
print(nil) -- Letterale nil diretto

-- 3. Chiamata a print() senza argomenti
print()

-- 4. Chiamate a print con argomenti multipli e misti
print("Saluti da:", str_var, "Numero int:", num_int, "e float:", num_float)
print("Booleani:", bool_true_var, bool_false_var, "e un valore nil:", nil_var)
print(str_var, 123, "fine") -- Variabile, letterale int, letterale stringa

-- 5. Stampa di espressioni
print("Risultato di 5 + 3:", 5 + 3)
print("Risultato di 10 / 2.0:", 10 / 2.0)
x = 5
print("x > 3 e':", x > 3)
print("x < 3 e':", x < 3)
print("NON (x < 3) e':", not (x < 3))