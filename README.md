# Hexdump for Windows

A simple hexdump utility for Windows, written in C and compatible with Clang. It can print, save, or visualize the contents of binary files.

## Features
- Print hexdump to terminal (`-t`)
- Save hexdump to a text file (`-s`)
- Visualize file as a color-coded image (`-v`, outputs PPM format)

## Usage

```
hexdump.exe -t file.bin      # Print hexdump to terminal
hexdump.exe -s file.bin      # Save hexdump to hexdump.txt
hexdump.exe -v file.bin      # Visualize file as hexdump.ppm
```

## Building

1. Install [Clang](https://releases.llvm.org/) or use MSVC.
2. Open a command prompt in the source directory.
3. Run:
   ```
   clang hexdump.c -o hexdump.exe
   ```

## Installation

1. Copy `hexdump.exe` to a folder (e.g., `C:\Program Files\hexdump`).
2. (Optional) Add that folder to your system PATH for global access.

## Visualization
- The `-v` option creates a `hexdump.ppm` image file.
- Each byte is mapped to a color:
  - Black: Null bytes
  - Green: Printable ASCII
  - Light gray: Whitespace
  - Red: Control characters
  - Blue: Other bytes
- Open the `.ppm` file with an image viewer that supports PPM (e.g., IrfanView, GIMP, or convert to PNG).

## License
MIT License
