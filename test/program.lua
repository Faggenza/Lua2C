-- Lua test program with various language features

-- Global variable definitions (different types)
number_int = 42
number_float = 3.14159
text = "Hello, world!"
is_active = true
empty_value = nil

-- Function definition
function calculate_factorial(n)
    if n <= 1 then
        return 1
    else
        return n * calculate_factorial(n - 1)
    end
end


-- Main program
print("Welcome to the Lua test program!")
print("Current values:")
print("number_int = ", number_int)
print("number_float = ", number_float)
print("text = ", text)

-- Using if statement
if is_active then
    print("The program is active")
else
    print("The program is not active")
end

-- Using for loop
print("\nCounting from 1 to 5:")
for i = 1, 5 do
    print(i)
end

-- Using io.read for input
print("\nEnter your name:")
user_name = io.read()
print("Hello, ", user_name)

print("\nEnter a number to calculate its factorial:")
user_number = io.read("*n")

if user_number then
    fact_result = calculate_factorial(user_number)
    print("The factorial of", user_number, "is", fact_result)
else
    print("Invalid input. Please enter a number.")
end

print("\nTest completed!")