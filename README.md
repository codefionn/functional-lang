# Functional lang

An example functional language + interpreter.

## Examples

```
double = \ x = x * x
double 4

multtwo = \ x = x * 2
multtwo 4

double (double 2)

true = \t = \f = t
false = \t = \f = f
```

## Build

```bash
mkdir build ; cd build
cmake .. ; cmake --build .
```
