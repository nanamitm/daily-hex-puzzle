/* hex_solver.h — C FFI for the hex puzzle solver */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define HEX_GRID 9

/* cells: 0=empty, 1-11=piece ID, 0xFE=date/weekday marker, 0xFF=wall/off-board */
typedef struct {
    uint8_t cells[HEX_GRID][HEX_GRID];
} HexBoard;

typedef struct {
    HexBoard* solutions;  /* malloc'd array, free with hex_free_result */
    size_t    count;
    double    elapsed_ms;
    bool      cancelled;
} HexSolveResult;

/*
 * variant:
 *   0 = 43-cell v1  (hex43.c pieces,   flip optional, NON cells = wall)
 *   1 = 43-cell v2  (hex43-2.c pieces, flip=false,    NON cells = wall)
 *   2 = 61-cell     (hex61.c pieces,   flip required, weekday cell used)
 *
 * month:   1-12
 * day:     1-31
 * weekday: 0=Sun..6=Sat (only used for variant==2)
 */
HexSolveResult hex_solve(int month, int day, int weekday,
                         int variant, bool allow_flip, bool find_all);

/* Initialize to {0} before starting the worker. Keep alive until it finishes.
 * Use a separate token for each search; do not reset it while in use. */
typedef struct {
    long requested;
} HexCancelToken;

HexSolveResult hex_solve_cancellable(int month, int day, int weekday,
                                     int variant, bool allow_flip, bool find_all,
                                     HexCancelToken* cancel);
void hex_request_cancel(HexCancelToken* cancel);

/* Stops the legacy hex_solve search. For worker threads use a per-search token. */
void hex_cancel(void);

void hex_free_result(HexSolveResult r);

#ifdef __cplusplus
}
#endif
