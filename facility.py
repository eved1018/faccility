from math import inf
from collections import defaultdict


"""
Based off alg by https://www.jsoftware.us/index.php?m=content&c=index&a=show&catid=88&id=1445

C -> set of clients to connect to facility 
F -> facilities to open 
f(i) -> opening cost for facility i in F
d(i,j) -> cost of serving client j in C from facility i in F
"""


def show_cost_matrix(matrix):
    for row in matrix.values():
        rs = [f"c{c}{s} = {d}" for (s, c, d) in row]
        print(" ".join(rs))

def show_cost_effectivness(ce):
    for s, data in ce.items():
        for t, (r, cs) in data.items():
            print(f"{s} {t}: {r} {cs}") 


def connection_cost(c, s):
    connection_costs = {
        (1, 1): 4,
        (1, 2): 2,
        (1, 3): 5,
        (1, 4): 8,
        (1, 5): 6,
        (2, 1): 3,
        (2, 2): 2,
        (2, 3): 6,
        (2, 4): 7,
        (2, 5): 9,
        (3, 1): 6,
        (3, 2): 8,
        (3, 3): 1,
        (3, 4): 4,
        (3, 5): 7,
        (4, 1): 5,
        (4, 2): 7,
        (4, 3): 2,
        (4, 4): 3,
        (4, 5): 4,
        (5, 1): 7,
        (5, 2): 4,
        (5, 3): 10,
        (5, 4): 9,
        (5, 5): 3,
        (6, 1): 1,
        (6, 2): 9,
        (6, 3): 8,
        (6, 4): 3,
        (6, 5): 6,
        (7, 1): 12,
        (7, 2): 5,
        (7, 3): 8,
        (7, 4): 3,
        (7, 5): 7,
    }

    return connection_costs[(c, s)]


def opening_cost(s):
    data = {1: 6, 2: 10, 3: 12, 4: 5, 5: 8}
    return data[s]


def main(F: set, C: set):
    U = C
    t = 0
    M = {}  # open facilities -> served clients

    # step 1: sort opening costs of facility i in F - no idea why
    # facilities = sorted([opening_cost(s) for s in F])

    # step 2: sort the connection cost of all facility-client pairs
    cost_matrix = {}
    for c in C:
        row = []
        for s in F:
            row.append((s, c, connection_cost(c, s)))

        row = sorted(row, key=lambda x: x[2])
        cost_matrix[c] = row

    print("Cost matrix")
    show_cost_matrix(cost_matrix)

    # step 3.0: iterate until all c in U have been connected (removed from U)
    cost_effectivness = {}
    

    while len(U):
        # step 3.1: get the t-th ranked facility-client pair
        # TODO what if cost is the same for 2 pairs
        pairs_at_rank = [r[t] for c, r in cost_matrix.items() if c in U]
        group_by_facility = defaultdict(list)
        for s, c, d in pairs_at_rank:
            group_by_facility[s].append((c, d))

        for s, cs in group_by_facility.items():
            r = sum([i[1] for i in cs])
            if s not in M:  # closed facility
                r += opening_cost(s)
            r = r / len(cs)
            
            if s in cost_effectivness:
                pt, pr, pcs = cost_effectivness[s]
                if (r > pr) or (r == pr and len(cs) < len(pcs)):
                    continue
            cost_effectivness[s] = (t, r, [i[0] for i in cs])

        # print(cost_effectivness)

        best_assignment = None
        best_cost = inf

        for s, (t, r, cs) in cost_effectivness.items():
            if (r < best_cost) or (
                r == best_cost
                and best_assignment is not None
                and len(cs) > len(best_assignment[2])
            ):
                best_cost = r
                best_assignment = (s, t, cs)

        # step 3.2
        assert(best_assignment is not None)
        s, t, cs = best_assignment
        U = U - set(cs)
        del cost_effectivness[s]
        if s not in M:
            M[s] = []
        M[s].extend(cs)

        # # step 3.3 keep only the most cost effective threshold per facility
        # # TODO why dont we only update cost effectiveness if cost is lower in step 3.2??
        # tmp = {}
        # for s, data in cost_effectivness.items():
        #     if not data: # top level kety can exists after deletion above
        #         continue
        #     tmp[s] = {}
        #     tmp_best_assign = None
        #     tmp_best_cost = inf
        #     for t, (r, cs) in data.items():
        #         if (r < tmp_best_cost) or (
        #             r == tmp_best_cost
        #             and tmp_best_assign is not None
        #             and len(cs) > len(tmp_best_assign[2])
        #         ):
        #             tmp_best_cost = r
        #             tmp_best_assign = (s, t, cs)
        #     assert(tmp_best_assign is not None)
        #     tmp[s][tmp_best_assign[1]] = (tmp_best_cost, tmp_best_assign[2])
        # cost_effectivness = tmp

        # step 3.4 update t 
        t += 1
    
    total_cost = 0
    for s, cs in M.items():
        total_cost += opening_cost(s)
        for c in cs:
            total_cost += connection_cost(c, s)

    return dict(M), total_cost


F = {1, 2, 3, 4, 5}
C = {1, 2, 3, 4, 5, 6, 7}
m, tc = main(F, C)
print(f"Total cost: {tc}")
print(f"Mapping: {m}")
assert(m == {2: [1, 2, 5, 7], 4: [3, 4, 6]})

