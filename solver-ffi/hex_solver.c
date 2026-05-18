/*
 * hex_solver.c
 *
 * Thin FFI wrapper around the three hex-puzzle C solvers.
 * Compiled as a shared library (DLL on Windows, .so/.dylib on Unix).
 *
 * The three solver variants share the same algorithm (puzzlelib.c) but
 * differ in their piece definitions and board map.  Because each solver
 * file defines the same global symbols (piece[], map[][], …) we compile
 * each variant inside its own translation unit by #including the headers
 * inside isolated static functions — the trick is to use local structs and
 * static variables so the three sets of globals never clash at link time.
 *
 * Implementation strategy
 * ───────────────────────
 * Each variant is self-contained in a separate .c file that we include
 * here inside a distinct namespace-like block using a macro wrapper.
 * Rather than fighting with the existing include structure, we replicate
 * the small solver core inline for each variant, parameterised by the
 * piece/map data.  This keeps the FFI independent of the original
 * solver43.c / solver61.c main() programs.
 */

#include "hex_solver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── cancellation flag (written by hex_cancel, read by DFS) ─────────────── */
static volatile int g_cancel = 0;

void hex_cancel(void) { g_cancel = 1; }

void hex_free_result(HexSolveResult r)
{
    free(r.solutions);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Shared solver core
 * The three variants all use the same 9×9 Field and the same DFS algorithm.
 * We parameterise via function pointers / passed-in data so we can have one
 * copy of the algorithm and three sets of piece/map data.
 * ═══════════════════════════════════════════════════════════════════════════ */

#define F_SIZE   9
#define F_CENTER 4
#define POSE     12
#define MAX_PN   11   /* hex61 has 11 pieces */

typedef struct {
    int p[F_SIZE][F_SIZE];
    int x, y;
} Field;

typedef struct {
    Field f;
    int   used[MAX_PN];
} Sdata;

/* ── field geometry helpers (identical to puzzlelib.c) ─────────────────── */

static Field simpleRot(Field f)
{
    Field nf;
    for (int y = 0; y < F_SIZE; y++)
        for (int x = 0; x < F_SIZE; x++) {
            int x2 = y, y2 = F_CENTER - x + y;
            nf.p[y][x] = (0 <= x2 && x2 < F_SIZE && 0 <= y2 && y2 < F_SIZE)
                         ? f.p[y2][x2] : -1;
        }
    return nf;
}

static Field maskField(Field f)
{
    for (int y = 0; y < F_SIZE; y++)
        for (int x = 0; x < F_SIZE; x++) {
            int x2 = y, y2 = F_CENTER - x + y;
            if (!(0 <= x2 && x2 < F_SIZE && 0 <= y2 && y2 < F_SIZE))
                f.p[y][x] = -1;
        }
    return f;
}

static int outofMask(const Field *fm, const Field *f)
{
    for (int y = 0; y < F_SIZE; y++)
        for (int x = 0; x < F_SIZE; x++)
            if (fm->p[y][x] < 0 && f->p[y][x] > 0)
                return 1;
    return 0;
}

static Field transpose(Field f)
{
    Field nf;
    for (int y = 0; y < F_SIZE; y++)
        for (int x = 0; x < F_SIZE; x++)
            nf.p[y][x] = f.p[x][y];
    return nf;
}

static Field shiftUp(const Field *fm, Field f)
{
    for (int i = 0; i < F_SIZE; i++) {
        int sum = 0;
        for (int x = 0; x < F_SIZE; x++)
            if (f.p[0][x] > 0) sum++;
        if (sum != 0) break;
        Field f2 = f;  /* start from current state */
        for (int y = 1; y < F_SIZE; y++)
            for (int x = 0; x < F_SIZE; x++)
                f2.p[y-1][x] = (f2.p[y][x] >= 0) ? f2.p[y][x] : 0;
        for (int x = F_CENTER; x < F_SIZE; x++)
            f2.p[F_SIZE-1][x] = 0;
        if (outofMask(fm, &f2)) break;
        f = maskField(f2);
    }
    return f;
}

static Field shiftXY(const Field *fm, Field f)
{
    f = shiftUp(fm, f);
    f = transpose(f);
    f = shiftUp(fm, f);
    f = transpose(f);
    return f;
}

static Field shiftRot(const Field *fm, Field f)
{
    return shiftXY(fm, simpleRot(f));
}

static int isSame(const Field *a, const Field *b)
{
    for (int y = 0; y < F_SIZE; y++)
        for (int x = 0; x < F_SIZE; x++)
            if (a->p[y][x] != b->p[y][x]) return 0;
    return 1;
}

static int isPlaceable(const Field *f, const Field *p, int sx, int sy)
{
    for (int y = 0; y < p->y; y++)
        for (int x = 0; x < p->x; x++)
            if (p->p[y][x] && f->p[y+sy][x+sx] != 0)
                return 0;
    return 1;
}

static void doPlace(Field *f, const Field *p, int sx, int sy)
{
    for (int y = 0; y < p->y; y++)
        for (int x = 0; x < p->x; x++)
            if (p->p[y][x])
                f->p[y+sy][x+sx] = p->p[y][x];
}

/* ── pose generation ────────────────────────────────────────────────────── */

static void genPoses(const Field *fm,
                     Field piece[][POSE], int poseNum[],
                     int pn_count, int allow_flip)
{
    for (int pn = 0; pn < pn_count; pn++) {
        poseNum[pn] = 1;
        piece[pn][0] = maskField(piece[pn][0]);
        Field f = piece[pn][0];
        for (int i = 0; i < 6; i++) {
            f = shiftRot(fm, f);
            int j;
            for (j = 0; j < poseNum[pn]; j++)
                if (isSame(&piece[pn][j], &f)) break;
            if (j == poseNum[pn])
                piece[pn][poseNum[pn]++] = f;
        }
        if (allow_flip) {
            f = transpose(piece[pn][0]);
            for (int i = 0; i < 6; i++) {
                f = shiftRot(fm, f);
                int j;
                for (j = 0; j < poseNum[pn]; j++)
                    if (isSame(&piece[pn][j], &f)) break;
                if (j == poseNum[pn])
                    piece[pn][poseNum[pn]++] = f;
            }
        }
        for (int pose = 0; pose < poseNum[pn]; pose++) {
            piece[pn][pose].x = 0;
            piece[pn][pose].y = 0;
            for (int y = 0; y < F_SIZE; y++)
                for (int x = 0; x < F_SIZE; x++)
                    if (piece[pn][pose].p[y][x] > 0) {
                        if (piece[pn][pose].x <= x) piece[pn][pose].x = x+1;
                        if (piece[pn][pose].y <= y) piece[pn][pose].y = y+1;
                    }
        }
    }
}

/* ── DFS ────────────────────────────────────────────────────────────────── */

typedef struct {
    HexBoard  *buf;
    size_t     count;
    size_t     cap;
    int        find_all;
} SolBuf;

static void solbuf_push(SolBuf *sb, const Field *f)
{
    if (sb->count == sb->cap) {
        sb->cap = sb->cap ? sb->cap * 2 : 16;
        sb->buf = realloc(sb->buf, sb->cap * sizeof(HexBoard));
    }
    HexBoard *b = &sb->buf[sb->count++];
    for (int y = 0; y < HEX_GRID; y++)
        for (int x = 0; x < HEX_GRID; x++) {
            int v = f->p[y][x];
            if      (v < 0)   b->cells[y][x] = 0xFF;  /* off-board / wall */
            else if (v == 0)  b->cells[y][x] = 0;
            else              b->cells[y][x] = (uint8_t)v;
        }
}

/* piece IDs used for markers (must not collide with piece IDs 1-11) */
#define MARK_NON  20   /* permanent wall within board */
#define MARK_DATE 21   /* month / day / weekday cell  */

static int placeCheck(Sdata *sd, int pn, int pn_count,
                      Field piece[][POSE], const int poseNum[],
                      SolBuf *sb)
{
    if (g_cancel) return 0;

    if (pn == pn_count) {
        solbuf_push(sb, &sd->f);
        return 1;
    }

    /* find top-left empty cell */
    int ex = -1, ey = -1;
    for (int y = 0; y < F_SIZE && ey < 0; y++)
        for (int x = 0; x < F_SIZE && ey < 0; x++)
            if (sd->f.p[y][x] == 0) { ex = x; ey = y; }
    if (ey < 0) return 0;

    int result = 0;
    for (int i = 0; i < pn_count; i++) {
        if (sd->used[i]) continue;
        for (int pose = 0; pose < poseNum[i]; pose++) {
            for (int py = 0; py < piece[i][pose].y; py++) {
                for (int px = 0; px < piece[i][pose].x; px++) {
                    if (piece[i][pose].p[py][px] <= 0) continue;
                    int posx = ex - px, posy = ey - py;
                    if (posx < 0 || posy < 0 ||
                        posx + piece[i][pose].x > F_SIZE ||
                        posy + piece[i][pose].y > F_SIZE) continue;
                    if (!isPlaceable(&sd->f, &piece[i][pose], posx, posy)) continue;

                    Sdata nsd = *sd;
                    nsd.used[i] = 1;
                    doPlace(&nsd.f, &piece[i][pose], posx, posy);
                    result += placeCheck(&nsd, pn+1, pn_count, piece, poseNum, sb);
                    if (!sb->find_all && result) return 1;
                }
            }
        }
    }
    return result;
}

/* ── mark helpers ────────────────────────────────────────────────────────── */

static void setMark(Field *f, const char *map[F_SIZE][F_SIZE],
                    const char *txt, int val)
{
    for (int y = 0; y < F_SIZE; y++)
        for (int x = 0; x < F_SIZE; x++)
            if (strcmp(map[y][x], txt) == 0)
                f->p[y][x] = val;
}

/* ── encode solved field → HexBoard (replace internal markers with 0xFE) ── */

static void encodeSolutions(HexBoard *boards, size_t count)
{
    /* MARK_NON (20) → 0xFF wall, MARK_DATE (21) → 0xFE date marker */
    for (size_t s = 0; s < count; s++)
        for (int y = 0; y < HEX_GRID; y++)
            for (int x = 0; x < HEX_GRID; x++) {
                uint8_t v = boards[s].cells[y][x];
                if      (v == MARK_NON)  boards[s].cells[y][x] = 0xFF;
                else if (v == MARK_DATE) boards[s].cells[y][x] = 0xFE;
            }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Variant data: 43-cell v1
 * ═══════════════════════════════════════════════════════════════════════════ */

static const char *MAP43V1[F_SIZE][F_SIZE] = {
    {                        "NON","NON","Dec","NON","NON",  "" ,  "" ,  "" ,  ""  },
    {                     "NON","Nov", "1" , "2" ,"Jan","NON",  "" ,  "" ,  ""  },
    {                  "Oct", "3" , "4" , "5" , "6" , "7" ,"Feb",  "" ,  ""  },
    {               "NON", "8" , "9" , "10", "11", "12", "13","NON",  ""  },
    {            "NON","Sep", "14", "15", "16", "17", "18","Mar","NON" },
    {           "" ,"NON", "19", "20", "21", "22", "23", "24","NON" },
    {        "" ,  "" ,"Aug", "25", "26", "27", "28", "29","Apr" },
    {     "" ,  "" ,  "" ,"NON","Jul", "30", "31","May","NON" },
    {  "" ,  "" ,  "" ,  "" ,"NON","NON","Jun","NON","NON" }
};

/* hex43.c pieces */
static Field PIECES43V1[10][POSE] = {
    {{{{ 1, 1, 0 }, { 1, 1, 1 }}}},
    {{{{ 2, 0, 0 }, { 2, 2, 2 }}}},
    {{{{ 3, 0, 0 }, { 3, 3, 3 }}}},
    {{{{ 4, 4, 4, 4 }}}},
    {{{{ 5, 5, 5, 5 }}}},
    {{{{ 6, 6 }, { 6, 6 }}}},
    {{{{ 0, 7, 0 }, { 7, 7, 0 }, { 0, 0, 7 }}}},
    {{{{ 8, 8, 0 }, { 8, 0, 8 }}}},
    {{{{ 0, 9 }, { 9, 9 }, { 9, 0 }}}},
    {{{{ 0, 0,10 }, {10,10,10 }}}},
};

/* ═══════════════════════════════════════════════════════════════════════════
 * Variant data: 43-cell v2
 * ═══════════════════════════════════════════════════════════════════════════ */

static const char *MAP43V2[F_SIZE][F_SIZE] = {
    {                        "NON","NON","Jan","NON","NON",  "" ,  "" ,  "" ,  ""  },
    {                     "NON","Sep","Jun","Apr","Feb","NON",  "" ,  "" ,  ""  },
    {                   "8" , "4" , "1" ,"Oct","Jul","May","Mar",  "" ,  ""  },
    {               "NON", "13", "9" , "5" , "2" ,"Nov","Aug","NON",  ""  },
    {            "NON", "20", "17", "14", "10", "6" , "3" ,"Dec","NON" },
    {           "" ,"NON", "24", "21", "18", "15", "11", "7" ,"NON" },
    {        "" ,  "" , "29", "27", "25", "22", "19", "16", "12" },
    {     "" ,  "" ,  "" ,"NON", "30", "28", "26", "23","NON" },
    {  "" ,  "" ,  "" ,  "" ,"NON","NON", "31","NON","NON" }
};

/* hex43-2.c pieces */
static Field PIECES43V2[10][POSE] = {
    {{{{ 1, 1, 0 }, { 1, 1, 1 }}}},
    {{{{ 2, 0, 0 }, { 2, 2, 2 }}}},
    {{{{ 0, 3, 0 }, { 3, 3, 3 }}}},
    {{{{ 4, 4, 4, 4 }}}},
    {{{{ 0, 0, 5 }, { 5, 5, 5 }}}},
    {{{{ 6, 0, 0, 0 }, { 0, 6, 6, 6 }}}},
    {{{{ 0, 7, 0 }, { 7, 7, 0 }, { 0, 0, 7 }}}},
    {{{{ 8, 8, 0 }, { 8, 0, 8 }}}},
    {{{{ 0, 9, 9 }, { 9, 9, 0 }}}},
    {{{{ 10,10 }, { 10,10 }}}},
};

/* ═══════════════════════════════════════════════════════════════════════════
 * Variant data: 61-cell
 * ═══════════════════════════════════════════════════════════════════════════ */

static const char *MAP61[F_SIZE][F_SIZE] = {
    {                       "May", "Jun",  "" , "Jul", "Aug",  "" ,  "" ,  "" ,  ""  },
    {                    "Apr", "Sun", "Mon", "Tue", "Wed", "Sep",  "" ,  "" ,  ""  },
    {                 "Mar", "NON", "Thu", "Fri", "Sat", "NON", "Oct",  "" ,  ""  },
    {              "Feb",  "1" ,  "2" ,  "3" ,  "4" ,  "5" ,  "6" , "Nov",  ""  },
    {           "Jan",  "7" ,  "8" ,  "9" , "10" , "11" , "12" , "13" , "Dec" },
    {         "" , "NON", "14" , "15" , "16" , "17" , "18" , "19" , "NON" },
    {      "" ,  "" , "NON", "20" , "21" , "22" , "23" , "24" , "NON" },
    {   "" ,  "" ,  "" , "NON", "25" , "26" , "27" , "28" , "NON" },
    {"" ,  "" ,  "" ,  "" , "NON", "29" , "30" , "31" , "NON" }
};

/* hex61.c pieces */
static Field PIECES61[11][POSE] = {
    {{{{ 1, 1, 1 }, { 1, 1, 1 }}}},
    {{{{ 2, 2, 2 }, { 0, 2, 2 }, { 0, 0, 2 }}}},
    {{{{ 3, 3, 3 }, { 0, 0, 3, 3, 3 }}}},
    {{{{ 4, 4, 4, 4, 4 }}}},
    {{{{ 5, 5 }, { 5, 5, 5 }}}},
    {{{{ 6, 6, 6, 6 }, { 6 }}}},
    {{{{ 7, 7, 7 }, { 7, 7, 0 }}}},
    {{{{ 8, 8, 8 }, { 8 }, { 8 }}}},
    {{{{ 0, 9, 9 }, { 0, 9, 0 }, { 9, 9, 0 }}}},
    {{{{ 10,10,10, 0, 0 }, { 0, 0, 0,10,10 }}}},
    {{{{ 0,11, 0 }, {11,11,11 }, { 0,11, 0 }}}},
};

/* ═══════════════════════════════════════════════════════════════════════════
 * hex_solve — public API
 * ═══════════════════════════════════════════════════════════════════════════ */

static const char *MTEXT[12] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};
static const char *DTEXT[7] = {
    "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

HexSolveResult hex_solve(int month, int day, int weekday,
                         int variant, bool allow_flip, bool find_all)
{
    g_cancel = 0;

    /* pick variant data */
    int         pn_count;
    Field     (*pieces)[POSE];
    const char *(*map)[F_SIZE];
    int         has_non;   /* whether the map uses "NON" permanent walls */
    int         has_weekday;

    if (variant == 2) {
        pn_count    = 11;
        pieces      = PIECES61;
        map         = MAP61;
        has_non     = 0;  /* 61-cell: NON cells are regular play cells, not walls */
        has_weekday = 1;
        allow_flip  = true;  /* 61-cell always needs flip */
    } else if (variant == 1) {
        pn_count    = 10;
        pieces      = PIECES43V2;
        map         = MAP43V2;
        has_non     = 1;
        has_weekday = 0;
        allow_flip  = false; /* v2 is single-sided */
    } else {
        pn_count    = 10;
        pieces      = PIECES43V1;
        map         = MAP43V1;
        has_non     = 1;
        has_weekday = 0;
    }

    /* work on local copies of pieces (genPoses mutates them) */
    Field   local_pieces[MAX_PN][POSE];
    int     poseNum[MAX_PN] = {0};
    memcpy(local_pieces, pieces, sizeof(Field) * pn_count * POSE);

    Field fm = {{{0}}};
    fm = maskField(fm);

    genPoses(&fm, local_pieces, poseNum, pn_count, allow_flip ? 1 : 0);

    /* build initial field: mark off-board as -1, NON/date as blocking values */
    Field f0 = fm;  /* already has -1 for off-board cells */

    if (has_non) {
        setMark(&f0, map, "NON", MARK_NON);
    }
    /* mark month and day cells */
    if (month >= 1 && month <= 12)
        setMark(&f0, map, MTEXT[month-1], MARK_DATE);
    if (day >= 1 && day <= 31) {
        char daytxt[4];
        snprintf(daytxt, sizeof(daytxt), "%d", day);
        setMark(&f0, map, daytxt, MARK_DATE);
    }
    if (has_weekday && weekday >= 0 && weekday <= 6)
        setMark(&f0, map, DTEXT[weekday], MARK_DATE);

    /* run DFS */
    clock_t t0 = clock();

    Sdata sd;
    memset(&sd, 0, sizeof(sd));
    sd.f = f0;

    SolBuf sb = {NULL, 0, 0, find_all ? 1 : 0};
    placeCheck(&sd, 0, pn_count, local_pieces, poseNum, &sb);

    double ms = (double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC;

    /* recode MARK_NON→0xFF and MARK_DATE→0xFE in every stored board */
    encodeSolutions(sb.buf, sb.count);

    HexSolveResult r;
    r.solutions  = sb.buf;
    r.count      = sb.count;
    r.elapsed_ms = ms;
    r.cancelled  = (bool)g_cancel;
    return r;
}
