#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <elf.h>
#if USE_ASPELL
#include <aspell.h>
#endif

/* Options */
static int opt_min_length = 3;
static _Bool opt_do_spell = false;
#if USE_ASPELL
AspellSpeller *spell;
static const _Bool use_aspell = true;
#else
static const _Bool use_aspell = false;
#endif

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

/* Returns index of the first non-whitespace char in 'data' starting at 'idx' */
#if USE_ASPELL
static size_t skip_whitespace(const unsigned char *data, size_t idx, size_t end)
{
    ssize_t i;
    for (i=idx; i<end; ++i) {
       if (!isspace(data[i]))
         break;
    }
    return (isspace(data[i]) && (i-1) >= idx) ? i-1 : i;
}
#endif

#if USE_ASPELL
static inline void print_char(char c, int n)
{
    int i;
    for (i=0; i<n; ++i)
      putc(c, stdout);
}
#endif

/* Spell check each space delimited word from st to en */
static void spell_check(unsigned char *data, size_t st, size_t en)
{
#if USE_ASPELL
    int n_errors, ok;
    size_t idx, word_st, spaces_count;

    word_st = 0;
    n_errors = 0;
    spaces_count = 0;

    /* For each word... */
    idx = skip_whitespace(data, st, en);
    for ( ; idx<en; ++idx) {
       if (isspace(data[idx]) && ((idx - word_st) > 0)) {
           size_t len = idx - word_st;
           if (!isalpha(data[idx]))
             --len;
           ok = aspell_speller_check(spell, (const char *)(data+word_st), len);
           /* Spelling error, highlight: */
           if (ok != 1) {
               int n_spaces = idx - st - spaces_count - (idx - word_st);
               ++n_errors;
               print_char('_', n_spaces);
               putc('^', stdout);
               spaces_count += n_spaces+1;
           }
           word_st = idx = skip_whitespace(data, idx, en);
       }
    }

    if (n_errors)
      printf(" [%d spelling error%s\n", n_errors, n_errors==1 ? "]" : "s]");
#endif /* USE_ASPELL */
}

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
                if (opt_do_spell)
                  spell_check(data, start, i);
            }
            start = -1;
        }
        else if (!isprint(val))
          start = -1;
        else if (start == -1 && !isspace(val)) /* Must be first printable we have seen */
          start = i;
    }
    fflush(NULL);
}

/* Parse the binary */
static void parse_elf(const char *fname)
{
    FILE *fp;
    long loc;
    size_t shdr_size;
    uint8_t *data;
    shdr_t shdr = {0};
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
    printf("Usage: %s %s [-n num] file...\n"
           "  -h: Help!\n"
#if USE_ASPELL
           "  -s: Spell check string literals\n"
#endif
           "  -n: Minimum string length to display (Default 4)\n",
           use_aspell ? "[-s]" : "",
           execname);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    int i, opt;

    while ((opt = getopt(argc, argv, "hsn:")) != -1) {
        switch (opt) {
        case 'h': usage(argv[0]); break;
#if USE_ASPELL
        case 's': opt_do_spell = true; break;
#endif
        case 'n': opt_min_length = atoi(optarg); break;
        default: usage(argv[0]); break;
        }
    }

#if USE_ASPELL
    if (opt_do_spell) {
        AspellConfig *cfg;
        AspellCanHaveError *err;
        const char *lang;
        cfg = new_aspell_config();
        lang = getenv("LANG");
        if (lang && strlen(lang))
          aspell_config_replace(cfg, "lang", lang);
        err = new_aspell_speller(cfg);
        if (aspell_error_number(err))
          ERR("aspell initilization error: %s", aspell_error_message(err));
        spell = to_aspell_speller(err);
    }
#endif /* USE_ASPELL */

    for (i=optind; i<argc; ++i) {
        if ((argc - optind) > 1)
          printf("== Parsing %s ==\n", argv[i]);
        parse_elf(argv[i]);
    }

    return 0;
}
