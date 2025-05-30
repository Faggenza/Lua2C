-- Programma per calcolare la serie di Fibonacci

function fibonacci(n)
    if n <= 0 then
        return 0
    end
    if n == 1 then
        return 1
    else
        a = 0
        b = 1
        result = 0
        for i = 2, n do
            result = a + b
            a = b
            b = result
        end
        return result
    end
end

print("\nEnter your name:")
user_name = io.read()
print("Hello, ", user_name)

print("Inserisci il numero di termini della serie di Fibonacci da calcolare:")
input = io.read("*n")

if input then
    print("Serie di Fibonacci:")
    for i = 0, input - 1 do
        print("Termine", i, ":", fibonacci(i))
    end
else
    print("Per favore, inserisci un numero valido.")
end