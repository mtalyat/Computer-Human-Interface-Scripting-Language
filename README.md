# Computer-Human Interface Scripting Language
CHISL is a scripting language for controlling your device. It harnesses the power of computer vision to dynamically identify and interact with the elements on your screen.

## Features
- Easy to learn.
- Human readable scripting language.
- Control the keyboard and mouse.
- Identify and act upon elements and text on the screen.

## Syntax
| Usage | Description |
|---|---|
| `# This is a comment.` | Single line comment. |
| `#- This is a comment. -#` | Multi-line/in-line comment. |
| `Capture <var>.` | Captures all of the screen and stores it in `var`. |
| `Capture <var> at <x> <y> <width> <height>.` | Captures part of the screen and stores it in `var`. |
| `Set <var> to <value>.` | Sets a variable to a value. |
| `Count <var> from <collection>.` | Stores the size of `collection` in `var`. |
| `Get <var> from <collection> at <index>.` | Gets the value from a collection at the `index` and stores it in `var`. |
| `Load <var> from <path>.` | Load a file from the path into the variable. |
| `Save <var> to <path>.` | Saves a variable to the disk. |
| `Delete <var>.` | Deletes the variable with the var. |
| `Copy <var> to <destination>.` | Copies the value of `variable` to `var`. |
| `Crop <image> at <x> <y> <width> <height>.` | Crops the image in `var` to the x y width height. |
| `Find <var> by <template> in <image>.` | Finds the best match of `template` within `image` equal to or above the default threshold. |
| `Find <var> by <template> in <image> with <threshold>.` | Finds the best match of `template` within `image` equal to or above the given threshold. |
| `Find all <var> by <template> in <image>.` | Finds all possible matches equal to or above the default threshold. |
| `Find all <var> by <template> in <image> with <threshold>.` | Finds all possible matches equal to or above the given threshold. |
| `Find text <block/paragraph/symbol/line/word> <var> by <template> in <image>.` | Finds the best match of text `template` within `image` equal to or above the default threshold. |
| `Find text <block/paragraph/symbol/line/word> <var> by <template> in <image> with <threshold>.` | Finds the best match of text `template` within `image` equal to or above the given threshold. |
| `Find all text <block/paragraph/symbol/line/word> <var> by <template> in <image>.` | Finds all possible text matches equal to or above the default threshold. |
| `Find all text <block/paragraph/symbol/line/word> <var> by <template> in <image> with <threshold>.` | Finds all possible text matches equal to or above the given threshold. |
| `Read <var> from <image>.` | Reads all of the text in `image`. |
| `Draw <match> on <image>.` | Draws an outline of `match` onto `image`. |
| `Draw <x> <y> <w> <h> on <image>.` | Draws a rectangle into `image`. |
| `Wait <time> <ms/s/m/h>.` | Pauses for the given number of milliseconds. |
| `Pause.` | Pauses until the user hits a key. |
| `Print <value>.` | Prints the text or variable to the console. |
| `Show <value>.` | Shows the text or variable. |
| `Move mouse to <x> <y>.` | Sets the mouse to the position. |
| `Move mouse to <match>.` | Sets the mouse to the match's center position. |
| `Move mouse by <x> <y>.` | Moves the mouse relative to its current position. |
| `Press mouse <button=left>.` | Sends a mouse button press event to the OS. |
| `Release mouse <button=left>.` | Sends a mouse button release event to the OS. |
| `Click mouse <button=left>.` | Sends a mouse down and mouse up event to the OS. |
| `Click mouse <button=left> <times> times.` | Sends multiple mouse down and mouse up event to the OS. |
| `Scroll mouse <y> <x=0>.` | Sends a mouse scroll event to the OS. |
| `Press key <key>.` | Sends a key down event to the OS. |
| `Release key <key>.` | Sends a key up event to the OS. |
| `Type <value>.` | Sends a series of key up/downs of the phrase or variable to the OS with no delay. |
| `Type <value> with <delay> <ms/s/m/h> delay.` | Sends a series of key up/downs of the phrase or variable to the OS. `delay` is the wait time between each character. |
| `Label <label>.` | Creates a label that can be gone to with a goto statement. |
| `Goto <label>.` | Starts executing at the given `label`. |
| `Goto <label> if <condition>.` | Goes to the label only if the `condition` is true. |
| `Record to <path>.` | Records keyboard and mouse actions taken to a file at the given path. |
| `Run <var>.` | Runs a script loaded into a variable. |
| `Open <path>.` | Opens a file or directory. |
| `Configure <setting> to <value>.` | Turns echoing commands on or off. |

### Example
    Capture screen.
    Save screen to "screen.png".
    Load windows from "windows.png".
    Find match by windows in screen.
    Draw match on screen.
    Save screen to "found_it.png".
    Print "Found it!".
    Set mouse to match.
    Click mouse left.

## Technologies
C++, OpenCV, and Tesseract OCR.