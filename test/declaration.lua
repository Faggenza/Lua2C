-- This is a test file for the Lua parser.
--[[ Test declarations and comments. This is a comment block.]]

-- Numbers
a = 3                   -- integer
b = 4.5                 -- float

-- Strings
e = 'Single quoted string\n'
f = "Double quoted string\t"
i = ""                  -- empty string

-- Boolean 
j = true
k = false

-- Nil
l = nil

-- Tables
m = {1, 2, 3}           -- array-like table
n = {a = 1, b = 2}      -- dictionary-like table
o = {1, 2, 3, a = 4}    -- mixed table
p = {{1, 2}, {3, 4}}    -- nested tables
q = {}                  -- empty table

-- Functions
-- r = function(x, y)      -- anonymous function
--   return x + y
-- end

function s(x, y)        -- named function
  somma = "ciao"
  return x * y
end

--result = s(2,4)         -- function call


-- function u(x, y)        -- multiple return values
--   return x, y, x+y
-- end