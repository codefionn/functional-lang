# Functional lang

An example functional language + interpreter.

## Examples

```haskell
-- This is a comment

add = \x = \y = x + y
addone = add 1

addone 4 -- == 5
add ($ 2 * 2) -- == add 4, because immediate evaluation

"I'm an identifier" = add -- that ... also works (because str seqs are ids)

if .true then 1 else 0 -- == 1

fib = \x = if x == 0 then 0 else
  if x == 1 then 1 else fib (x - 1) + fib (x - 2)

hello = hello
hello -- == hello, because at evaluation, it's value doesn't change
      -- This can be used to print "Hello, World!" to console

_ == 0 -- .true, because matched with ANY
addone 2 == add 2 1 -- because both expression evaluated equal 3

.hello == .hello -- == .true, because same atoms
.hello == .nothello --  .false, because not same atoms
```

## Build

```bash
mkdir build ; cd build
cmake .. ; cmake --build .
```
