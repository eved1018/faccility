#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define BUFFER_SIZE 1024

typedef struct {
    int facility;
    int client;
} pair_t;

typedef struct {
    pair_t key;
    double value;
} ConnectionCost;

typedef struct {
    int key; // facility
    double value;
} OpeningCost;

typedef struct {
    int key;    // client
    bool value; // true = unconnected, false = connected
} UsedClients;

typedef struct {
    size_t count;
    int facility;
    int* clients; // dynamic array
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
    int* clients; // dynamic array
} CostEffectivenessMatrix;

typedef struct {
    size_t n_facilities;
    int* facilities;
    OpeningCost* opening_costs;
    int* clients;
    size_t n_clients;
    ConnectionCost* connection_costs;
} Data;

// Compare function for qsort
int compare_pairs(const void* a, const void* b) {
    FacilityClientPair* pa = (FacilityClientPair*) a;
    FacilityClientPair* pb = (FacilityClientPair*) b;
    if (pa->cost < pb->cost)
        return -1;
    if (pa->cost > pb->cost)
        return 1;
    return 0;
}

#define print_cost_matrix(cost_matrix, n_clients, n_facilities)                                                        \
    do {                                                                                                               \
        for (size_t i = 0; i < n_clients; i++) {                                                                       \
            for (size_t j = 0; j < n_facilities; j++) {                                                                \
                FacilityClientPair p = cost_matrix[i][j];                                                              \
                printf("c%d,%d = %.0f | ", p.client + 1, p.facility + 1, p.cost);                                      \
            }                                                                                                          \
            printf("\n");                                                                                              \
        }                                                                                                              \
    } while (0)

#define print_assignment(M, n_facilities)                                                                              \
    do {                                                                                                               \
        for (size_t i = 0; i < n_facilities; i++) {                                                                    \
            Assignment a = M[i];                                                                                       \
            if (a.count < 1) {                                                                                         \
                continue;                                                                                              \
            }                                                                                                          \
            printf("Facility %d: [", a.facility);                                                                      \
            for (size_t j = 0; j < a.count; j++) {                                                                     \
                printf("%d ", a.clients[j]);                                                                           \
            }                                                                                                          \
            printf("] \n");                                                                                            \
        }                                                                                                              \
    } while (0)

int read_ints_from_line(FILE* fp, int* buffer) {
    int count = 0;
    char line[BUFFER_SIZE];

    if (fgets(line, sizeof(line), fp) == NULL) {
        return -1;
    }

    int number;
    char* line_ptr = line;
    int chars_read;

    while (sscanf(line_ptr, "%d%n", &number, &chars_read) == 1) {
        buffer[count] = number;
        count++;
        line_ptr += chars_read;
    }
    return count;
}

bool read_problem_data(char* filename, Data* data) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return false;
    }
    int buffer[BUFFER_SIZE];

    // 1) Read Facilities
    int n_f = read_ints_from_line(fp, buffer);
    assert(n_f != -1 && "Could not read facilities");
    data->n_facilities = (size_t) n_f;
    for (int i = 0; i < n_f; i++) {
        arrpush(data->facilities, buffer[i]);
    }

    // 2) Read Opening Costs
    int count = read_ints_from_line(fp, buffer);
    assert(count != -1 && "Could not read opening cost line");
    assert(count == n_f && "First and second line must have same number of values");
    for (int i = 0; i < count; i++) {
        hmput(data->opening_costs, data->facilities[i], (double) buffer[i]);
    }

    // 3) Read Clients
    int n_c = read_ints_from_line(fp, buffer);
    assert(n_c != -1 && "Could not read client ids");
    data->n_clients = (size_t) n_c;
    for (int i = 0; i < n_c; i++) {
        arrpush(data->clients, buffer[i]);
    }

    // 4) Read Cost Matrix
    int c = 0;
    int row_count;
    while (c < n_c && (row_count = read_ints_from_line(fp, buffer)) != -1) {
        assert(row_count == n_f && "Cost row length must match number of facilities");
        for (int i = 0; i < row_count; i++) {
            pair_t p = {.client = data->clients[c], .facility = data->facilities[i]};
            hmput(data->connection_costs, p, (double) buffer[i]);
        }
        c++;
    }

    assert(c == n_c && "Not enough cost rows for the number of clients specified");
    fclose(fp);
    return true;
}

