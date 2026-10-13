## flag.h

Single-header command line flag parser for C99, inspired by Python's argparse.

Copy [flag.h](./flag.h) into your project, `#include "flag.h"`, done. Nothing to
build or link. Needs a POSIX system.

### Example

```c
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
```

```
$ ./prog -h

usage: ./prog [-h] [--completions C] [-o O] [-v] INPUT

Copy INPUT to OUTPUT

options:
 --help, -h       Show this help
 --completions C  Print completions for bash, zsh or fish
 --output, -o O   where to write (default: out.txt)
 --verbose, -v    print what is going on

$ ./prog -v in.txt -o x.txt
copying in.txt to x.txt

$ source <(./prog --completions bash)
```

### Documentation

The full API is documented at the top of [flag.h](./flag.h).
