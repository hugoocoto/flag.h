## flag.h

Single file flag parser for C99. Inspired by python's argparse. 

## Contributing

Check [CONTRIBUTING.md](./CONTRIBUTING.md)

## Documentation

Check [flag.h](./flag.h). 
There is also a webpage [here](https://hugocoto.com/projects/flag.h).

### Example

```c
#include "flag.h"

int
main(int argc, char **argv)
{
        char *b;
        char *foo;

        /* Optional. Set program info.*/
        flag_program(.help = "flag.h by Hugo Coto", .positionals = flag_list("pos1", "pos2"));

        /* Register flags. The first argument is a pointer that should be set to
         * the constant string with the argument. If the flag is not set, it
         * would be set to NULL. See flag_opts */
        flag_add(&foo, "--foo", "-f", .defaults = "nothing", .help = "foo flag", .nargs = 1);
        flag_add(&b, "--b", .required = 1, .help = "required flag");

        /* Check if all the required flags are set. Check if argc is at least
         * the same as the number of positionals. Return 0 if succeed. */
        if (flag_parse(&argc, &argv)) {
                /* Print help. The flags --help, -h and -help are set by default */
                flag_show_help(STDOUT_FILENO);
                exit(1);
        }

        printf("foo = %s\n", foo);
        flag_free();

        return 0;
}
```
