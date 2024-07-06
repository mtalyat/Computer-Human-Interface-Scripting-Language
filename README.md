# Computer-Human Interaction Scripting Language
CHISL is a scripting language for controlling your device. It harnesses the power of computer vision to dynamically identify and interact with the elements on your screen.

## Features
- Easy, readable scripting language.
- Control the keyboard and mouse.
- Identify and use elements and text on the screen.

## Syntax
| Usage | Description |
|---|---|
| `# This is a comment.` | Comment. |
| `Capture <name>.` | Captures all of the screen and stores it in `name`. |
| `Capture <name> at <x> <y> <w> <h>.` | Captures part of the screen and stores it in `name`. |
| `Set <name> to <value>.` | Sets a variable to a value. |
| `Get <name> from <collection> at <index>.` | Gets the value from a collection at the `index` and stores it in `name`. |
| `Load <name> from <path>.` | Load a file from the path into the variable. |
| `Save <name> to <path>.` | Saves a variable to the disk. |
| `Delete <name>.` | Deletes the variable with the name. |
| `Copy <name> to <destination>.` | Copies the value of `variable` to `name`. |
| `Crop <image> at <x> <y> <w> <h>.` | Crops the image in `name` to the x y w h. |
| `Find <name> by <template> in <image>.` | Finds the best match of `template` within `image`. |
| `Find <name> by <template> in <image> with <threshold>.` | Finds the best match of `template` within `image` equal to or above the given threshold. |
| `Find all <name> by <template> in <image> with <threshold>.` | Finds all possible matches equal to or above the given threshold. |
| `Read <name> from <image>.` | Reads all of the text in `image`. |
| `Draw <match> on <image>.` | Draws an outline of `match` onto `image`. |
| `Draw <x> <y> <w> <h> on <image>.` | Draws a rectangle into `image`. |
| `Wait <time> <ms/s/m/h>.` | Pauses for the given number of milliseconds. |
| `Pause.` | Pauses until the user hits a key. |
| `Print <value>.` | Prints the text or variable to the console. |
| `Show <value>.` | Shows the text or variable. |
| `Set mouse to <x> <y>.` | Sets the mouse to the position. |
| `Set mouse to <match>.` | Sets the mouse to the match position. |
| `Move mouse by <x> <y>.` | Moves the mouse relative to its current position. |
| `Press mouse <button=left>.` | Sends a mouse button press event to the OS. |
| `Release mouse <button=left>.` | Sends a mouse button release event to the OS. |
| `Click mouse <button=left>.` | Sends a mouse down and mouse up event to the OS. |
| `Click mouse <button=left> <times> times.` | Sends multiple mouse down and mouse up event to the OS. |
| `Scroll mouse <y> <x=0>.` | Sends a mouse scroll event to the OS. |
| `Press key <key>.` | Sends a key down event to the OS. |
| `Release key <key>.` | Sends a key up event to the OS. |
| `Type <value>.` | Sends a series of key up/downs of the phrase or variable to the OS. `delay` is set to 0.1. |
| `Type <value> with <delay> delay.` | Sends a series of key up/downs of the phrase or variable to the OS. `delay` is the wait time between each character. |
| `Label <label>.` | Creates a label that can be gone to with a goto statement. |
| `Goto <label>.` | Starts executing at the given `label`. |
| `Goto <label> if <condition>.` | Goes to the label only if the `condition` is true. |

### Example
    Capture screen.
    Save screen to screen.png.
    Load windows from windows.png.
    Find match by windows in screen.
    Draw match on screen.
    Save screen to found_it.png.
    Print "Found it!".
    Set mouse to match.
    Click mouse left.