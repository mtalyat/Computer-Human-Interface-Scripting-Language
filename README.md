# Computer-Human Interaction Scripting Language
CHISL is a scripting language for controlling your device. It harnesses the power of computer vision to dynamically identify and interact with the elements on your screen.

## Features
- Easy, readable scripting language.
- Control the keyboard and mouse.
- Identify and use elements and text on the screen.

## Syntax

DEV NOTE:
. for end of statement
, for inside loops?

| Usage | Description |
|---|---|
| `# This is a comment.` | Comment. |
| `Capture <name>.` | Captures all of the screen and stores it in `name`. |
| `Capture <name> at <x> <y> <w> <h>.` | Captures part of the screen and stores it in `name`. |
| `Set <name> to <value>.` | Sets a variable to a value. |
| `Load <name> from <path>.` | Load a file from the path into the variable. |
| `Save <name> to <path>.` | Saves a variable to the disk. |
| `Delete <name>.` | Deletes the variable with the name. |
| `Copy <variable> to <name>.` | Copies the value of `variable` to `name`. |
| `Copy <variable> to <name> at <x> <y> <w> <h>.` | Copies the value of `variable` to `name`, and crops it.` |
| `Crop <name> to <x> <y> <w> <h>.` | Crops the image in `name` to the x y w h. |
| `Find <template> in <image>.` | Finds the `template` within `image` with 0.8 threshold. |
| `Find <template> in <image> with <threshold>.` | Finds the `template` within `image`. |
| `Clone <image>.` | Creates a clone of the `image`. |
| `Read <image>.` | Reads all of the text in `image`. |
| `Wait <milliseconds=1000>.` | Pauses for the given number of milliseconds. |
| `Pause.` | Pauses until the user hits a key. |
| `Print <value>.` | Prints the text or variable to the console. |
| `Show <value>.` | Shows the text or variable. |
| `Draw <match> on <image>.` | Draws an outline of `match` onto `image`. |
| `Draw <x> <y> <w> <h> on <image>.` | Draws a rectangle into `image`. |
| `Move mouse by <x> <y>.` | Moves the mouse relative to its current position. |
| `Set mouse to <x> <y>.` | Sets the mouse to the position. |
| `Press mouse <button=left>.` | Sends a mouse button press event to the OS. |
| `Release mouse <button=left>.` | Sends a mouse button release event to the OS. |
| `Click mouse <button=left>.` | Sends a mouse down and mouse up event to the OS. |
| `Click mouse <button=left> <times> times.` | Sends multiple mouse down and mouse up event to the OS. |
| `Scroll mouse <y> <x=0>.` | Sends a mouse scroll event to the OS. |
| `Press key <key>.` | Sends a key down event to the OS. |
| `Release key <key>.` | Sends a key up event to the OS. |
| `Type <value>.` | Sends a series of key up/downs of the phrase or variable to the OS. `delay` is set to 0.1. |
| `Type <value> with <delay> delay.` | Sends a series of key up/downs of the phrase or variable to the OS. `delay` is the wait time between each character. |
| `If <condition>:` | Runs the following code when `condition` is true. |
| `Elif <condition>:` | Runs the following code when `condition` is true. |
| `Else:` | Runs the following code when the previous condition was false. |
| `Loop:` | Loops the following code forever. |
| `Loop while <condition>:` |
| `Break.` | Breaks out of a loop. |
| `Continue.` | Returns to the beginning of the loop. |

### Example

    Capture screen.
    Load tmp from template.png.
    Set i to 0.
    Loop while i < 10:
        Print i,
        Set i to i + 1.
    Set x to (find tmp in screen).
    Draw x on screen.
    Show screen.

Loops and conditionals can be written multiple ways.

Multi-line:

    Loop:
        If x < 5:
            Set x to x * 2.
        Else:
            Set x to x - 3.
        Capture screen,
        load y from image.png,
        find y in screen,
        if x > 6:
            Break.
        .

Single line:

    Loop: If x < 5: Set x to x * 2. Else: Set x to x - 3. Capture screen, load y from image.png, find y in screen, if x > 6: Break. .






