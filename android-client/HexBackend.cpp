#include "HexBackend.h"
#include <cstring>
#include <cstdio>

// ── Board maps (copied from HexBoardWidget) ───────────────────────────────

static const char* MAP43V1[HEX_GRID][HEX_GRID] = {
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

static const char* MAP43V2[HEX_GRID][HEX_GRID] = {
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

static const char* MAP61[HEX_GRID][HEX_GRID] = {
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

// ── helpers ────────────────────────────────────────────────────────────────

static bool isOnBoard(int r, int c)
{
    return r >= 0 && r < HEX_GRID
        && c >= 0 && c < HEX_GRID
        && (c - r) >= -4 && (c - r) <= 4;
}

static const char* mapAt(int r, int c, int variant)
{
    if (r < 0 || r >= HEX_GRID || c < 0 || c >= HEX_GRID) return "";
    switch (variant) {
    case 1:  return MAP43V2[r][c];
    case 2:  return MAP61[r][c];
    default: return MAP43V1[r][c];
    }
}

static int dateGroup(const char* lbl, QDate date, int weekday, int variant)
{
    if (!date.isValid()) return 0;

    static const char* MONTHS[12] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    static const char* WDAYS[7] = {
        "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
    };

    if (date.month() >= 1 && date.month() <= 12
        && strcmp(lbl, MONTHS[date.month()-1]) == 0)
        return -1;

    char daytxt[4];
    snprintf(daytxt, sizeof(daytxt), "%d", date.day());
    if (strcmp(lbl, daytxt) == 0) return -1;

    if (variant == 2 && weekday >= 0 && weekday <= 6
        && strcmp(lbl, WDAYS[weekday]) == 0)
        return -1;

    return 0;
}

// ── HexBackend ─────────────────────────────────────────────────────────────

HexBackend::HexBackend(QObject* parent) : QObject(parent)
{
    m_displayDate = QDate::currentDate();

    m_boardData.reserve(HEX_GRID * HEX_GRID);
    m_boardLabels.reserve(HEX_GRID * HEX_GRID);

    m_slideshowTimer = new QTimer(this);
    m_slideshowTimer->setInterval(2000);
    connect(m_slideshowTimer, &QTimer::timeout, this, &HexBackend::onSlideshowTick);

    updateBoardData(nullptr, m_displayDate);
}

// ── Property setters ───────────────────────────────────────────────────────

void HexBackend::setVariant(int v)
{
    if (m_variant == v) return;
    m_variant = v;
    // Enforce flip constraint
    if (v == 1) {
        m_allowFlip = false;
        emit allowFlipChanged();
    } else if (v == 2) {
        m_allowFlip = true;
        emit allowFlipChanged();
    }
    m_solutions.clear();
    m_idx = 0;
    m_solLabel.clear();
    m_statusText.clear();
    updateBoardData(nullptr, m_displayDate);
    emit variantChanged();
    emit boardChanged();
    emit statusTextChanged();
}

void HexBackend::setFindAll(bool v)
{
    if (m_findAll == v) return;
    m_findAll = v;
    emit findAllChanged();
}

void HexBackend::setAllowFlip(bool v)
{
    // Variants 1 and 2 override flip setting
    if (m_variant == 1) v = false;
    if (m_variant == 2) v = true;
    if (m_allowFlip == v) return;
    m_allowFlip = v;
    emit allowFlipChanged();
}

void HexBackend::setSlideshow(bool on)
{
    if (m_slideshow == on) return;
    m_slideshow = on;
    emit slideshowChanged();

    if (on) {
        if ((int)m_solutions.size() > 1) {
            initLcg();
            m_slideshowTimer->start();
        }
    } else {
        m_slideshowTimer->stop();
    }
}

// ── LCG shuffle ────────────────────────────────────────────────────────────

void HexBackend::initLcg()
{
    quint32 m = 1;
    while (m < (quint32)m_solutions.size()) m <<= 1;
    m_lcgM     = m;
    m_lcgState = (quint32)m_idx;
}

int HexBackend::nextLcgIdx()
{
    int n = (int)m_solutions.size();
    do { m_lcgState = (5 * m_lcgState + 1) % m_lcgM; }
    while ((int)m_lcgState >= n);
    return (int)m_lcgState;
}

void HexBackend::onSlideshowTick()
{
    if (m_solutions.empty()) return;
    m_idx = nextLcgIdx();
    showSolution(m_idx);
    m_solLabel = QString("Solution %1 / %2")
                     .arg(m_idx + 1).arg(m_solutions.size());
    emit boardChanged();
}

// ── Solve ──────────────────────────────────────────────────────────────────

void HexBackend::solve(int year, int month, int day)
{
    m_pendingDate    = QDate(year, month, day);
    m_pendingFindAll = m_findAll || m_slideshow;
    m_hasPending     = true;

    if (m_worker && m_worker->isRunning()) {
        m_worker->requestCancel();
        return;
    }
    doSolve();
}

void HexBackend::doSolve()
{
    if (!m_hasPending) return;
    m_hasPending = false;

    QDate date    = m_pendingDate;
    bool findAll  = m_pendingFindAll;

    m_displayDate = date;
    m_slideshowTimer->stop();
    m_solutions.clear();
    m_idx = 0;
    updateBoardData(nullptr, date);
    m_solLabel = "Solving…";
    emit boardChanged();

    if (m_worker) {
        m_worker->wait();
        delete m_worker;
    }
    m_worker = new HexSolverWorker(this);
    m_worker->date      = date;
    m_worker->variant   = m_variant;
    m_worker->allowFlip = m_allowFlip;
    m_worker->findAll   = findAll;

    m_solving = true;
    emit solvingChanged();

    connect(m_worker, &HexSolverWorker::solved, this, &HexBackend::onSolved);
    m_worker->start();
}

void HexBackend::cancel()
{
    if (m_worker && m_worker->isRunning())
        m_worker->requestCancel();
}

void HexBackend::onSolved()
{
    m_solving = false;
    emit solvingChanged();

    const HexSolverOutput& out = m_worker->result;
    m_solvedDate = m_worker->date;

    m_solutions.assign(out.solutions.begin(), out.solutions.end());
    m_idx = 0;

    if (!m_solutions.empty()) {
        showSolution(0);
        m_solLabel = QString("Solution 1 / %1").arg(m_solutions.size());
        if (m_slideshow && (int)m_solutions.size() > 1) {
            initLcg();
            m_slideshowTimer->start();
        }
    } else {
        updateBoardData(nullptr, m_solvedDate);
        m_solLabel = "No solution found";
    }

    if (out.cancelled)
        m_statusText = QString("Cancelled  ·  %1 solution(s)  ·  %2 ms")
                           .arg(m_solutions.size())
                           .arg(out.elapsedMs, 0, 'f', 1);
    else
        m_statusText = QString("%1 solution(s)  ·  %2 ms")
                           .arg(m_solutions.size())
                           .arg(out.elapsedMs, 0, 'f', 1);

    emit boardChanged();
    emit statusTextChanged();

    if (m_hasPending) doSolve();
}

void HexBackend::showSolution(int idx)
{
    updateBoardData(&m_solutions[idx], m_solvedDate);
}

void HexBackend::prevSolution()
{
    if (m_solutions.empty()) return;
    m_idx = (m_idx - 1 + (int)m_solutions.size()) % (int)m_solutions.size();
    showSolution(m_idx);
    m_solLabel = QString("Solution %1 / %2")
                     .arg(m_idx + 1).arg(m_solutions.size());
    m_lcgState = (quint32)m_idx;
    if (m_slideshowTimer->isActive()) m_slideshowTimer->start();
    emit boardChanged();
}

void HexBackend::nextSolution()
{
    if (m_solutions.empty()) return;
    m_idx = (m_idx + 1) % (int)m_solutions.size();
    showSolution(m_idx);
    m_solLabel = QString("Solution %1 / %2")
                     .arg(m_idx + 1).arg(m_solutions.size());
    m_lcgState = (quint32)m_idx;
    if (m_slideshowTimer->isActive()) m_slideshowTimer->start();
    emit boardChanged();
}

// ── Board data ─────────────────────────────────────────────────────────────

void HexBackend::updateBoardData(const HexBoard* board, QDate date)
{
    // Qt dayOfWeek(): 1=Mon..7=Sun → 0=Sun..6=Sat
    int weekday = (m_variant == 2) ? (date.dayOfWeek() % 7) : 0;

    m_boardData.clear();
    m_boardLabels.clear();

    for (int r = 0; r < HEX_GRID; ++r) {
        for (int c = 0; c < HEX_GRID; ++c) {
            const char* lbl = mapAt(r, c, m_variant);

            int group;
            if (!isOnBoard(r, c)) {
                group = -2;
            } else if (board) {
                int v = (int)(unsigned char)board->cells[r][c];
                if (v == 0xFF) {
                    group = (m_variant != 2) ? -2 : 0;
                } else if (v == 0xFE) {
                    group = -1;
                } else if (v == 0) {
                    group = 0;
                } else {
                    group = v;
                }
            } else {
                if (strcmp(lbl, "NON") == 0) {
                    group = (m_variant != 2) ? -2 : 0;
                } else {
                    group = dateGroup(lbl, date, weekday, m_variant);
                }
            }

            m_boardData.append(group);

            if (lbl[0] == '\0' || strcmp(lbl, "NON") == 0)
                m_boardLabels.append(QString());
            else
                m_boardLabels.append(QString::fromLatin1(lbl));
        }
    }
}
