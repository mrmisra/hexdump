#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_FILE_SIZE (1024 * 1024 * 10) // 10 MB max for image mode

#define is_printable_ascii(b) ((b) >= 0x20 && (b) <= 0x7E)
#define is_whitespace(b) ((b) == ' ' || ((b) >= '\t' && (b) <= '\r'))

// Fill a 256-entry RGB lookup table for byte visualization
static void build_rgb_table(unsigned char rgb_table[256][3]) {
    for (int i = 0; i < 256; i++) {
        if (i == 0x00) {
            rgb_table[i][0] = 0; rgb_table[i][1] = 0; rgb_table[i][2] = 0;
        } else if (is_printable_ascii(i)) {
            rgb_table[i][0] = 0; rgb_table[i][1] = 200; rgb_table[i][2] = 0;
        } else if (is_whitespace(i)) {
            rgb_table[i][0] = 200; rgb_table[i][1] = 200; rgb_table[i][2] = 200;
        } else if (i < 0x20 || i == 0x7F) {
            rgb_table[i][0] = 200; rgb_table[i][1] = 0; rgb_table[i][2] = 0;
        } else {
            rgb_table[i][0] = 0; rgb_table[i][1] = 0; rgb_table[i][2] = 200;
        }
    }
}

// Write a PPM image visualizing the file's byte structure
void visualize_file(const char *filename, long offset, long max_bytes) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        perror("stat");
        return;
    }
    if (st.st_size == 0) {
        fprintf(stderr, "File '%s' is empty\n", filename);
        return;
    }
    if (st.st_size > MAX_FILE_SIZE) {
        fprintf(stderr, "File '%s' too large (max %d bytes)\n", filename, MAX_FILE_SIZE);
        return;
    }
    if (offset < 0 || offset >= st.st_size) {
        fprintf(stderr, "Offset %ld out of range (file size: %ld)\n", offset, st.st_size);
        return;
    }
    
    long region = st.st_size - offset;
    if (max_bytes > 0 && max_bytes < region) region = max_bytes;
    
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }
    if (fseek(fp, offset, SEEK_SET) != 0) {
        perror("fseek");
        fclose(fp);
        return;
    }
    
    unsigned char *data = malloc(region);
    if (!data) {
        fprintf(stderr, "Memory allocation failed for %ld bytes\n", region);
        fclose(fp);
        return;
    }
    
    size_t size = fread(data, 1, region, fp);
    fclose(fp);
    if (size == 0) {
        fprintf(stderr, "File read error or empty region\n");
        free(data);
        return;
    }
    
    int width = 256;
    int height = (int)((size + width - 1) / width);
    FILE *img = fopen("hexdump.ppm", "wb");
    if (!img) {
        perror("fopen output");
        free(data);
        return;
    }
    
    if (fprintf(img, "P6\n%d %d\n255\n", width, height) < 0) {
        fprintf(stderr, "Image header write error\n");
        fclose(img);
        free(data);
        return;
    }
    
    unsigned char rgb_table[256][3];
    build_rgb_table(rgb_table);
    
    unsigned char *row = malloc(3 * width);
    if (!row) {
        fprintf(stderr, "Row memory allocation failed\n");
        fclose(img);
        free(data);
        return;
    }
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = y * width + x;
            if (idx < size) {
                memcpy(&row[3*x], rgb_table[data[idx]], 3);
            } else {
                memset(&row[3*x], 255, 3);
            }
        }
        if (fwrite(row, 3, width, img) != width) {
            fprintf(stderr, "Image row write error at line %d\n", y);
            break;
        }
    }
    
    free(row);
    fclose(img);
    free(data);
    printf("Visualization saved to hexdump.ppm (%d x %d pixels)\n", width, height);
}

// Output hexdump in 16-byte lines, reading in 4KB blocks
#define BYTES_PER_LINE 16
#define BLOCK_SIZE 4096
void print_hexdump(FILE *fp, FILE *out, long max_bytes) {
    struct stat st;
    if (fstat(fileno(fp), &st) != 0) {
        perror("fstat");
        return;
    }
    if (st.st_size == 0) {
        fprintf(stderr, "File is empty\n");
        return;
    }
    
    long file_bytes_left = st.st_size;
    if (max_bytes > 0 && max_bytes < file_bytes_left) {
        file_bytes_left = max_bytes;
    }
    
    unsigned char block[BLOCK_SIZE];
    size_t offset = 0;
    long bytes_left = file_bytes_left;
    size_t n;
    
    while (bytes_left > 0) {
        // Calculate how much to read this iteration
        size_t to_read = (bytes_left < BLOCK_SIZE) ? bytes_left : BLOCK_SIZE;
        n = fread(block, 1, to_read, fp);
        
        if (n == 0) break; // EOF or error
        
        size_t processed = 0;
        while (processed < n) {
            size_t line_len = (n - processed > BYTES_PER_LINE) ? BYTES_PER_LINE : (n - processed);
            
            // Print offset
            fprintf(out, "%08zx  ", offset);
            
            // Print hex bytes
            for (size_t i = 0; i < BYTES_PER_LINE; ++i) {
                if (i < line_len) {
                    fprintf(out, "%02x ", block[processed + i]);
                } else {
                    fprintf(out, "   ");
                }
                if (i == 7) fprintf(out, " ");
            }
            
            // Print ASCII representation
            fprintf(out, " |");
            for (size_t i = 0; i < line_len; ++i) {
                unsigned char c = block[processed + i];
                fprintf(out, "%c", is_printable_ascii(c) ? c : '.');
            }
            fprintf(out, "|\n");
            
            offset += line_len;
            processed += line_len;
            
            // Update bytes_left only if we have a limit
            if (max_bytes > 0) {
                bytes_left -= line_len;
                if (bytes_left <= 0) return;
            }
        }
        
        // If no byte limit, update bytes_left based on what we read
        if (max_bytes <= 0) {
            bytes_left -= n;
        }
    }
}

