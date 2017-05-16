Vacuum Controller software
--------------------------

This software powers the vacuum controller, controlling when the valves are opened/closed and for how long.  It's powered by an ATtiny85, so it's written in ANSI C targetting the Atmel AVR libraries.

We use simavr to simulate the firmware so we can write it without hardware in hand, and also more easily pin down behaviors without needing to trace an entire board.

how to use
----------

You should be able to run `make clean run` from this directory and have it just work.

You may or may not need to do something like:

    brew tap osx-cross/avr
    brew install --HEAD simavr
