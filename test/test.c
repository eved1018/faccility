/* file minunit_example.c */

#include "minunit.h"
#include <stdio.h>
#include <stdlib.h>
// Define TEST_BUILD before including main.c to exclude main()
#define TEST_BUILD
#include "facc.c"

int tests_run = 0;

static char* test_example(void) {
    Data data     = {0};
    Assignment* M = NULL;
    init_data(&data);
    read_problem_data("example.txt", &data);
    double total_cost = flp(&data, &M);
    print_assignment(M, data.n_facilities);

    mu_assert("error, cost != 38", (int) total_cost == 38);
    mu_assert("Facility 2 - 0 assigned to 1", M[1].clients[0] == 1);
    mu_assert("Facility 2 - 1 assigned to 2", M[1].clients[1] == 2);
    mu_assert("Facility 2 - 2 assigned to 5", M[1].clients[2] == 5);
    mu_assert("Facility 2 - 3 assigned to 7", M[1].clients[3] == 7);
    mu_assert("Facility 4 - 0 assigned to 3", M[3].clients[0] == 3);
    mu_assert("Facility 4 - 1 assigned to 4", M[3].clients[1] == 4);
    mu_assert("Facility 4 - 2 assigned to 6", M[3].clients[2] == 6);
    free_assignments(&data, M);
    free_data(&data);

    return 0;
}

static char* all_tests(void) {
    mu_run_test(test_example);
    return 0;
}

int main(void) {

    printf("=== FLP Tests ===\n\n");

    char* result = all_tests();

    printf("\n=================================\n");
    if (result != 0) {
        printf("FAILED: %s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);
    printf("=================================\n");

    return result != 0;
}
