/* flag.h - single-header command line flag parser for C99
 *
 * Install: copy this file into your project and #include "flag.h".
 * Nothing to build or link. Needs a POSIX system.
 *
 *
 * Copyright (c) 2026 Hugo Coto Florez
 *
 * This work is licensed under the Creative Commons Attribution 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by/4.0/
 *
 * SPDX-License-Identifier: CC-BY-4.0
 *
----------------------------------- template -----------------------------------

#include "flag.h"

int
main(int argc, char **argv)
{
        const char *output;  // flag with a value: -o file
        const char *verbose; // boolean flag: -v

        flag_program(.help = "Copy INPUT to OUTPUT", .positionals = flag_list("INPUT"));
        flag_add(&output, "--output", "-o", .nargs = 1, .defaults = "out.txt", .help = "where to write");
        flag_add(&verbose, "--verbose", "-v", .help = "print what is going on");

        if (flag_parse(&argc, &argv)) {
                flag_show_help(STDERR_FILENO);
                return 1;
        }

        // Flags are removed from argv: argv[1] is now INPUT
        if (verbose) printf("copying %s to %s\n", argv[1], output);

        flag_free();
        return 0;
}

 * $ ./prog -h
 *
 * usage: ./prog [-h] [--completions C] [-o O] [-v] INPUT
 *
 * Copy INPUT to OUTPUT
 *
 * options:
 *  --help, -h       Show this help
 *  --completions C  Print completions for bash, zsh or fish
 *  --output, -o O   where to write (default: out.txt)
 *  --verbose, -v    print what is going on
 *
 * $ ./prog -v in.txt -o x.txt
 * copying in.txt to x.txt

------------------------------------- API --------------------------------------

 * Optional arguments are passed by name: flag_add(&v, "--foo", .nargs = 1).
 *
 * flag_program(...)                        Optional. Describes the program.
 *     .help        = "..."                 Text shown under the usage line.
 *     .positionals = flag_list("A", "B")   Required positional arguments.
 *     .name        = "..."                 Name in the usage line (default argv[0]).
 *
 * flag_add(&var, "--long", "-s", ...)      Register a flag (var is a const char *).
 *     .nargs    = 0 (default)              Boolean. var is non-NULL if given, else NULL.
 *     .nargs    = 1                        Takes a value (-s val, -s=val). var points to it.
 *     .defaults = "..."                    Value of var when the flag is not given.
 *     .required = 1                        Error if the flag is not given.
 *     .help     = "..."                    Text shown in the help.
 *
 * flag_parse(&argc, &argv)                 Parse the command line. Returns 0 on success,
 *                                          or non-zero after printing errors to stderr.
 *                                          Flags and their values are removed from argv,
 *                                          so argv[1..argc-1] are the positionals.
 *                                          -h, -help and --help print help and exit(0).
 *                                          --completions SHELL prints a completion
 *                                          script for bash, zsh or fish and exit(0):
 *                                            source <(prog --completions bash)
 *
 * flag_show_help(fd)                       Print usage and options to fd
 *                                          (STDOUT_FILENO or STDERR_FILENO).
 *
 * flag_write_bash_completions(fd)          Print a shell completion script to fd.
 * flag_write_zsh_completions(fd)           Values and positionals complete as files.
 * flag_write_fish_completions(fd)          Call after flag_parse (needs the program name).
 *
 * flag_basename()                          Program name without its directory
 *                                          (./bin/prog -> prog).
 *
 * flag_free()                              Free the parsed values. Don't use the flag
 *                                          variables after calling it.
 *
 * Notes:
 * - Flags can go anywhere on the command line, before or after positionals.
 * - A flag takes at most one value (nargs > 1 is not supported).
 * - Positionals are a minimum: extra arguments are left in argv.
 * - If a flag is repeated, only the first one is used; the rest stay in argv.
 * - Short flags can't be combined: use -a -b, not -ab.
 */

#ifndef FLAG_H_
#define FLAG_H_

#if _POSIX_C_SOURCE < 200809L
#define _POSIX_C_SOURCE 200809L
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FLAG_LIST_GROWTH 3

struct flag_opts {
        const char *opt;        // Flag (--help)
        const char *abbr;       // Flag abbreviation (-h)
        const char *help;       // Help message for the flag
        const char *defaults;   // Default value as string (default is a keyword)
        const char **var;       // Stores the pointer to the variable where the value should be set
        int nargs;              // Number of args to catch (max 1)
        int required;           // Set to 1 if the flag must be set
        signed char _need_free; // Asigned by flag.h
};

static struct program_opts {
        const char *name;         // program name. Used in the help message
        const char *help;         // program help. Used in the help message
        const char **positionals; // possitional arguments (check that argc -1 >= len(it))
} flag_prog = { 0 };


