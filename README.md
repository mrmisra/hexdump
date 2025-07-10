
# Hexdump for Windows

A simple hexdump utility for Windows, written in C and compatible with Clang. It can print, save, or visualize the contents of binary files, and is easy to install with a batch script.

## Features
- Print hexdump to terminal (`-t`)
- Save hexdump to a text file (`-s`)
- Visualize file as a color-coded image (`-v`, outputs PPM format)
- Start reading from a specific byte offset (`-o <offset>`)
- Limit output to the first N bytes (`-n <num>`)
- Easy installation via `install.bat` (downloads latest release and sets up PATH)

## Usage

```
hexdump.exe -t [-o offset] [-n num] file.bin      # Print hexdump to terminal
hexdump.exe -s [-o offset] [-n num] file.bin      # Save hexdump to hexdump.txt
hexdump.exe -v file.bin                          # Visualize file as hexdump.ppm
```

- `-o <offset>`: Start at byte offset (e.g., `-o 512`)
- `-n <num>`: Read only first N bytes (e.g., `-n 256`)

## Installation

### Automatic (Recommended)
1. Download `install.bat` from the [latest GitHub release](https://github.com/OWNER/REPO_NAME/releases/latest).
2. Run `install.bat` as administrator.
   - The script will:
     - Download the latest `hexdump.exe` from GitHub
     - Create `C:\Program Files\hexdump`
     - Place the exe in that folder
     - Add the folder to your system PATH

### Manual
1. Download `hexdump.exe` from the latest release.
2. Copy it to a folder (e.g., `C:\Program Files\hexdump`).
3. Add that folder to your system PATH for global access.

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
