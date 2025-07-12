
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_FILE_SIZE (1024 * 1024 * 10) // 10 MB limit for visualization


// Fast byte categorization using bitmasks
#define is_printable_ascii(b) ((b) >= 0x20 && (b) <= 0x7E)
#define is_whitespace(b) ((b) == ' ' || ((b) >= '\t' && (b) <= '\r'))

// Precomputed RGB LUT for visualization
static void build_rgb_table(unsigned char rgb_table[256][3]) {
    for (int i = 0; i < 256; i++) {
        if (i == 0x00) { // Null
            rgb_table[i][0] = 0; rgb_table[i][1] = 0; rgb_table[i][2] = 0;
        } else if (is_printable_ascii(i)) { // Printable ASCII
            rgb_table[i][0] = 0; rgb_table[i][1] = 200; rgb_table[i][2] = 0;
        } else if (is_whitespace(i)) { // Whitespace
            rgb_table[i][0] = 200; rgb_table[i][1] = 200; rgb_table[i][2] = 200;
        } else if (i < 0x20 || i == 0x7F) { // Control
            rgb_table[i][0] = 200; rgb_table[i][1] = 0; rgb_table[i][2] = 0;
        } else { // Other
            rgb_table[i][0] = 0; rgb_table[i][1] = 0; rgb_table[i][2] = 200;
        }
    }
}


// Visualize file as a PPM image (Binvis-style, optimized)
void visualize_file(const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        perror("stat");
        return;
    }
    if (st.st_size <= 0 || st.st_size > MAX_FILE_SIZE) {
        fprintf(stderr, "File size invalid or too large (max %d bytes)\n", MAX_FILE_SIZE);
        return;
    }
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }
    unsigned char *data = malloc(st.st_size);
    if (!data) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        return;
    }
    size_t size = fread(data, 1, st.st_size, fp);
    fclose(fp);
    if (size != (size_t)st.st_size) {
        fprintf(stderr, "File read error\n");
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
            if (idx < size) memcpy(&row[3*x], rgb_table[data[idx]], 3);
            else memset(&row[3*x], 255, 3); // White
        }
        if (fwrite(row, 3, width, img) != width) {
            fprintf(stderr, "Image row write error\n");
            break;
        }
    }
    free(row);
    fclose(img);
    free(data);
    printf("Visualization saved to hexdump.ppm\n");
}


#define BYTES_PER_LINE 16
#define BLOCK_SIZE 4096

// Optimized hexdump: block processing
void print_hexdump(FILE *fp, FILE *out, long max_bytes) {
    struct stat st;
    if (fstat(fileno(fp), &st) != 0) {
        perror("fstat");
        return;
    }
    if (st.st_size < 0) {
        fprintf(stderr, "File size invalid\n");
        return;
    }
    long file_bytes_left = st.st_size;
    if (max_bytes > 0 && max_bytes < file_bytes_left) file_bytes_left = max_bytes;
    unsigned char block[BLOCK_SIZE];
    size_t offset = 0;
    long bytes_left = file_bytes_left;
    size_t n;
    while (bytes_left > 0 && (n = fread(block, 1, (bytes_left > BLOCK_SIZE ? BLOCK_SIZE : bytes_left), fp)) > 0) {
        size_t processed = 0;
        while (processed < n) {
            size_t line_len = (n - processed > BYTES_PER_LINE) ? BYTES_PER_LINE : (n - processed);
            fprintf(out, "%08zx  ", offset);
            for (size_t i = 0; i < BYTES_PER_LINE; ++i) {
                if (i < line_len)
                    fprintf(out, "%02x ", block[processed + i]);
                else
                    fprintf(out, "   ");
                if (i == 7) fprintf(out, " ");
            }
            fprintf(out, " |");
            for (size_t i = 0; i < line_len; ++i)
                fprintf(out, "%c", is_printable_ascii(block[processed + i]) ? block[processed + i] : '.');
            fprintf(out, "|\n");
            offset += line_len;
            processed += line_len;
            if (bytes_left >= 0) {
                bytes_left -= line_len;
                if (bytes_left <= 0) return;
            }
        }
    }
}


void print_help(const char *prog) {
    printf("Usage: %s -t|-s|-v [-o offset] [-n num] <file>\n", prog);
    printf("  -t         : print hexdump to terminal\n");
    printf("  -s         : save hexdump to file.txt\n");
    printf("  -v         : visualize file as hexdump.ppm\n");
    printf("  -o offset  : start at byte offset\n");
    printf("  -n num     : read only first num bytes\n");
    printf("  -h, --help : show this help message\n");
}

int main(int argc, char *argv[]) {
    int mode = 0; // 1=text, 2=save, 3=visualize
    long offset = 0;
    long max_bytes = -1;
    char *filename = NULL;
    int show_help = 0;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-t") == 0) mode = 1;
        else if (strcmp(argv[i], "-s") == 0) mode = 2;
        else if (strcmp(argv[i], "-v") == 0) mode = 3;
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            offset = strtol(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            max_bytes = strtol(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help = 1;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }

    if (show_help) {
        print_help(argv[0]);
        return 0;
    }

    if (!filename || mode == 0) {
        print_help(argv[0]);
        return 1;
    }

    if (mode == 3) {
        // Visualization mode does not support offset or max_bytes for now
        visualize_file(filename);
        return 0;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    if (offset > 0) {
        if (fseek(fp, offset, SEEK_SET) != 0) {
            perror("fseek");
            fclose(fp);
            return 1;
        }
    }
    if (mode == 1) {
        print_hexdump(fp, stdout, max_bytes);
    } else if (mode == 2) {
        FILE *out = fopen("hexdump.txt", "w");
        if (!out) {
            perror("fopen output");
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