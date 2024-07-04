# Computer-Human Interaction Scripting Language
CHISL is a scripting language for controlling your device. It harnesses the power of computer vision to dynamically identify and interact with the elements on your screen.

## Features
- Easy, readable scripting language.
- Control the keyboard and mouse.
- Identify and use elements and text on the screen.

DEV NOTE:
. for end of statement
, for inside loops?

## Syntax
| Usage | Description |
|---|---|
| `...this is a comment...` | Comment. |
| `Capture <name>` | Captures all of the screen and stores it in `name`. |
| `Capture <name> at <x=0> <y=0> <w=-1> <h=-1>` | Captures part of the screen and stores it in `name`. |
| `Set <name> to <value>` | Sets a variable to a value. |
| `Load <name> from <path>` | Load a file from the path into the variable. |
| `Save <name> to <path>` | Saves a variable to the disk. |
| `Delete <name>` | Deletes the variable with the name. |
| `Find <template> in <image>` | Finds the `template` within `image` with 0.8 threshold. |
| `Find <template> in <image> with <threshold>` | Finds the `template` within `image`. |
| `Clone <image>` | Creates a clone of the `image`. |
| `Read <image>` | Reads all of the text in `image`. |
| `Wait <milliseconds=1000>` | Pauses for the given number of milliseconds. |
| `Pause` | Pauses until the user hits a key. |
| `Print <value>` | Prints the text or variable to the console. |
| `Show <value>` | Shows the text or variable. |
| `Draw <template> on <image>` | Draws `template` onto `image`. |
| `Move mouse by <x> <y>` | Moves the mouse relative to its current position. |
| `Set mouse to <x> <y>` | Sets the mouse to the position. |
| `Press mouse <button=left>` | Sends a mouse button press event to the OS. |
| `Release mouse <button=left>` | Sends a mouse button release event to the OS. |
| `Click mouse <button=left>` | Sends a mouse down and mouse up event to the OS. |
| `Click mouse <button=left> <times> times` | Sends multiple mouse down and mouse up event to the OS. |
| `Scroll mouse <y> <x=0>` | Sends a mouse scroll event to the OS. |
| `Press key <key>` | Sends a key down event to the OS. |
| `Release key <key>` | Sends a key up event to the OS. |
| `Type <value>` | Sends a series of key up/downs of the phrase or variable to the OS. `delay` is the wait time between each character, set to 0.1. |
| `Type <value> with <delay>` | Sends a series of key up/downs of the phrase or variable to the OS. `delay` is the wait time between each character. |
| `If <condition>:` |
| `Elif <condition>:` |
| `Else:` |
| `Loop <condition>:` |
| `Break` |
| `Continue` |

| `snip <name> <target> <x> <y> <w> <h>` | Clips a portion of an image and stores it in `name`. |