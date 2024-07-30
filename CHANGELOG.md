# Change log

## 1.4.0
- Add printing error codes for invalid commands.
- Add `Test` command.
- Add operators for strings.
- Add printing color. Allows for foreground and background. Allows for common word colors. Ex. `Print "text" in red.` or `Print "text" in red with white.`
  - Red
  - Yellow
  - Green
  - Blue
  - Magenta
  - Cyan
  - White
  - Black
- Changed `Print` and `Show` to only allow for string or variable types.
- Add `Input` command. Accepts input from the user.
- Add `Countdown` command. Waits, but shows the time remaining in real time.
- Add `Exit` command. Quits the program.
- Add `and` and `or` operators.
- Add `OUTPUT`, `RESULT`, `PASS_COUNT`, `FAIL_COUNT`, `true`, `false` constants.
- Fix various bugs.

## 1.3.1
- Update installer to ignore extra dll files.

## 1.3.0
- Added `Configure` command for editing settings.
- Remove echo. Now able to use with `Configure`.
- Change `Set mouse to` to `Move mouse to`.
- Remove `Run script from` command. Now able to run scripts directly with `Run "path.to.chisl".`.
- Scripts can be ran with `Run` directly from a string or variable.
- Change multi-line comments to `#- This format -#`.

## 1.2.0
- Implement draw rect.
- Add echo on/off.
- Fix goto if.

## 1.1.0
- Add Record, Run, Run Script, and Open.
- Fix type with delay.
- Various string bug fixes.
- Update extension.

## 1.0.0
- Initial release.