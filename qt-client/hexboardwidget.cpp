#include "hexboardwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <cstring>
#include <cstdio>

// ── board maps ────────────────────────────────────────────────────────────

const char* HexBoardWidget::MAP43V1[HEX_GRID][HEX_GRID] = {
    {"NON","NON","Dec","NON","NON",  "",    "",    "",    ""},
    {"NON","Nov", "1",  "2","Jan","NON",   "",    "",    ""},
    {"Oct", "3",  "4",  "5",  "6",  "7","Feb",   "",    ""},
    {"NON", "8",  "9", "10", "11", "12", "13","NON",    ""},
    {"NON","Sep","14", "15", "16", "17", "18","Mar","NON"},
    {  "","NON","19", "20", "21", "22", "23", "24","NON"},
    {  "",  "","Aug", "25", "26", "27", "28", "29","Apr"},
    {  "",  "",  "","NON","Jul", "30", "31","May","NON"},
    {  "",  "",  "",   "","NON","NON","Jun","NON","NON"},
};

const char* HexBoardWidget::MAP43V2[HEX_GRID][HEX_GRID] = {
    {"NON","NON","Jan","NON","NON",  "",    "",    "",    ""},
    {"NON","Sep","Jun","Apr","Feb","NON",   "",    "",    ""},
    {  "8",  "4",  "1","Oct","Jul","May","Mar",   "",    ""},
    {"NON", "13",  "9",  "5",  "2","Nov","Aug","NON",    ""},
    {"NON", "20", "17", "14", "10",  "6",  "3","Dec","NON"},
    {  "","NON", "24", "21", "18", "15", "11",  "7","NON"},
    {  "",  "", "29", "27", "25", "22", "19", "16", "12"},
    {  "",  "",  "","NON", "30", "28", "26", "23","NON"},
    {  "",  "",  "",   "","NON","NON", "31","NON","NON"},
};

const char* HexBoardWidget::MAP61[HEX_GRID][HEX_GRID] = {
    {"May","Jun",  "","Jul","Aug",  "",    "",    "",    ""},
    {"Apr","Sun","Mon","Tue","Wed","Sep",   "",    "",    ""},
    {"Mar","NON","Thu","Fri","Sat","NON","Oct",   "",    ""},
    {"Feb",  "1",  "2",  "3",  "4",  "5",  "6","Nov",   ""},
    {"Jan",  "7",  "8",  "9", "10", "11", "12", "13","Dec"},
    {  "","NON", "14", "15", "16", "17", "18", "19","NON"},
    {  "",  "","NON", "20", "21", "22", "23", "24","NON"},
    {  "",  "",  "","NON", "25", "26", "27", "28","NON"},
    {  "",  "",  "",   "","NON", "29", "30", "31","NON"},
};

// ── piece / UI colours ────────────────────────────────────────────────────

static const QColor PIECE_COLORS[] = {
    {},                      // 0  unused
    {  70, 130, 180},        // 1  steel blue
    { 255, 160,  50},        // 2  orange
    {  60, 179, 113},        // 3  sea green
    { 220,  80,  80},        // 4  red
    { 148, 103, 189},        // 5  purple
    {  64, 200, 200},        // 6  cyan
    { 240, 200,  50},        // 7  yellow
    { 220, 100, 180},        // 8  magenta
    { 100, 200,  80},        // 9  lime
    {  50, 180, 170},        // 10 teal
    { 180, 130,  70},        // 11 brown (hex61 only)
};
static constexpr int N_COLORS = int(sizeof(PIECE_COLORS)/sizeof(PIECE_COLORS[0]));

static const QColor DATE_BG {240, 235, 210};
static const QColor BORDER  { 60,  60,  60};

// ── HexBoardWidget ────────────────────────────────────────────────────────

HexBoardWidget::HexBoardWidget(QWidget* parent) : QWidget(parent)
{
    // Compute tight bounding box from the actual hex-grid extremes.
    // Row 4 spans cols 0-8 (widest row).  Row 0/8 are the vertical extremes.
    // hexCenter adds PAD_inner to every coordinate.
    constexpr double PAD_inner = 10.0;  // must match hexCenter()
    constexpr double PAD_outer = 14.0;  // extra margin around the board
    const double h = STEP * std::sqrt(3.0) / 2.0;

    // rightmost hex right edge:  cx(r=4,c=8) + RADIUS
    const double right  = STEP * 9.0 + PAD_inner + RADIUS;
    // bottommost hex bottom edge: cy(r=8) + RADIUS
    const double bottom = h * 9.0    + PAD_inner + RADIUS;

    setFixedSize(static_cast<int>(right  + PAD_outer),
                 static_cast<int>(bottom + PAD_outer));
}

void HexBoardWidget::setVariant(int variant)
{
    m_variant  = variant;
    m_hasCells = false;
    update();
}

