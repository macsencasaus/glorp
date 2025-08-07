# Glorp

Glorp is a simple dynamically-typed scripting language.
The goal of this project was to combine a few features from different languages to experiment with different language constructs.

---

## Implementation
The language is interpreted and supports an interactive repl.

## The Language

```glorp
# This is a comment!

# Primitive Values

()      # the unit, similar to null/void in other languages
1       # integers
1.0     # float (double precision)
'c'     # character

# Reference Values

[1,2,3]             # dynamic list (linked list)
[1,2,3,]            # trailing commas are allowed
"abc"               # strings are just lists of characters
['a', 'b', 'c']     # is the same as above 

(x, y) -> x + y     # functions (are all lambdas)
x -> x * x
() -> 1

# Numerical Binary Expressions

x + y
x - y
x * y
x / y
x % y

# Numerical Comparison Expressions

x < y
x > y
x <= y
x >= y

# Comparison Expression

x == y  # identity comparison
TODO:

# Assignment

x = 1   # regular assignment
y :: 2  # const assignment

id :: x -> x    # identity function, for example

a, b, c = [1, 2, 3]     # lists may be unpacked
[a, b, c] = [1, 2, 3]   # this works too

x : xs = [1, 2, 3]      # prefix unpacking (x = 1, xs = [2, 3])

# Conditional Expressions

foo ? 1 : 2     # ternary expression (1 if foo is true (!= 0), 2 otherwise)

| foo => 1      # guards (1 if foo is true, 2 if bar is true, () otherwise)
| bar => 2

# Special Operators

f <| x  # Pipe operator (creates a closure with the first parameter of f set to x)

add :: (x, y) -> x + y  # For example
plus_two :: add <| 2
plus_two(1)             # returns 3

plus_three :: 3 |> add  # Forward pipe also exists

f <<< g                 # Composition operator

f :: x -> x + 2
g :: x -> 3 * x
f <<< g                 # equivalent to 3x + 2
g <<< f                 # equivalent to 3(x + 2)

f >>> g                 # forward composition also exists

# Method Operator
# The first parameter of a function can be brought to the front and used as the
# 'object' to the 'method'

map :: (list, f) -> 
    list ? { # an empty list is false
        x : xs = list;        
        [f(x)] + map(f, xs);
    } : []

[1,2,3].map(x -> x + 2)
        

# Imports

+ "another_file.glorp"  # full relative path to other file
+ "another_object.so"   # shared object may be imported to load C functions into glorp
                        # see ./examples/ffi_add for more

# Builtin Functions begin with the prefix __builtin_*

__builtin_print("Hello, World!")

```
