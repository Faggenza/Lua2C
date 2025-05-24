-- Test file for Lua if statements

-- Basic if statement
a = 10
if a > 5 then
    s = "a is greater than 5"
end

-- If-else statement
b = 3
if b > 5 then
    s = "b is greater than 5"
else
    s = "b is not greater than 5"
end

-- If-elseif-else statement
-- local c = 5
-- if c > 10 then
--     s = "c is greater than 10"
-- elseif c == 5 then
--     s = "c is equal to 5"
-- else
--     s = "c is less than 5"
-- end

-- Nested if statements
d = 15
e = 20
if d > 10 then
    s = "d is greater than 10"
    if e > d then
        s = "e is greater than d"
    else
        s = "e is not greater than d"
    end
end

-- Different comparison operators
f = 7
if f < 10 then s = "f < 10" end
if f <= 7 then s = "f <= 7" end
if f > 5 then s = "f > 5" end
if f >= 7 then s = "f >= 7" end
if f == 7 then s = "f == 7" end
if f ~= 8 then s = "f ~= 8" end

-- Logical operators
g = true
h = false

if g and h then s = "g and h are both true" end
if g or h then s = "g or h is true" end
if not h then s = "h is not true" end

-- Compound conditions
i = 8
j = 12
if i < 10 and j > 10 then
    s = "i < 10 and j > 10"
end