double connection_cost(Data* data, int client, int facility) {
    pair_t p = {.client = client, .facility = facility};
    return hmget(data->connection_costs, p);
}

double opening_cost(Data* data, int facility) { 
    return hmget(data->opening_costs, facility); 
}

double flp(Data* data, Assignment** assignment) {
    size_t n_facilities = data->n_facilities;
    size_t n_clients    = data->n_clients;

    // Initialize assignments
    for (size_t i = 0; i < n_facilities; i++) {
        Assignment a = {.count = 0, .facility = data->facilities[i], .clients = NULL};
        arrpush(*assignment, a);
    }

    int t = 0;
    UsedClients* U = NULL;
    
    // Track which facilities have been opened (to avoid counting opening cost multiple times)
    typedef struct { int key; bool value; } FacilityOpened;
    FacilityOpened* opened = NULL;

    for (size_t i = 0; i < n_clients; i++) {
        hmput(U, data->clients[i], true);
    }
    
    for (size_t i = 0; i < n_facilities; i++) {
        hmput(opened, data->facilities[i], false);
    }

    // Sort the connection cost of all facility-client pairs
    FacilityClientPair cost_matrix[n_clients][n_facilities];
    for (size_t i = 0; i < n_clients; i++) {
        int client = data->clients[i];
        for (size_t j = 0; j < n_facilities; j++) {
            int facility               = data->facilities[j];
            cost_matrix[i][j].facility = facility;
            cost_matrix[i][j].client   = client;
            cost_matrix[i][j].cost     = connection_cost(data, client, facility);
        }
        qsort(cost_matrix[i], n_facilities, sizeof(FacilityClientPair), compare_pairs);
    }
    print_cost_matrix(cost_matrix, n_clients, n_facilities);

    // Initialize cost effectiveness matrix
    CostEffectivenessMatrix* ce = NULL;
    for (size_t i = 0; i < n_facilities; i++) {
        CostEffectivenessMatrix c = {
            .facility   = data->facilities[i],
            .threshold  = -1,
            .count      = 0,
            .cost_ratio = 0.0,
            .clients    = NULL
        };
        arrpush(ce, c);
    }

    size_t n_unassigned = n_clients;
    while (n_unassigned > 0 && t < (int)n_facilities) {
        // Create temporary arrays for this iteration
        int** facility_clients = NULL;   // array of arrays
        double** facility_costs = NULL;  // array of arrays
        size_t* facility_counts = NULL;  // simple counts array
        
        // Initialize per-facility arrays
        for (size_t i = 0; i < n_facilities; i++) {
            int* clients = NULL;
            double* costs = NULL;
            arrpush(facility_clients, clients);
            arrpush(facility_costs, costs);
            arrpush(facility_counts, 0);
        }

        // Get all the pairs at rank t
        for (size_t i = 0; i < n_clients; i++) {
            int client = data->clients[i];
            
            // If client is already assigned, skip it
            if (!hmget(U, client)) {
                continue;
            }

            int facility = cost_matrix[i][t].facility;
            double cost = cost_matrix[i][t].cost;
            
            // Find facility index
            size_t fac_idx = 0;
            for (size_t j = 0; j < n_facilities; j++) {
                if (data->facilities[j] == facility) {
                    fac_idx = j;
                    break;
                }
            }
            
            arrpush(facility_clients[fac_idx], client);
            arrpush(facility_costs[fac_idx], cost);
            facility_counts[fac_idx]++;
        }

        // Compute cost effectiveness for each facility
        for (size_t i = 0; i < n_facilities; i++) {
            int facility = data->facilities[i];
            size_t ce_n_clients = facility_counts[i];
            
            if (ce_n_clients == 0) {
                continue;
            }
            
            double cost_ratio = 0;
            for (size_t j = 0; j < ce_n_clients; j++) {
                cost_ratio += facility_costs[i][j];
            }
            
            // Only add opening cost if facility hasn't been opened yet
            if (!hmget(opened, facility)) {
                cost_ratio += opening_cost(data, facility);
            }
            
            cost_ratio = cost_ratio / (double) ce_n_clients;

            if (ce[i].count > 0) {
                if (cost_ratio > ce[i].cost_ratio ||
                    (cost_ratio == ce[i].cost_ratio && ce_n_clients < ce[i].count)) {
                    continue;
                }
            }
            
            // Update cost effectiveness
            ce[i].facility   = facility;
            ce[i].threshold  = t;
            ce[i].count      = ce_n_clients;
            ce[i].cost_ratio = cost_ratio;
            
            // Free old clients array and create new one
            arrfree(ce[i].clients);
            ce[i].clients = NULL;
            for (size_t k = 0; k < ce_n_clients; k++) {
                arrpush(ce[i].clients, facility_clients[i][k]);
            }
        }

        // Find best facility
        double best_cost = INFINITY;
        int best_facility_idx = -1;
        size_t best_client_count = 0;

        for (size_t i = 0; i < n_facilities; i++) {
            if (ce[i].count < 1) {
                continue;
            }
            if (ce[i].cost_ratio < best_cost ||
                (ce[i].cost_ratio == best_cost && ce[i].count > best_client_count)) {
                best_cost         = ce[i].cost_ratio;
                best_client_count = ce[i].count;
                best_facility_idx = i;
            }
        }

        // Clean up temporary arrays
        for (size_t i = 0; i < n_facilities; i++) {
            arrfree(facility_clients[i]);
            arrfree(facility_costs[i]);
        }
        arrfree(facility_clients);
        arrfree(facility_costs);
        arrfree(facility_counts);

        if (best_facility_idx == -1) {
            break;
        }

        // Assign clients to best facility
        int best_facility = ce[best_facility_idx].facility;
        for (size_t i = 0; i < ce[best_facility_idx].count; i++) {
            int client = ce[best_facility_idx].clients[i];
            hmput(U, client, false);
            n_unassigned--;
        }
        
        // Mark facility as opened
        hmput(opened, best_facility, true);

        // Add clients to assignment
        for (size_t i = 0; i < best_client_count; i++) {
            arrpush((*assignment)[best_facility_idx].clients, ce[best_facility_idx].clients[i]);
            (*assignment)[best_facility_idx].count++;
        }

        ce[best_facility_idx].count = 0; // don't use this set again
        t++;
    }

    // Calculate total cost
    double total_cost = 0;
    for (size_t i = 0; i < n_facilities; i++) {
        Assignment a = (*assignment)[i];
        if (a.count < 1) {
            continue;
        }
        // Add opening cost for this facility (once)
        total_cost += opening_cost(data, a.facility);
        // Add connection costs for all clients
        for (size_t j = 0; j < a.count; j++) {
            total_cost += connection_cost(data, a.clients[j], a.facility);
        }
    }

    // Cleanup
    for (size_t i = 0; i < n_facilities; i++) {
        arrfree(ce[i].clients);
    }
    arrfree(ce);
    hmfree(U);
    hmfree(opened);

    return total_cost;
}

int main(int argc, char** argv) {
    Data data = {0};
    data.clients = NULL;
    data.facilities = NULL;
    data.connection_costs = NULL;
    data.opening_costs = NULL;

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

    Assignment* M = NULL;

    double total_cost = flp(&data, &M);
    printf("total cost: %f\n", total_cost);

    print_assignment(M, data.n_facilities);

    // Cleanup
    for (size_t i = 0; i < data.n_facilities; i++) {
        arrfree(M[i].clients);
    }
    arrfree(M);
    arrfree(data.facilities);
    arrfree(data.clients);
    hmfree(data.connection_costs);
    hmfree(data.opening_costs);

    return 0;
}
