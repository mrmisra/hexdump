
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_FILE_SIZE (1024 * 1024 * 10) // 10 MB limit for visualization

// Map a byte to an RGB color (simple scheme)
void byte_to_rgb(unsigned char byte, unsigned char *r, unsigned char *g, unsigned char *b) {
    if (byte == 0x00) { // Null
        *r = 0; *g = 0; *b = 0;
    } else if (isprint(byte)) { // Printable ASCII
        *r = 0; *g = 200; *b = 0;
    } else if (isspace(byte)) { // Whitespace
        *r = 200; *g = 200; *b = 200;
    } else if (byte < 0x20 || byte == 0x7F) { // Control
        *r = 200; *g = 0; *b = 0;
    } else { // Other
        *r = 0; *g = 0; *b = 200;
    }
}

// Visualize file as a PPM image (Binvis-style)
void visualize_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }
    unsigned char *data = malloc(MAX_FILE_SIZE);
    if (!data) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        return;
    }
    size_t size = fread(data, 1, MAX_FILE_SIZE, fp);
    fclose(fp);
    if (size == 0) {
        fprintf(stderr, "File is empty or too large\n");
        free(data);
        return;
    }
    // Image dimensions: width = 256, height = ceil(size/256)
    int width = 256;
    int height = (int)((size + width - 1) / width);
    FILE *img = fopen("hexdump.ppm", "wb");
    if (!img) {
        perror("fopen output");
        free(data);
        return;
    }
    fprintf(img, "P6\n%d %d\n255\n", width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = y * width + x;
            unsigned char r = 255, g = 255, b = 255;
            if (idx < size) {
                byte_to_rgb(data[idx], &r, &g, &b);
            }
            fwrite(&r, 1, 1, img);
            fwrite(&g, 1, 1, img);
            fwrite(&b, 1, 1, img);
        }
    }
    fclose(img);
    free(data);
    printf("Visualization saved to hexdump.ppm\n");
}

#define BYTES_PER_LINE 16

void print_hexdump(FILE *fp, FILE *out, long max_bytes) {
    unsigned char buffer[BYTES_PER_LINE];
    size_t offset = 0;
    size_t n;
    long bytes_left = max_bytes > 0 ? max_bytes : -1;

    while ((n = fread(buffer, 1, BYTES_PER_LINE, fp)) > 0) {
        if (bytes_left >= 0 && n > (size_t)bytes_left) n = (size_t)bytes_left;
        fprintf(out, "%08zx  ", offset);
        for (size_t i = 0; i < BYTES_PER_LINE; ++i) {
            if (i < n)
                fprintf(out, "%02x ", buffer[i]);
            else
                fprintf(out, "   ");
            if (i == 7) fprintf(out, " ");
        }
        fprintf(out, " |");
        for (size_t i = 0; i < n; ++i)
            fprintf(out, "%c", isprint(buffer[i]) ? buffer[i] : '.');
        fprintf(out, "|\n");
        offset += n;
        if (bytes_left >= 0) {
            bytes_left -= n;
            if (bytes_left <= 0) break;
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