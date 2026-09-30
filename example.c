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