void HexBoardWidget::setDate(QDate date, int weekday)
{
    m_date     = date;
    m_weekday  = weekday;
    m_hasCells = false;
    update();
}

void HexBoardWidget::setBoard(const uint8_t cells[HEX_GRID][HEX_GRID])
{
    for (int r = 0; r < HEX_GRID; ++r)
        for (int c = 0; c < HEX_GRID; ++c)
            m_cells[r][c] = cells[r][c];
    m_hasCells = true;
    update();
}

void HexBoardWidget::clearBoard()
{
    m_hasCells = false;
    update();
}

// ── helpers ───────────────────────────────────────────────────────────────

const char* HexBoardWidget::mapAt(int r, int c) const
{
    if (r < 0 || r >= HEX_GRID || c < 0 || c >= HEX_GRID) return "";
    switch (m_variant) {
    case 1:  return MAP43V2[r][c];
    case 2:  return MAP61[r][c];
    default: return MAP43V1[r][c];
    }
}

// Mirrors the JS formula: cx = STEP*(col + F_CENTER/2 + 1) - STEP*row/2
//                          cy = (STEP*sqrt3/2) * (row+1)
// plus a small offset so the board is centred in the widget.
QPointF HexBoardWidget::hexCenter(int row, int col) const
{
    constexpr double F_HALF = 2.0;   // F_CENTER/2 = 4/2
    constexpr double PAD    = 10.0;
    double cx = STEP * (col + F_HALF + 1.0) - STEP * row / 2.0 + PAD;
    double cy = (STEP * std::sqrt(3.0) / 2.0) * (row + 1.0) + PAD;
    return {cx, cy};
}

QPolygonF HexBoardWidget::hexPoly(int row, int col) const
{
    QPointF c = hexCenter(row, col);
    QPolygonF poly;
    for (int i = 0; i < 6; ++i) {
        double theta = i * 2.0 * M_PI / 6.0;
        poly << QPointF(c.x() + std::sin(theta) * RADIUS,
                        c.y() + std::cos(theta) * RADIUS);
    }
    return poly;
}

// Mathematical on-board test: |r - c| <= F_CENTER (= 4).
// This matches the mask() function in puzzlelib.c and is correct for all variants.
static bool isOnBoard(int r, int c)
{
    return r >= 0 && r < HEX_GRID && c >= 0 && c < HEX_GRID
        && (c - r) >= -4 && (c - r) <= 4;
}

int HexBoardWidget::cellGroup(int r, int c) const
{
    if (!isOnBoard(r, c)) return -2;  // outside the hexagonal boundary

    if (m_hasCells) {
        uint8_t v = m_cells[r][c];
        // isOnBoard() already filtered mathematical off-board cells above.
        // 0xFF here means NON permanent wall (43-cell variants only).
        // For 43-cell we hide NON cells (return -2 = transparent).
        if (v == 0xFF) return (m_variant != 2) ? -2 : 0;
        if (v == 0xFE) return -1;   // date/weekday marker
        if (v == 0)    return  0;   // empty
        return static_cast<int>(v); // piece ID 1-11
    }

    // Empty-board mode:
    // 43-cell NON cells are hidden; 61-cell NON cells are regular play cells.
    const char* lbl = mapAt(r, c);
    if (strcmp(lbl, "NON") == 0) return (m_variant != 2) ? -2 : 0;

    // Highlight the selected date / weekday cells
    if (m_date.isValid()) {
        static const char* MONTHS[12] = {
            "Jan","Feb","Mar","Apr","May","Jun",
            "Jul","Aug","Sep","Oct","Nov","Dec"
        };
        if (m_date.month() >= 1 && m_date.month() <= 12 &&
            strcmp(lbl, MONTHS[m_date.month()-1]) == 0) return -1;

        char daytxt[4];
        snprintf(daytxt, sizeof(daytxt), "%d", m_date.day());
        if (strcmp(lbl, daytxt) == 0) return -1;

        if (m_variant == 2) {
            static const char* WDAYS[7] = {
                "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
            };
            if (m_weekday >= 0 && m_weekday <= 6 &&
                strcmp(lbl, WDAYS[m_weekday]) == 0) return -1;
        }
    }
    return 0;
}

QString HexBoardWidget::cellLabel(int r, int c) const
{
    const char* lbl = mapAt(r, c);
    if (lbl[0] == '\0' || strcmp(lbl, "NON") == 0) return {};
    return QString::fromLatin1(lbl);
}

// ── paintEvent ────────────────────────────────────────────────────────────

void HexBoardWidget::setShowInnerLines(bool show)
{
    m_showInnerLines = show;
    update();
}

// ── helpers ───────────────────────────────────────────────────────────────

static QColor pieceColor(int g)
{
    return (g > 0 && g < N_COLORS) ? PIECE_COLORS[g]
                                   : PIECE_COLORS[g % (N_COLORS - 1) + 1];
}

