#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define N_CLIENTS    7
#define N_FACILITIES 5

typedef struct {
    size_t count;
    int facility;
    int clients[N_CLIENTS];
} Assignment;

typedef struct {
    int facility;
    int client;
    double cost;
} FacilityClientPair;

typedef struct {
    int facility;
    int threshold;
    size_t count;
    double cost_ratio;
    int clients[N_CLIENTS];
} CostEffectivenessMatrix;

// Compare function for qsort - from wikipedia
int compare_pairs(const void* a, const void* b) {
    FacilityClientPair* pa = (FacilityClientPair*) a;
    FacilityClientPair* pb = (FacilityClientPair*) b;
    if (pa->cost < pb->cost)
        return -1;
    if (pa->cost > pb->cost)
        return 1;
    return 0;
}

void print_cost_matrix(FacilityClientPair cost_matrix[N_CLIENTS][N_FACILITIES]) {
    for (size_t i = 0; i < N_CLIENTS; i++) {
        for (size_t j = 0; j < N_FACILITIES; j++) {
            FacilityClientPair p = cost_matrix[i][j];
            printf("c%d,%d = %.0f | ", p.client + 1, p.facility + 1, p.cost);
        }
        printf("\n");
    }
}

void print_assignment(Assignment M[N_FACILITIES]) {
    for (size_t i = 0; i < N_FACILITIES; i++) {
        Assignment a = M[i];
        if (a.count < 1) {
            continue;
        }
        printf("Facility %d: [", a.facility + 1);
        for (size_t j = 0; j < a.count; j++) {
            printf("%d ", a.clients[j] + 1);
        }
        printf("] \n");
    }
}

void make_connection_cost(int connection_costs[N_CLIENTS][N_FACILITIES]) {

    connection_costs[0][0] = 4;
    connection_costs[0][1] = 2;
    connection_costs[0][2] = 5;
    connection_costs[0][3] = 8;
    connection_costs[0][4] = 6;
    connection_costs[1][0] = 3;
    connection_costs[1][1] = 2;
    connection_costs[1][2] = 6;
    connection_costs[1][3] = 7;
    connection_costs[1][4] = 9;
    connection_costs[2][0] = 6;
    connection_costs[2][1] = 8;
    connection_costs[2][2] = 1;
    connection_costs[2][3] = 4;
    connection_costs[2][4] = 7;
    connection_costs[3][0] = 5;
    connection_costs[3][1] = 7;
    connection_costs[3][2] = 2;
    connection_costs[3][3] = 3;
    connection_costs[3][4] = 4;
    connection_costs[4][0] = 7;
    connection_costs[4][1] = 4;
    connection_costs[4][2] = 10;
    connection_costs[4][3] = 9;
    connection_costs[4][4] = 3;
    connection_costs[5][0] = 1;
    connection_costs[5][1] = 9;
    connection_costs[5][2] = 8;
    connection_costs[5][3] = 3;
    connection_costs[5][4] = 6;
    connection_costs[6][0] = 12;
    connection_costs[6][1] = 5;
    connection_costs[6][2] = 8;
    connection_costs[6][3] = 3;
    connection_costs[6][4] = 7;
}

int connection_cost(int connection_costs[N_CLIENTS][N_FACILITIES], int client, int facility) {
    return connection_costs[client][facility];
}

int opening_cost(int s) {
    switch (s) {
        case 0:
            return 6;
        case 1:
            return 10;
        case 2:
            return 12;
        case 3:
            return 5;
        case 4:
            return 8;
        default:
            return -1;
    }
}