void print_help(const char *prog) {
    printf("Usage: %s -t|-s|-v [-o offset] [-n num] <file>\n", prog);
    printf("  -t         : print hexdump to terminal\n");
    printf("  -s         : save hexdump to file.txt\n");
    printf("  -v         : visualize file as hexdump.ppm\n");
    printf("  -o offset  : start at byte offset (default: 0)\n");
    printf("  -n num     : read only first num bytes (default: entire file)\n");
    printf("  -h, --help : show this help message\n");
    printf("\nExamples:\n");
    printf("  %s -t /bin/ls              # Display hexdump of /bin/ls\n", prog);
    printf("  %s -v -o 100 -n 1000 file  # Visualize 1000 bytes starting at offset 100\n", prog);
    printf("  %s -s -n 512 data.bin      # Save first 512 bytes to hexdump.txt\n", prog);
}

// Parse a numeric argument with error checking
long parse_number(const char *str, const char *arg_name) {
    if (!str || *str == '\0') {
        fprintf(stderr, "Invalid %s: empty string\n", arg_name);
        return -1;
    }
    
    char *endptr;
    errno = 0;
    long value = strtol(str, &endptr, 0);
    
    if (errno != 0) {
        fprintf(stderr, "Invalid %s '%s': %s\n", arg_name, str, strerror(errno));
        return -1;
    }
    if (*endptr != '\0') {
        fprintf(stderr, "Invalid %s '%s': contains non-numeric characters\n", arg_name, str);
        return -1;
    }
    if (value < 0) {
        fprintf(stderr, "Invalid %s '%s': cannot be negative\n", arg_name, str);
        return -1;
    }
    
    return value;
}

int main(int argc, char *argv[]) {
    int mode = 0; // 1=text, 2=save, 3=visualize
    long offset = 0;
    long max_bytes = -1;
    char *filename = NULL;
    int show_help = 0;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-t") == 0) {
            mode = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            mode = 2;
        } else if (strcmp(argv[i], "-v") == 0) {
            mode = 3;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Option -o requires an argument\n");
                return 1;
            }
            offset = parse_number(argv[++i], "offset");
            if (offset < 0) return 1;
        } else if (strcmp(argv[i], "-n") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Option -n requires an argument\n");
                return 1;
            }
            max_bytes = parse_number(argv[++i], "byte count");
            if (max_bytes < 0) return 1;
            if (max_bytes == 0) {
                fprintf(stderr, "Byte count cannot be zero\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help = 1;
        } else if (argv[i][0] != '-') {
            if (filename) {
                fprintf(stderr, "Multiple filenames specified\n");
                return 1;
            }
            filename = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_help(argv[0]);
            return 1;
        }
    }

    if (show_help) {
        print_help(argv[0]);
        return 0;
    }

    if (!filename) {
        fprintf(stderr, "No input file specified\n");
        print_help(argv[0]);
        return 1;
    }
    
    if (mode == 0) {
        fprintf(stderr, "No mode specified (use -t, -s, or -v)\n");
        print_help(argv[0]);
        return 1;
    }

    if (mode == 3) {
        visualize_file(filename, offset, max_bytes);
        return 0;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open file '%s': %s\n", filename, strerror(errno));
        return 1;
    }
    
    if (offset > 0) {
        if (fseek(fp, offset, SEEK_SET) != 0) {
            fprintf(stderr, "Cannot seek to offset %ld: %s\n", offset, strerror(errno));
            fclose(fp);
            return 1;
        }
    }
    
    if (mode == 1) {
        print_hexdump(fp, stdout, max_bytes);
    } else if (mode == 2) {
        FILE *out = fopen("hexdump.txt", "w");
        if (!out) {
            fprintf(stderr, "Cannot create output file 'hexdump.txt': %s\n", strerror(errno));
            fclose(fp);
            return 1;
        }
        print_hexdump(fp, out, max_bytes);
        fclose(out);
        printf("Hexdump saved to hexdump.txt\n");
    }
    
    fclose(fp);
    return 0;
}