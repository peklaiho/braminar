#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_text(const unsigned char* text, size_t length)
{
    putchar('"');
    for (size_t i = 0; i < length; ++i) {
        unsigned char ch = text[i];
        if (ch == '"' || ch == '\\') {
            putchar('\\');
            putchar(ch);
        } else if (ch >= 32 && ch <= 126) {
            putchar(ch);
        } else {
            printf("\\x%02X", (unsigned int)ch);
        }
    }
    putchar('"');
}

static bool extract_questions(FILE* file)
{
    unsigned char record[144];
    const size_t offsets[] = {0, 81, 102, 123};
    const size_t capacities[] = {80, 20, 20, 20};
    const char* labels[] = {"Question", "Answer 1", "Answer 2", "Answer 3"};
    size_t count = 0;
    size_t size;

    while ((size = fread(record, 1, sizeof(record), file)) != 0) {
        size_t base = count * sizeof(record);
        if (size != sizeof(record)) {
            fprintf(stderr, "Incomplete record at 0x%08zx: %zu of %zu bytes\n",
                    base, size, sizeof(record));
            return false;
        }

        for (size_t i = 0; i < 4; ++i) {
            if (record[offsets[i]] > capacities[i]) {
                fprintf(stderr, "Invalid %s length at 0x%08zx: %u exceeds %zu\n",
                        labels[i], base + offsets[i],
                        (unsigned int)record[offsets[i]], capacities[i]);
                return false;
            }
        }

        printf("Record %zu at 0x%08zx\n", count + 1, base);
        for (size_t i = 0; i < 4; ++i) {
            size_t offset = offsets[i];
            size_t length = record[offset];
            printf("  0x%08zx  %-8s (%2zu bytes): ",
                   base + offset, labels[i], length);
            print_text(record + offset + 1, length);
            putchar('\n');
        }
        putchar('\n');
        ++count;
    }

    if (ferror(file)) {
        return false;
    }
    printf("Records: %zu\n", count);
    return true;
}

static bool extract_strings(FILE* file)
{
    unsigned char prefix[4];
    size_t length = 0;
    size_t offset = 0;
    int ch;

    /* These are ASCII runs, not verified Pascal strings or code boundaries. */
    while ((ch = fgetc(file)) != EOF) {
        if (ch >= 32 && ch <= 126) {
            if (length < sizeof(prefix)) {
                prefix[length] = (unsigned char)ch;
            }
            ++length;
            if (length == sizeof(prefix)) {
                printf("0x%08zx  ", offset + 1 - length);
                fwrite(prefix, 1, sizeof(prefix), stdout);
            } else if (length > sizeof(prefix)) {
                putchar(ch);
            }
        } else {
            if (length >= sizeof(prefix)) {
                putchar('\n');
            }
            length = 0;
        }
        ++offset;
    }

    if (length >= sizeof(prefix)) {
        putchar('\n');
    }
    return !ferror(file);
}

int main(int argc, char** argv)
{
    if (argc != 3 ||
        (strcmp(argv[1], "questions") != 0 && strcmp(argv[1], "strings") != 0)) {
        fprintf(stderr, "Usage: %s questions|strings FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE* file = fopen(argv[2], "rb");
    if (file == NULL) {
        perror(argv[2]);
        return EXIT_FAILURE;
    }

    printf("File: %s\n\n", argv[2]);
    bool success = strcmp(argv[1], "questions") == 0
        ? extract_questions(file) : extract_strings(file);
    if (ferror(file)) {
        fprintf(stderr, "Error reading %s\n", argv[2]);
    }
    if (fclose(file) != 0) {
        perror(argv[2]);
        success = false;
    }
    if (fflush(stdout) == EOF || ferror(stdout)) {
        fputs("Error writing extracted data\n", stderr);
        success = false;
    }
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