// ── paintEvent ────────────────────────────────────────────────────────────

void HexBoardWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QFont fnt("Segoe UI", 8, QFont::Bold);
    p.setFont(fnt);

    // ── Pass 1: empty cells and date markers ──────────────────────────────
    for (int r = 0; r < HEX_GRID; ++r) {
        for (int c = 0; c < HEX_GRID; ++c) {
            int g = cellGroup(r, c);
            if (g == -2 || g >= 1) continue;  // skip off-board and pieces

            QPolygonF poly = hexPoly(r, c);
            QString   lbl  = cellLabel(r, c);

            if (g == -1) {
                p.setPen(QPen(BORDER, 1.5));
                p.setBrush(DATE_BG);
                p.drawPolygon(poly);
                if (!lbl.isEmpty()) {
                    p.setPen(QColor(30, 30, 30));
                    p.drawText(poly.boundingRect().toRect(), Qt::AlignCenter, lbl);
                }
            } else {  // g == 0: empty
                p.setPen(Qt::NoPen);
                p.setBrush(palette().color(QPalette::Mid));
                p.drawPolygon(poly);
                if (!lbl.isEmpty()) {
                    p.setPen(palette().color(QPalette::Text));
                    p.drawText(poly.boundingRect().toRect(), Qt::AlignCenter, lbl);
                }
            }
        }
    }

    // ── Pass 2: piece cells ───────────────────────────────────────────────
    if (m_showInnerLines) {
        // Per-hexagon fill (seams visible between adjacent hexes)
        for (int r = 0; r < HEX_GRID; ++r) {
            for (int c = 0; c < HEX_GRID; ++c) {
                int g = cellGroup(r, c);
                if (g < 1) continue;
                p.setPen(Qt::NoPen);
                p.setBrush(pieceColor(g));
                p.drawPolygon(hexPoly(r, c));
            }
        }
        // Borders at piece perimeters
        p.setRenderHint(QPainter::Antialiasing, false);
        p.setPen(QPen(BORDER, 1.5));
        const int dr[] = {-1, 0, 1, 1, 0,-1};
        const int dc[] = { 0, 1, 1, 0,-1,-1};
        for (int r = 0; r < HEX_GRID; ++r) {
            for (int c = 0; c < HEX_GRID; ++c) {
                int g = cellGroup(r, c);
                if (g < 1) continue;
                QPolygonF poly = hexPoly(r, c);
                for (int side = 0; side < 6; ++side) {
                    if (cellGroup(r + dr[side], c + dc[side]) != g)
                        p.drawLine(poly[side], poly[(side+1)%6]);
                }
            }
        }
        p.setRenderHint(QPainter::Antialiasing, true);
    } else {
        // Unified QPainterPath per piece — no internal seams.
        // Use a slightly enlarged radius so adjacent hexagons overlap and
        // QPainterPath::united() closes the gap between them.
        constexpr double RADIUS_FILL = RADIUS * 1.04;

        auto enlargedPoly = [&](int row, int col) {
            QPointF c = hexCenter(row, col);
            QPolygonF poly;
            for (int i = 0; i < 6; ++i) {
                double theta = i * 2.0 * M_PI / 6.0;
                poly << QPointF(c.x() + std::sin(theta) * RADIUS_FILL,
                                c.y() + std::cos(theta) * RADIUS_FILL);
            }
            return poly;
        };

        for (int g = 1; g < N_COLORS; ++g) {
            QPainterPath path;
            bool any = false;
            for (int r = 0; r < HEX_GRID; ++r) {
                for (int c = 0; c < HEX_GRID; ++c) {
                    if (cellGroup(r, c) != g) continue;
                    QPainterPath cell;
                    cell.addPolygon(enlargedPoly(r, c));
                    cell.closeSubpath();
                    path = any ? path.united(cell) : cell;
                    any = true;
                }
            }
            if (any) {
                p.setPen(QPen(BORDER, 1.5));
                p.setBrush(pieceColor(g));
                p.drawPath(path);
            }
        }
    }

    // ── Pass 3: date markers on top (already drawn in pass 1, re-draw only
    //            when needed to cover piece cells placed over them) ─────────
    for (int r = 0; r < HEX_GRID; ++r) {
        for (int c = 0; c < HEX_GRID; ++c) {
            if (cellGroup(r, c) != -1) continue;
            QPolygonF poly = hexPoly(r, c);
            p.setPen(QPen(BORDER, 1.5));
            p.setBrush(DATE_BG);
            p.drawPolygon(poly);
            QString lbl = cellLabel(r, c);
            if (!lbl.isEmpty()) {
                p.setPen(QColor(30, 30, 30));
                p.drawText(poly.boundingRect().toRect(), Qt::AlignCenter, lbl);
            }
        }
    }
}
