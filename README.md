# Functional lang

An example functional language + interpreter.

## Examples

```haskell
add = \x = \y = x + y
addone = add 1

addone 4 -- == 5

if .true then 1 else 0 -- == 1
```

## Build

```bash
mkdir build ; cd build
cmake .. ; cmake --build .
```
