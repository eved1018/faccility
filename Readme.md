# Facility Location Problem Solver

# Facility Location Problem Solver

A C implementation of a greedy approximation algorithm for solving the uncapacitated facility location problem. Implemention is based on: https://www.jsoftware.us/index.php?m=content&c=index&a=show&catid=88&id=1445


## Input Format

Input files contain 4 sections (space-separated integers):

```
<facility_1> <facility_2> ... <facility_n>
<open_cost_1> <open_cost_2> ... <open_cost_n>
<client_1> <client_2> ... <client_m>
<connection_costs_matrix>
```

1. **Facility IDs** (one line)
2. **Opening costs** (one line, same count as facilities)
3. **Client IDs** (one line)
4. **Connection cost matrix** (m lines × n columns)
   - Each row = one client
   - Each column = cost to connect to that facility


## Usage

```bash
./facc <input_file>
```
