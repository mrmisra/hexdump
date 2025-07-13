#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_FILE_SIZE (1024 * 1024 * 10) // 10 MB max for image mode
#define BYTES_PER_LINE 16
#define BLOCK_SIZE (64 * 1024) // Increased from 4KB to 64KB for better I/O efficiency

#define is_printable_ascii(b) ((b) >= 0x20 && (b) <= 0x7E)
#define is_whitespace(b) ((b) == ' ' || ((b) >= '\t' && (b) <= '\r'))

// Pre-computed lookup tables for better performance
static const char hex_chars[] = "0123456789abcdef";
static unsigned char rgb_table[256][3];
static int rgb_table_initialized = 0;

// Initialize RGB lookup table once
static void init_rgb_table(void) {
    if (rgb_table_initialized) return;
    
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
    rgb_table_initialized = 1;
}

// Fast hex to string conversion using lookup table
static inline void byte_to_hex(unsigned char byte, char *output) {
    output[0] = hex_chars[byte >> 4];
    output[1] = hex_chars[byte & 0x0F];
}

// Get file size more efficiently
static long get_file_size(FILE *fp) {
    if (fseek(fp, 0, SEEK_END) != 0) return -1;
    long size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) != 0) return -1;
    return size;
}

// Optimized visualization with better memory management
void visualize_file(const char *filename, long offset, long max_bytes) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open file '%s': %s\n", filename, strerror(errno));
        return;
    }
    
    long file_size = get_file_size(fp);
    if (file_size <= 0) {
        fprintf(stderr, "Cannot determine file size or file is empty\n");
        fclose(fp);
        return;
    }
    
    if (file_size > MAX_FILE_SIZE) {
        fprintf(stderr, "File '%s' too large (max %d bytes)\n", filename, MAX_FILE_SIZE);
        fclose(fp);
        return;
    }
    
    if (offset < 0 || offset >= file_size) {
        fprintf(stderr, "Offset %ld out of range (file size: %ld)\n", offset, file_size);
        fclose(fp);
        return;
    }
    
    long region = file_size - offset;
    if (max_bytes > 0 && max_bytes < region) region = max_bytes;
    
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
    
    size_t bytes_read = fread(data, 1, region, fp);
    fclose(fp);
    
    if (bytes_read == 0) {
        fprintf(stderr, "File read error or empty region\n");
        free(data);
        return;
    }
    
    int width = 256;
    int height = (int)((bytes_read + width - 1) / width);
    
    FILE *img = fopen("hexdump.ppm", "wb");
    if (!img) {
        perror("fopen output");
        free(data);
        return;
    }
    
    // Write header
    fprintf(img, "P6\n%d %d\n255\n", width, height);
    
    init_rgb_table();
    
    // Write image data more efficiently
    unsigned char *row_buffer = malloc(3 * width);
    if (!row_buffer) {
        fprintf(stderr, "Row buffer allocation failed\n");
        fclose(img);
        free(data);
        return;
    }
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = (size_t)y * width + x;
            if (idx < bytes_read) {
                memcpy(&row_buffer[3*x], rgb_table[data[idx]], 3);
            } else {
                memset(&row_buffer[3*x], 255, 3); // White for padding
            }
        }
        fwrite(row_buffer, 3, width, img);
    }
    
    free(row_buffer);
    fclose(img);
    free(data);
    printf("Visualization saved to hexdump.ppm (%d x %d pixels)\n", width, height);
}

// Optimized hexdump with better buffering and formatting
void print_hexdump(FILE *fp, FILE *out, long max_bytes) {
    long file_size = get_file_size(fp);
    if (file_size <= 0) {
        fprintf(stderr, "Cannot determine file size or file is empty\n");
        return;
    }
    
    long bytes_to_read = (max_bytes > 0 && max_bytes < file_size) ? max_bytes : file_size;
    
    // Pre-allocate output buffer for better performance
    char *output_buffer = malloc(BLOCK_SIZE * 5); // Rough estimate for output size
    if (!output_buffer) {
        fprintf(stderr, "Output buffer allocation failed\n");
        return;
    }
    
    unsigned char *block = malloc(BLOCK_SIZE);
    if (!block) {
        fprintf(stderr, "Block buffer allocation failed\n");
        free(output_buffer);
        return;
    }
    
    size_t global_offset = 0;
    long bytes_left = bytes_to_read;
    
    while (bytes_left > 0) {
        size_t to_read = (bytes_left < BLOCK_SIZE) ? bytes_left : BLOCK_SIZE;
        size_t bytes_read = fread(block, 1, to_read, fp);
        
        if (bytes_read == 0) break;
        
        // Process block in chunks of 16 bytes
        for (size_t i = 0; i < bytes_read; i += BYTES_PER_LINE) {
            size_t line_len = (bytes_read - i > BYTES_PER_LINE) ? BYTES_PER_LINE : (bytes_read - i);
            
            // Build output line in buffer first
            char line_buffer[128];
            char *ptr = line_buffer;
            
            // Format offset
            ptr += sprintf(ptr, "%08zx  ", global_offset + i);
            
            // Format hex bytes
            for (size_t j = 0; j < BYTES_PER_LINE; ++j) {
                if (j < line_len) {
                    byte_to_hex(block[i + j], ptr);
                    ptr += 2;
                    *ptr++ = ' ';
                } else {
                    *ptr++ = ' '; *ptr++ = ' '; *ptr++ = ' ';
                }
                if (j == 7) *ptr++ = ' ';
            }
            
            // Format ASCII
            *ptr++ = ' '; *ptr++ = '|';
            for (size_t j = 0; j < line_len; ++j) {
                unsigned char c = block[i + j];
                *ptr++ = is_printable_ascii(c) ? c : '.';
            }
            *ptr++ = '|'; *ptr++ = '\n'; *ptr = '\0';
            
            // Write the complete line
            fputs(line_buffer, out);
        }
        
        global_offset += bytes_read;
        bytes_left -= bytes_read;
    }
    
    free(block);
    free(output_buffer);
}

// Optimized number parsing
static long parse_number(const char *str, const char *arg_name) {
    if (!str || *str == '\0') {
        fprintf(stderr, "Invalid %s: empty string\n", arg_name);
        return -1;
    }
    
    char *endptr;
    errno = 0;
    long value = strtol(str, &endptr, 0);
    
    if (errno != 0 || *endptr != '\0' || value < 0) {
        fprintf(stderr, "Invalid %s '%s'\n", arg_name, str);
        return -1;
    }
    
    return value;
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

int main(int argc, char *argv[]) {
    int mode = 0; // 1=text, 2=save, 3=visualize
    long offset = 0;
    long max_bytes = -1;
    char *filename = NULL;

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
            if (max_bytes <= 0) {
                fprintf(stderr, "Byte count must be positive\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
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