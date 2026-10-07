#include "hex_solver.h"
#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    /* A request received before DFS starts must not be cleared at entry. */
    HexCancelToken cancelled = {0};
    hex_request_cancel(&cancelled);
    HexSolveResult r = hex_solve_cancellable(10, 7, 3, 2, true, true, &cancelled);
    CHECK(r.cancelled);
    CHECK(r.count == 0);
    hex_free_result(r);

    /* Cancelling one search must not cancel a new, independent search. */
    for (int variant = 0; variant < 3; ++variant) {
        HexCancelToken fresh = {0};
        r = hex_solve_cancellable(10, 7, 3, variant, true, false, &fresh);
        CHECK(!r.cancelled);
        CHECK(r.count == 1);
        int markers = 0;
        for (int y = 0; y < HEX_GRID; ++y) {
            for (int x = 0; x < HEX_GRID; ++x) {
                CHECK(r.solutions[0].cells[y][x] != 0);
                markers += r.solutions[0].cells[y][x] == 0xFE;
            }
        }
        CHECK(markers == (variant == 2 ? 3 : 2));
        hex_free_result(r);
    }

    /* Keep the original synchronous API usable for existing callers. */
    hex_cancel();
    r = hex_solve(10, 7, 3, 0, true, false);
    CHECK(!r.cancelled);
    CHECK(r.count == 1);
    hex_free_result(r);
    return 0;
}
