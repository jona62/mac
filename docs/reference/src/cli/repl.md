# REPL

The interactive Read-Eval-Print Loop starts when `mac` is run with no arguments.

```
$ mac
|> print "Hello, Mac!";
Hello, Mac!
|>
```

## Prompt

| Prompt | Meaning |
|--------|---------|
| `\|>` | Ready for input |
| `..` | Continuation (inside `{ }`, `[ ]`, or `( )`) |

## Multi-Line Input

Open braces, brackets, or parentheses automatically enter multi-line mode. The REPL waits for all delimiters to close before executing.

```
|> var greet = (name) -> {
..     return "Hello, " + name;
.. };
|> print greet("Mac");
Hello, Mac
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Up / Down | Navigate history |
| Left / Right | Move cursor |
| Option+Left / Right | Move by word |
| Ctrl+A | Move to start of line |
| Ctrl+E | Move to end of line |
| Ctrl+W | Delete previous word |
| Option+Delete | Delete next word |
| Ctrl+K | Delete to end of line |
| Ctrl+U | Delete entire line |
| Ctrl+L | Clear screen |
| Ctrl+C | Cancel current input |
| Ctrl+D | Exit REPL |

## History

Command history is saved to `~/.mac_history` and persists across sessions. Each line of multi-line input is stored individually for easier recall.

## Auto-Semicolons

Simple expressions without a trailing `;` or `}` get a semicolon appended automatically.

```
|> print "no semicolon needed"
no semicolon needed
```

## Pasting

Multi-line code can be pasted directly. Newlines in pasted text are processed through the same brace-tracking as typed input.

## Commands

| Command | Action |
|---------|--------|
| `exit` | Exit the REPL |
| `clear` | Clear the screen |

## Piped Input

When stdin is not a terminal, Mac reads all input as a batch and executes it.

```bash
echo 'print "hello";' | mac
# Output: hello
```