double flp(int facilities[N_FACILITIES], int clients[N_CLIENTS], Assignment assignment[N_FACILITIES],
           int connection_costs[N_CLIENTS][N_FACILITIES]) {
    int t                = 0;
    bool U[N_CLIENTS]    = {false}; // true = unconncted, false = connected
    bool A[N_FACILITIES] = {false}; // true = opened, false = unopened

    for (size_t i = 0; i < N_CLIENTS; i++) {
        U[clients[i]] = true;
    }

    // step 1: sort opening costs of facility i in F - no idea why

    // step 2: sort the connection cost of all facility-client pairs
    FacilityClientPair cost_matrix[N_CLIENTS][N_FACILITIES] = {0};
    for (size_t i = 0; i < N_CLIENTS; i++) {
        int client = clients[i];
        for (size_t j = 0; j < N_FACILITIES; j++) {
            int facility               = facilities[j];
            cost_matrix[i][j].facility = facility;
            cost_matrix[i][j].client   = client;
            cost_matrix[i][j].cost     = connection_cost(connection_costs, client, facility);
        }
        qsort(cost_matrix[i], N_FACILITIES, sizeof(FacilityClientPair), compare_pairs);
    }
    print_cost_matrix(cost_matrix);

    // step 3.1: get the t-th ranked facility-client pair
    CostEffectivenessMatrix ce[N_FACILITIES];
    for (size_t i = 0; i < N_FACILITIES; i++) {
        ce[i].facility   = facilities[i];
        ce[i].threshold  = -1;
        ce[i].count      = 0;
        ce[i].cost_ratio = 0.0;
    }

    int n_unassigned = N_CLIENTS;
    while (n_unassigned > 0) {
        // TODO also break if t > n_facc or if ce has run out

        // step 3.1: get the t-th ranked facility-client pair
        // 3 assoc arrays to store info for each facility
        // TODO combine into one DS
        int facility_clients[N_FACILITIES][N_CLIENTS];
        double facility_costs[N_FACILITIES][N_CLIENTS];
        size_t facility_counts[N_FACILITIES] = {0};
        // get all the pairs at rank of t
        for (size_t i = 0; i < N_CLIENTS; i++) {
            int client = clients[i];
            // if c is already assigned skip it
            if (!U[client]) {
                continue;
            }
            int facility      = cost_matrix[i][t].facility;
            double cost       = cost_matrix[i][t].cost;
            size_t nth_client = facility_counts[facility];

            facility_clients[facility][nth_client] = client;
            facility_costs[facility][nth_client] += cost;
            facility_counts[facility]++; // count++
        }
        // compute cost effectiveness for each facility and update ce if smaller
        for (size_t i = 0; i < N_FACILITIES; i++) {
            int facility     = facilities[i];
            size_t n_clients = facility_counts[facility];
            if (n_clients == 0) {
                continue;
            }
            double cost_ratio = 0;
            for (size_t j = 0; j < n_clients; j++) {
                cost_ratio += facility_costs[facility][j];
            }
            if (!A[facility]) {
                cost_ratio += opening_cost(facility);
            }
            cost_ratio = cost_ratio / (int) n_clients;

            if (ce[facility].count > 0) {
                if (cost_ratio > ce[facility].cost_ratio ||
                    (cost_ratio == ce[facility].cost_ratio && n_clients < ce[facility].count)) {
                    continue;
                }
            }
            ce[facility].facility   = facility;
            ce[facility].threshold  = t;
            ce[facility].count      = n_clients;
            ce[facility].cost_ratio = cost_ratio;
            for (size_t k = 0; k < n_clients; k++) {
                ce[facility].clients[k] = facility_clients[facility][k];
            }
        }

        double best_cost         = INFINITY;
        int best_facility        = -1;
        size_t best_client_count = 0;

        for (size_t i = 0; i < N_FACILITIES; i++) {
            int facility = facilities[i];
            if (ce[facility].count < 1) {
                continue;
            }
            if (ce[facility].cost_ratio < best_cost ||
                (ce[facility].cost_ratio == best_cost && ce[facility].count > best_client_count)) {
                best_cost         = ce[facility].cost_ratio;
                best_client_count = ce[facility].count;
                best_facility     = facility;
            }
        }
        // step 3.2
        assert(best_facility != -1);
        for (size_t i = 0; i < ce[best_facility].count; i++) {
            int client = ce[best_facility].clients[i];
            U[client]  = false; // now assigned
            n_unassigned--;
        }

        if (!A[best_facility]) {
            A[best_facility] = true;
        }

        for (size_t i = 0; i < best_client_count; i++) {
            size_t count                             = assignment[best_facility].count;
            assignment[best_facility].clients[count] = ce[best_facility].clients[i];
            assignment[best_facility].count++;
        }

        ce[best_facility].count = 0; // dont use this set again
        t++;
    }

    double total_cost = 0;
    for (size_t i = 0; i < N_FACILITIES; i++) {
        Assignment a = assignment[i];
        if (a.count < 1) {
            continue;
        }
        total_cost += opening_cost(a.facility);
        for (size_t j = 0; j < a.count; j++) {
            total_cost += connection_cost(connection_costs, a.clients[j], a.facility);
        }
    }

    return total_cost;
}

int main(int argc, char** argv) {

    // Check if a filename was provided
    if (argc > 1) {
        // Read from file
        if (!read_problem_data(argv[1], &data)) {
            return 1;
        }
    } else {
        printf("No input file provided. Using hardcoded test data.\n");
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    int connection_costs[N_CLIENTS][N_FACILITIES];

    make_connection_cost(connection_costs);

    int F[N_FACILITIES] = {0, 1, 2, 3, 4};
    int C[N_CLIENTS]    = {0, 1, 2, 3, 4, 5, 6};

    Assignment M[N_FACILITIES] = {0};
    for (size_t i = 0; i < N_FACILITIES; i++) {
        M[i].count    = 0;
        M[i].facility = F[i];
    }

    double total_cost = flp(F, C, M, connection_costs);
    printf("total cost: %f\n", total_cost);

    print_assignment(M);

    return 0;
}
