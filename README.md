# roll
A simple dice-percentage calculator. It can output either a histogram or roll a given dice expression.

## Building
Simply run `make` to build the `roll` executable.

## Usage
roll implements a simple language that allows for specifying dice expressions.

A dice expression is a string describing a probability function.
It consists of constants, which are either numbers or die names (i.e. d20) and operators.
Such operators include the normal arithmetical operators +,-,* and / as well as more complex operations such as "reroll the first 1" or "roll 5 times and sum the 2 highest rolls".

roll can either just roll the given dice expression, or analyse its stochastic profile.
Use `roll -p` to predict the behavior of any given dice, printing a histogram, expected value, variance, etc.

See `roll -h` for more information.

## Examples
A 5e skill check with advantage (and +8 to hit):
`roll d20^2+8` would roll 2 20-sided dice, select the higher one and add 8.
You could also use `roll -p d20^2+8` to see exactly which outcomes are most likely,
or `roll -c15 d20^2+8` to see how likely that roll is to clear an AC of 15.

4d6-drop-lowest D&D character creation:
`roll -r6 d6^3/4` would roll 4 d6 and sum the 3 highest results, then repeat that 5 times.

`roll 'd10!+8+6'` A Cyberpunk Red skill check with +8 stat and +6 skill modifiers.
Note that `!` is a reserved character in bash and you'll have to quote the die string.

`roll '5x(d6>4)'` rolls a pool of 5 d6 and checks how many exceed a 4.
Again, note that `>` and `()` are reserved characters.

`roll -p '(d20~1^2+4+3)[^:2;15-*:1]x(d8+3d6)+4'`
Expected damage of a level 5 halfling rogue with advantage and +4 dex attacking with a light crossbow against a target with 15AC, assuming a hit.
