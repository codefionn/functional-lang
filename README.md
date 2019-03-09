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

fib = \x = if x == 0
           then 0
           else if x == 1
                then 1
                else fib (x - 1) + fib (x - 2)
-- Or (with named functions):
namedfib 0 = 0
namedfib 1 = 1
namedfib x = namedfib (x - 1) + namedfib (x - 2)
-- Fun corner: just type 'namedfib' and see what happens

hello = hello
hello -- == hello, because at evaluation, it's value doesn't change
      -- This can be used to print "Hello, World!" to console

_ == 0 -- .true, because matched with ANY
addone 2 == add 2 1 -- because both expression evaluated equal 3

.hello == .hello -- == .true, because same atoms
.hello == .nothello --  .false, because not same atoms

x = 0
let x = 1 in x -- == 1, because variable shadowing

-- use some pattern matching
sub = \x = if x == .zero
           then .zero
           else let .succ x = x in x
sub (.succ (.succ .zero)) -- == .succ .zero
```

## Build

Normal:

```bash
mkdir build ; cd build
cmake .. ; cmake --build .
```

Debug (some things ARE broken):

```bash
mkdir build ; cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. ; cmake --build .
```
