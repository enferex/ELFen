#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <elf.h>

/* Options */
static int opt_min_length = 3;
static _Bool opt_do_spell = false;

#define ERR(...) \
    do { \
        fprintf(stderr, "[error] "__VA_ARGS__); \
        fputc('\n', stderr); \
        exit(EXIT_FAILURE);\
    } while (0)

typedef struct _shdr_t
{
    _Bool is_64bit;
    union
    {
        Elf64_Shdr ver64;
        Elf32_Shdr ver32;
    } u;
} shdr_t;

#define SHDR(_shdr, _field) \
    ((_shdr).is_64bit ? (_shdr).u.ver64._field : (_shdr).u.ver32._field)

/* Dump the printable characters terminated by a '\0' */
static void stringz(uint8_t *data, size_t len)
{
    size_t i, s; 
    ssize_t start = -1;

    for (i=0; i<len; ++i)
    {
        const int val = data[i];
        if (val == '\0' && start != -1) {
            if ((i-start) >= opt_min_length) {
                for (s=start; s<i; ++s)
                  putc(data[s], stdout);
                putc('\n', stdout);
                fflush(NULL);
            }
            start = -1;
        }
        else if (!isprint(val))
          start = -1;
        else if (start == -1 && !isspace(val)) /* Must be first printable we have seen */
          start = i;
    }
}

/* Parse the binary */
static void parse_elf(const char *fname)
{
    FILE *fp;
    long loc;
    size_t shdr_size;
    uint8_t *data;
    shdr_t shdr;
    Elf64_Ehdr hdr;
    
    if (!(fp = fopen(fname, "r")))
      ERR("Error opening binary %s", fname);

    if (fread((void *)&hdr, 1, sizeof(hdr), fp) != sizeof(hdr))
      ERR("Error reading header from %s", fname);

    if (memcmp(hdr.e_ident, ELFMAG, SELFMAG) != 0)
      ERR("Error: Invalid ELF file: %s", fname);

    fseek(fp, hdr.e_shoff, SEEK_SET);
    if (hdr.e_ident[EI_CLASS] == ELFCLASS64) {
        shdr_size = sizeof(Elf64_Shdr);
    }
    else if (hdr.e_ident[EI_CLASS] == ELFCLASS32) {
        shdr_size = sizeof(Elf32_Shdr);
    }
    else
      ERR("Invalid binary, expected 32 or 64 bit");

    /* For each section: Look for rodata only */
    while (fread((void *)&shdr.u.ver64, 1, shdr_size, fp) == shdr_size) {

        if ((SHDR(shdr, sh_type) != SHT_PROGBITS) || (SHDR(shdr,sh_flags) != SHF_ALLOC))
          continue;

        /* Hopefully .rodata (PROGBITS and SHF_ALLOC) */
        if (!(data = malloc(SHDR(shdr,sh_size))))
          ERR("Error allocating room to store the binary's read-only data");
        loc = ftell(fp);
        fseek(fp, SHDR(shdr,sh_offset), SEEK_SET);
        if (fread(data, 1, SHDR(shdr,sh_size), fp) != SHDR(shdr,sh_size))
          ERR("Error reading section contents");

        stringz(data, SHDR(shdr,sh_size));
        fseek(fp, loc, SEEK_SET);
        free(data);
    }
}

static void usage(const char *execname)
{
    printf("Usage: %s [-s] [-n num] file...\n"
           "  -h: Help!\n"
           "  -s: Spell check string literals\n"
           "  -n: Minimum string length to display (Default 4)\n",
           execname);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    int i, opt;

    while ((opt = getopt(argc, argv, "hsn:")) != -1) {
        switch (opt) {
        case 'h': usage(argv[0]); break;
        case 's': opt_do_spell = true; break;
        case 'n': opt_min_length = atoi(optarg); break;
        default: usage(argv[0]); break;
        }
    }

    for (i=optind; i<argc; ++i) {
        printf("== Parsing %s ==\n", argv[i]);
        parse_elf(argv[i]);
    }

    return 0;
}
