-- test_io_read_valid.lua

-- Chiamata semplice, default è "*l"
line = io.read()
print("Linea letta (default):", line)

-- Formato "*n" (numero)
num = io.read("*n")
print("Numero letto:", num)

-- Formato "*l" (linea senza newline)
line_l = io.read("*l")
print("Linea (*l):", line_l)

-- Formato "*L" (linea con newline)
line_L = io.read("*L")
print("Linea (*L):", line_L)

-- Formato "*a" (tutto il resto)
all_content = io.read("*a")
print("Tutto il contenuto rimanente:", all_content)

-- Formato numerico (numero di byte)
few_bytes = io.read(5)
print("Primi 5 byte:", few_bytes)

-- Uso in un'espressione
if io.read("*l") == "quit" then
    print("Uscita richiesta.")
end

-- Assegnazione diretta senza variabile locale (se la grammatica lo permette come statement)
-- Questo potrebbe essere un "expression statement"
io.read() -- Legge e scarta una linea

-- Un caso un po' più complesso (ipotetico)
x = io.read("*n") + 10
print("x (numero + 10):", x)

print("Fine test validi.")