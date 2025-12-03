#include <stdio.h>
#include "c-flags.h"

#ifndef TEST_BUILD
int main(int argc, char** argv) {

    if (argc > 0)
        c_flags_set_application_name(argv[0]);

    c_flags_set_positional_args_description("<arg1>");
    c_flags_set_description("C template");

    bool* help = c_flag_bool("help", "h", "show usage", false);
    c_flags_parse(&argc, &argv, false);

    if (*help) {
        c_flags_usage();
        return 0;
    }

    if (argc != 1) {
        c_flags_usage();
        return 1;
    }

    printf("Hello World!\n");

    return 0;
}

#endif // TEST_BUILD
