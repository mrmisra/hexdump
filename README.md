
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

You can write `hexdump` or just `hd` rather then the full `hexdump.exe` in terminal after installation and it'll work fine.

```
hexdump.exe -t [-o offset] [-n num] file.bin      # Print hexdump to terminal
hexdump.exe -s [-o offset] [-n num] file.bin      # Save hexdump to hexdump.txt
hexdump.exe -v file.bin                          # Visualize file as hexdump.ppm
```

- `-o <offset>`: Start at byte offset (e.g., `-o 512`)
- `-n <num>`: Read only first N bytes (e.g., `-n 256`)

## Installation

### Windows (Automatic)
1. Download `install.bat` from the [latest GitHub release](https://github.com/mrmisra/hexdump/releases/latest).
2. Run `install.bat` as **administrator**.
   - The script will:
     - Download the latest `hexdump.exe` from GitHub
     - Create `C:\Program Files\hexdump`
     - Place the exe in that folder
     - Add the folder to your system PATH
     - Create an `hd.bat` wrapper so you can use `hd` as a shortcut

### Windows (Manual)
1. Download `hexdump.exe` from the latest release.
2. Copy it to a folder (e.g., `C:\Program Files\hexdump`).
3. Add that folder to your system PATH for global access.
4. (Optional) Create a batch file `hd.bat` in the same folder with:
   ```bat
   @echo off
   hexdump.exe %*
   ```

### Linux (Automatic)
1. Download `install.sh` from the [latest GitHub release](https://github.com/mrmisra/hexdump/releases/latest).
2. Make it executable and run it:
   ```sh
   chmod +x install.sh
   ./install.sh
   ```
   - The script will:
     - Download the latest `hd` ELF binary from GitHub
     - Place it in `/usr/local/bin` (or another directory in your PATH)
     - Make it available as `hd` (to avoid conflict with the system `hexdump`)

### Linux (Manual)
1. Download the `hd` binary from the latest release.
2. Copy it to a directory in your PATH (e.g., `/usr/local/bin`).
3. Make it executable:
   ```sh
   chmod +x /usr/local/bin/hd
   ```

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