static struct {
        int count;
        int capacity;
        struct flag_opts *flags;
} flag_flags = { 0 };

#define flag_list(...) (const char *[]){ __VA_ARGS__, 0 }
#define flag_add(var, ...) __flag_add(var, (struct flag_opts) { __VA_ARGS__ })
#define flag_program(...) __flag_program((struct program_opts) { __VA_ARGS__ })

static inline void
__flag_list_append(struct flag_opts opts)
{
        if (flag_flags.count >= flag_flags.capacity) {
                flag_flags.capacity += FLAG_LIST_GROWTH;
                flag_flags.flags = (struct flag_opts *) realloc(
                flag_flags.flags,
                sizeof(*flag_flags.flags) * flag_flags.capacity);
        }
        flag_flags.flags[flag_flags.count++] = opts;
}

static inline void
__flag_ensure_help(void)
{
        if (flag_flags.count > 0) return;
        __flag_list_append((struct flag_opts) {
        .opt  = "--help",
        .abbr = "-h",
        .help = "Show this help",
        });
        __flag_list_append((struct flag_opts) {
        .opt   = "--completions",
        .help  = "Print completions for bash, zsh or fish",
        .nargs = 1,
        });
}

static inline int
__flag_opt_width(struct flag_opts *f)
{
        int w = 2 * f->nargs; // " X" per arg
        if (f->opt) w += strlen(f->opt);
        if (f->abbr) w += strlen(f->abbr) + 2; // ", "
        return w;
}

static inline void
flag_show_help(int fileno)
{
        int i, j, k, w, width = 0;

        __flag_ensure_help();

        dprintf(fileno, "\nusage: %s", flag_prog.name);
        if (flag_flags.count == 0) goto prog_help;

        for (i = 0; i < flag_flags.count; i++) {
                dprintf(fileno, flag_flags.flags[i].required ? " " : " [");
                if (flag_flags.flags[i].abbr) {
                        dprintf(fileno, "%s", flag_flags.flags[i].abbr);
                } else {
                        dprintf(fileno, "%s", flag_flags.flags[i].opt);
                }
                for (j = 0; j < flag_flags.flags[i].nargs; j++) {
                        for (k = 0; flag_flags.flags[i].opt[k]; k++) {
                                if (isalpha(flag_flags.flags[i].opt[k])) {
                                        dprintf(fileno, " %c", toupper(flag_flags.flags[i].opt[k]));
                                        break;
                                }
                        }
                }
                dprintf(fileno, flag_flags.flags[i].required ? "" : "]");
        }

        if (flag_prog.positionals == NULL) goto prog_help;
        for (i = 0; flag_prog.positionals[i]; i++) {
                dprintf(fileno, " %s", flag_prog.positionals[i]);
        }

prog_help:
        dprintf(fileno, "\n\n");
        if (flag_prog.help) dprintf(fileno, "%s\n\n", flag_prog.help);
        if (flag_flags.count == 0) return;

        for (i = 0; i < flag_flags.count; i++) {
                if ((w = __flag_opt_width(&flag_flags.flags[i])) > width) width = w;
        }

        dprintf(fileno, "options:\n");
        for (i = 0; i < flag_flags.count; i++) {
                dprintf(fileno, " ");
                if (flag_flags.flags[i].opt) dprintf(fileno, "%s", flag_flags.flags[i].opt);
                if (flag_flags.flags[i].abbr) dprintf(fileno, ", %s", flag_flags.flags[i].abbr);
                for (j = 0; j < flag_flags.flags[i].nargs; j++) {
                        for (k = 0; flag_flags.flags[i].opt[k]; k++) {
                                if (isalpha(flag_flags.flags[i].opt[k])) {
                                        dprintf(fileno, " %c", toupper(flag_flags.flags[i].opt[k]));
                                        break;
                                }
                        }
                }
                dprintf(fileno, "%*s", width - __flag_opt_width(&flag_flags.flags[i]) + 2, "");
                if (flag_flags.flags[i].help) dprintf(fileno, "%s", flag_flags.flags[i].help);
                if (flag_flags.flags[i].defaults) dprintf(fileno, " (default: %s)", flag_flags.flags[i].defaults);
                dprintf(fileno, "\n");
        }
        dprintf(fileno, "\n");
}

static inline const char *
flag_basename(void)
{
        const char *s     = flag_prog.name ? flag_prog.name : "";
        const char *slash = strrchr(s, '/');
        return slash ? slash + 1 : s;
}

/* Print s for use inside single quotes. Chars in `esc` get a backslash. */
static inline void
__flag_print_sq(int fileno, const char *s, const char *esc)
{
        if (!s) return;
        for (; *s; s++) {
                if (*s == '\'') {
                        dprintf(fileno, "'\\''");
                } else if (strchr(esc, *s)) {
                        dprintf(fileno, "\\%c", *s);
                } else {
                        dprintf(fileno, "%c", *s);
                }
        }
}

