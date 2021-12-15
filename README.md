# IKEA Hackant

A fork of [IKEA-Hackant](https://github.com/robin7331/IKEA-Hackant).
The original solution replaced the original keypad.\
My solution enables the default IKEA controller to store and move to positions. Also the circuit is very simplified because i coded the inverter into the lin_processor and using the original keypad.

It does this by emulating a button press while maintaining the function of the keypad.
With the resistors in the circuit you can stop the moving table my pressing any key.
The resistors must have values of 1kΩ<R<3kΩ (Only tested with a value of 1,5kΩ).


## Instructions

Hold *up/down* button for moving table up/down.

Double click *up* for moving to position 1.\
Double click *down* for moving to position 2.

Trible click *up* for storing current position to position 1.\
Trible click *down* for storing current position to position 2.

While the table is moving, you can interrupt it by pressing any key

## Schematics

![Schematic](https://github.com/MasterTim17/IKEA-Hackant/raw/master/Schematic.png)

Im using SMD resistors between the analog pins for a smaller layout.

![Board](https://github.com/MasterTim17/IKEA-Hackant/raw/master/Board.jpg)
