# roll
A simple dice-percentage calculator. It can output either a histogram or roll a given dice expression.

## Building
Simply run `make` to build the `roll` executable.

## Usage
roll implements a simple language that allows for specifying dice expressions.

A dice expression is a string describing a probability function.
It consists of constants, which are either numbers or die names (i.e. d20) and operators.
Such operators include the normal arithmetical operators +,-,* and / as well as more complex operations such as "reroll the first 1" or "roll 5 times and sum the 2 highest rolls".

See `roll -h` for more information.