static inline void
flag_write_bash_completions(int fileno)
{
        const char *name = flag_basename();
        struct flag_opts *fopt;
        int i;

        __flag_ensure_help();
        dprintf(fileno, "_%s() {\n", name);
        dprintf(fileno, "        local cur=\"${COMP_WORDS[COMP_CWORD]}\" prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n");
        dprintf(fileno, "        case \"$prev\" in\n");

        for (i = 0; i < flag_flags.count; i++) {
                fopt = flag_flags.flags + i;
                if (fopt->nargs == 0) continue;
                dprintf(fileno, "        %s%s%s) return ;;\n", fopt->opt ? fopt->opt : "",
                        fopt->opt && fopt->abbr ? "|" : "", fopt->abbr ? fopt->abbr : "");
        }

        dprintf(fileno, "        esac\n");
        dprintf(fileno, "        [[ $cur == -* ]] || return\n");
        dprintf(fileno, "        COMPREPLY=($(compgen -W \"");

        for (i = 0; i < flag_flags.count; i++) {
                fopt = flag_flags.flags + i;
                if (fopt->opt) dprintf(fileno, "%s ", fopt->opt);
                if (fopt->abbr) dprintf(fileno, "%s ", fopt->abbr);
        }

        dprintf(fileno, "\" -- \"$cur\"))\n");
        dprintf(fileno, "}\n");
        dprintf(fileno, "complete -o default -F _%s %s\n", name, name);
}

static inline void
flag_write_zsh_completions(int fileno)
{
        const char *name = flag_basename();
        const char *names[2];
        struct flag_opts *fopt;
        int i, j;

        __flag_ensure_help();
        dprintf(fileno, "#compdef %s\n\n", name);
        dprintf(fileno, "_%s() {\n", name);
        dprintf(fileno, "        _arguments \\\n");

        for (i = 0; i < flag_flags.count; i++) {
                fopt     = flag_flags.flags + i;
                names[0] = fopt->opt;
                names[1] = fopt->abbr;
                for (j = 0; j < 2; j++) {
                        if (!names[j]) continue;
                        dprintf(fileno, "                '%s%s[", names[j], fopt->nargs ? "=" : "");
                        __flag_print_sq(fileno, fopt->help, "[]");
                        dprintf(fileno, "]%s' \\\n", fopt->nargs ? ":value:_files" : "");
                }
        }

        for (i = 0; flag_prog.positionals && flag_prog.positionals[i]; i++) {
                dprintf(fileno, "                '%d:", i + 1);
                __flag_print_sq(fileno, flag_prog.positionals[i], ":");
                dprintf(fileno, ":_files' \\\n");
        }

        dprintf(fileno, "                '*:file:_files'\n");
        dprintf(fileno, "}\n\n");
        dprintf(fileno, "if [ \"$funcstack[1]\" = \"_%s\" ]; then _%s \"$@\"; else compdef _%s %s; fi\n",
                name, name, name, name);
}

static inline void
flag_write_fish_completions(int fileno)
{
        const char *name = flag_basename();
        struct flag_opts *fopt;
        int i;

        __flag_ensure_help();
        for (i = 0; i < flag_flags.count; i++) {
                fopt = flag_flags.flags + i;
                dprintf(fileno, "complete -c %s", name);
                if (fopt->opt) {
                        int dd = fopt->opt[1] == '-'; // --long vs -long
                        dprintf(fileno, " %s %s", dd ? "-l" : "-o", fopt->opt + 1 + dd);
                }
                if (fopt->abbr) dprintf(fileno, " %s %s", fopt->abbr[2] ? "-o" : "-s", fopt->abbr + 1);
                if (fopt->nargs) dprintf(fileno, " -r -F");
                if (fopt->help) {
                        dprintf(fileno, " -d '");
                        __flag_print_sq(fileno, fopt->help, "\\");
                        dprintf(fileno, "'");
                }
                dprintf(fileno, "\n");
        }
}

static inline void
__flag_add(const char **var, struct flag_opts opts)
{
        __flag_ensure_help();
        if ((opts.var = var)) *opts.var = NULL;
        __flag_list_append(opts);
}

static inline void
__flag_program(struct program_opts opts)
{
        flag_prog = opts;
}

static inline void
__flag_pop_arg(int *argc, char ***argv, int *i)
{
        if (*i + 1 < *argc) {
                memmove(&(*argv)[*i], &(*argv)[*i + 1], (*argc - *i - 1) * sizeof(char *));
                --*i;
        }
        --*argc;
}

