#pragma once
#include <cstdint>

// LCG-based shuffled cycle: visits every index in [0, n) exactly once
// per cycle, using a power-of-2 modulus LCG (Hull-Dobell full period).
struct LcgCycle {
    static constexpr uint32_t A = 1664525u;   // odd multiplier (Numerical Recipes)
    static constexpr uint32_t C = 1013904223u; // odd increment

    int      m_n    = 0;
    uint32_t m_mask = 0;  // 2^k − 1, where 2^k is smallest power-of-2 >= n
    uint32_t m_x    = 0;  // LCG state
    int      m_done = 0;  // valid values emitted in the current cycle

    void init(int n, uint32_t seed);
    int  next();           // returns index in [0, n), all values once per cycle
};

#include <QMainWindow>
#include <QDateEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>
#include <QAction>
#include <QVector>
#include "hexboardwidget.h"
#include "solverworker.h"

class SolveOverlay;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onTriggerSolve();
    void onSolved();
    void onPrev();
    void onNext();
    void onSlideshowTick();
    void onMidnight();
    void onVariantChanged();

private:
    void buildUi();
    void scheduleSolve();
    void scheduleMidnight();
    void showSolution(int idx);
    void updateTodayMarker();
    void updateBoardDate();
    int  currentWeekdayIdx() const;

    // ── widgets ──────────────────────────────────────────────────────────
    QDateEdit*      m_dateEdit      = nullptr;
    QPushButton*    m_todayBtn      = nullptr;
    QComboBox*      m_variantCombo  = nullptr;
    QCheckBox*      m_findAllChk    = nullptr;
    QCheckBox*      m_flipChk       = nullptr;
    QAction*        m_autoAct       = nullptr;
    QAction*        m_slideshowAct  = nullptr;
    QAction*        m_alwaysTopAct  = nullptr;
    QAction*        m_innerLinesAct = nullptr;
    HexBoardWidget* m_board         = nullptr;
    QPushButton*    m_prevBtn       = nullptr;
    QPushButton*    m_nextBtn       = nullptr;
    QLabel*         m_solLabel      = nullptr;
    QLabel*         m_statusLbl     = nullptr;

    // ── state ─────────────────────────────────────────────────────────────
    QTimer*              m_debounce     = nullptr;
    QTimer*              m_tickTimer    = nullptr;
    QTimer*              m_midnight     = nullptr;
    QTimer*              m_slideshow    = nullptr;
    bool                 m_savedFindAll = false;
    bool                 m_savedAutoMid = false;
    QElapsedTimer        m_elapsed;
    SolverWorker*        m_worker       = nullptr;
    SolveOverlay*        m_overlay      = nullptr;
    QVector<HexBoard>    m_solutions;
    int                  m_idx          = 0;
    bool                 m_solving      = false;
    bool                 m_pendingSolve = false;
    LcgCycle             m_lcgCycle;
};