static inline void
__flag_completions(const char *shell)
{
        if (shell && !strcmp(shell, "bash"))
                flag_write_bash_completions(STDOUT_FILENO);
        else if (shell && !strcmp(shell, "zsh"))
                flag_write_zsh_completions(STDOUT_FILENO);
        else if (shell && !strcmp(shell, "fish"))
                flag_write_fish_completions(STDOUT_FILENO);
        else {
                fprintf(stderr, "Flag error: --completions expects bash, zsh or fish\n");
                exit(1);
        }
        exit(0);
}

static inline int
flag_parse(int *argc, char ***argv)
{
        struct flag_opts *fopt;
        int i, j;
        int has_error = 0;

        __flag_ensure_help();

        if (!flag_prog.name || !*flag_prog.name) flag_prog.name = **argv;

        for (i = 0; i < *argc; i++) {
                if (strcmp((*argv)[i], "--completions") == 0) {
                        __flag_completions(i + 1 < *argc ? (*argv)[i + 1] : NULL);
                }
                if (strncmp((*argv)[i], "--completions=", 14) == 0) {
                        __flag_completions((*argv)[i] + 14);
                }
                if (strcmp((*argv)[i], "-h") == 0 ||
                    strcmp((*argv)[i], "-help") == 0 ||
                    strcmp((*argv)[i], "--help") == 0) {
                        __flag_pop_arg(argc, argv, &i);
                        flag_show_help(STDOUT_FILENO);
                        exit(0);
                }
        }

        for (i = 0; i < *argc; i++) {
                for (j = 0; j < flag_flags.count; j++) {
                        fopt = flag_flags.flags + j;
                        if (!fopt->var) continue;

                        int o = fopt->opt && *fopt->opt &&
                                !strncmp(fopt->opt, (*argv)[i], strlen(fopt->opt)) &&
                                ((*argv)[i][strlen(fopt->opt)] == 0 ||
                                 (*argv)[i][strlen(fopt->opt)] == '=');
                        int a = fopt->abbr && *fopt->abbr &&
                                !strncmp(fopt->abbr, (*argv)[i], strlen(fopt->abbr)) &&
                                ((*argv)[i][strlen(fopt->abbr)] == 0 ||
                                 (*argv)[i][strlen(fopt->abbr)] == '=');

                        /* var already set or name not match */
                        if (*fopt->var || (!o && !a)) continue;

                        if (fopt->nargs > 0) {
                                if (fopt->nargs > 1) {
                                        fprintf(stderr, "Flag error: Unsupported nargs > 1\n");
                                        return 1;
                                }
                                if ((o && (*argv)[i][strlen(fopt->opt)] == '=') ||
                                    (a && (*argv)[i][strlen(fopt->abbr)] == '=')) {
                                        *fopt->var       = strdup(strchr((*argv)[i], '=') + 1);
                                        fopt->_need_free = 1;
                                } else if (*argc <= i + 1) {
                                        fprintf(stderr, "Flag error: OOB when reading value for `%s`\n", fopt->abbr ? fopt->abbr : fopt->opt);
                                        return 1;
                                } else {
                                        ++i;
                                        *fopt->var       = strdup((*argv)[i]);
                                        fopt->_need_free = 1;
                                        __flag_pop_arg(argc, argv, &i);
                                }
                        } else {
                                *fopt->var = (char *) 1;
                        }
                        __flag_pop_arg(argc, argv, &i);
                }
        }

        for (j = 0; j < flag_flags.count; j++) {
                fopt = flag_flags.flags + j;
                if (fopt->var == NULL || *fopt->var != NULL) continue;
                if (fopt->defaults) *fopt->var = fopt->defaults;
                if (fopt->required && *fopt->var == NULL) {
                        fprintf(stderr, "Flag error: Required flag %s not set!\n",
                                fopt->opt ? fopt->opt : fopt->abbr ? fopt->abbr :
                                                                     "??");
                        has_error = 1;
                }
        }

        if (flag_prog.positionals == NULL) return 0;
        for (j = 0; flag_prog.positionals[j]; j++) {
                if (j >= *argc - 1) {
                        fprintf(stderr, "Flag error: Positional argument %s not provided!\n",
                                flag_prog.positionals[j]);
                        has_error = 1;
                }
        }

        return has_error;
}

static inline void
flag_free()
{
        struct flag_opts *fopt;
        for (int j = 0; j < flag_flags.count; j++) {
                fopt = flag_flags.flags + j;
                if (fopt->var == NULL) continue;
                if (!fopt->_need_free) continue;
                free((void *) *fopt->var);
        }
        free(flag_flags.flags);
        flag_flags.flags    = NULL;
        flag_flags.count    = 0;
        flag_flags.capacity = 0;
}

#endif // !FLAG_H_
