#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <ctime>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QCalendarWidget>
#include <QTextCharFormat>
#include <QMenu>
#include <cstdio>

// ── LcgCycle ───────────────────────────────────────────────────────────────

void LcgCycle::init(int n, uint32_t seed)
{
    m_n = n;
    // Smallest power of 2 >= n
    uint32_t m = 1;
    while (m < (uint32_t)n) m <<= 1;
    m_mask = m - 1;
    m_x    = seed & m_mask;
    m_done = n;  // triggers new-cycle logic on the first next() call
}

int LcgCycle::next()
{
    if (m_done >= m_n) {
        // Start of a new cycle.
        // Advance a few LCG steps so the next cycle's starting point —
        // and therefore its permutation — differs from the previous one.
        m_x = (A * m_x + C) & m_mask;
        m_x = (A * m_x + C) & m_mask;
        m_x = (A * m_x + C) & m_mask;
        m_done = 0;
    }
    // Advance until we land on a valid index (< n).
    // Because the LCG has full period 2^k, every value in [0, 2^k) appears
    // exactly once per period, so [0, n) values appear exactly n times.
    do { m_x = (A * m_x + C) & m_mask; } while (m_x >= (uint32_t)m_n);
    ++m_done;
    return (int)m_x;
}

// ── SolveOverlay (identical pattern to a-puzzle-a-day-solver) ─────────────
class SolveOverlay : public QWidget {
    Q_OBJECT
public:
    explicit SolveOverlay(QWidget* parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setGeometry(parent->rect());

        m_timeLabel = new QLabel("0.0 s", this);
        QFont f = m_timeLabel->font();
        f.setPointSize(22); f.setBold(true);
        m_timeLabel->setFont(f);
        m_timeLabel->setAlignment(Qt::AlignCenter);
        m_timeLabel->setStyleSheet("color: white; background: transparent;");

        m_cancelBtn = new QPushButton("Cancel", this);
        m_cancelBtn->setStyleSheet(
            "QPushButton { background:#e74c3c; color:white; border:none;"
            "  border-radius:6px; padding:6px 22px; font-size:13px; }"
            "QPushButton:hover   { background:#c0392b; }"
            "QPushButton:pressed { background:#a93226; }");
        connect(m_cancelBtn, &QPushButton::clicked,
                this,        &SolveOverlay::cancelClicked);

        layoutWidgets();
        hide();
    }

    void updateTime(qint64 ms) {
        m_timeLabel->setText(QString("%1 s").arg(ms / 1000.0, 0, 'f', 1));
    }

signals:
    void cancelClicked();

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor(0, 0, 0, 110));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(28, 28, 28, 230));
        QPainterPath path;
        path.addRoundedRect(panelRect(), 14, 14);
        p.drawPath(path);
    }

    void resizeEvent(QResizeEvent*) override { layoutWidgets(); }

private:
    QLabel*      m_timeLabel;
    QPushButton* m_cancelBtn;

    QRect panelRect() const {
        return QRect(0, 0, 180, 120)
            .translated((width()-180)/2, (height()-120)/2);
    }

    void layoutWidgets() {
        QRect pr = panelRect();
        m_timeLabel->setGeometry(pr.left(), pr.top()+14, pr.width(), 50);
        m_cancelBtn->adjustSize();
        int bw = qMax(m_cancelBtn->sizeHint().width(), 90);
        int bh = m_cancelBtn->sizeHint().height();
        m_cancelBtn->setGeometry(pr.center().x()-bw/2,
                                  pr.bottom()-bh-12, bw, bh);
    }
};

// ── MainWindow ─────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Daily Hex Puzzle Solver");
    buildUi();
}

MainWindow::~MainWindow()
{
    QSettings s;
    bool slideshowOn = m_slideshowAct->isChecked();
    s.setValue("findAll",      slideshowOn ? m_savedFindAll : m_findAllChk->isChecked());
    s.setValue("autoMidnight", slideshowOn ? m_savedAutoMid : m_autoAct->isChecked());
    s.setValue("slideshow",    slideshowOn);
    s.setValue("allowFlip",    m_flipChk->isChecked());
    s.setValue("variant",      m_variantCombo->currentIndex());
    s.setValue("alwaysOnTop",  m_alwaysTopAct->isChecked());
    s.setValue("hideInnerLines", m_innerLinesAct->isChecked());

    if (m_worker) { m_worker->quit(); m_worker->wait(); }
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* vbox = new QVBoxLayout(central);
    vbox->setSpacing(8);
    vbox->setContentsMargins(12, 12, 12, 12);

    // ── Row 1: date navigation ─────────────────────────────────────────────
    auto* row1 = new QHBoxLayout;

    auto* prevDay = new QPushButton("◀", this);
    prevDay->setFixedWidth(28);
    row1->addWidget(prevDay);

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    QString dateFmt = QLocale::system().dateFormat(QLocale::ShortFormat);
    if (!dateFmt.contains("yyyy")) dateFmt.replace("yy", "yyyy");
    m_dateEdit->setDisplayFormat(dateFmt);
    row1->addWidget(m_dateEdit);

    auto* nextDay = new QPushButton("▶", this);
    nextDay->setFixedWidth(28);
    row1->addWidget(nextDay);

    m_todayBtn = new QPushButton("Today", this);
    m_todayBtn->setFixedWidth(54);
    row1->addWidget(m_todayBtn);

    row1->addSpacing(8);

    // Variant selector
    m_variantCombo = new QComboBox(this);
    m_variantCombo->addItems({"43-cell v1", "43-cell v2", "61-cell"});
    row1->addWidget(m_variantCombo);

    row1->addSpacing(4);
    m_findAllChk = new QCheckBox("Find all", this);
    row1->addWidget(m_findAllChk);

    m_flipChk = new QCheckBox("Allow flip", this);
    row1->addWidget(m_flipChk);

    // Gear menu
    auto* gearBtn = new QPushButton("⚙", this);
    gearBtn->setFixedWidth(28);
    gearBtn->setToolTip("Settings");
    row1->addWidget(gearBtn);
    row1->addStretch();

    auto* gearMenu = new QMenu(this);
    m_autoAct      = gearMenu->addAction("Auto-update at midnight");
    m_slideshowAct = gearMenu->addAction("Slideshow (5 min)");
    m_alwaysTopAct  = gearMenu->addAction("Always on top");
    m_innerLinesAct = gearMenu->addAction("Hide inner lines");
    m_autoAct->setCheckable(true);
    m_slideshowAct->setCheckable(true);
    m_alwaysTopAct->setCheckable(true);
    m_innerLinesAct->setCheckable(true);
    connect(gearBtn, &QPushButton::clicked, this, [this, gearBtn, gearMenu]() {
        gearMenu->exec(gearBtn->mapToGlobal(gearBtn->rect().bottomLeft()));
    });

    vbox->addLayout(row1);

    // ── Board ──────────────────────────────────────────────────────────────
    m_board = new HexBoardWidget(this);
    vbox->addWidget(m_board, 0, Qt::AlignHCenter);

    // ── Navigation row ─────────────────────────────────────────────────────
    auto* navRow = new QHBoxLayout;
    m_prevBtn = new QPushButton("◀", this); m_prevBtn->setFixedWidth(40);
    m_nextBtn = new QPushButton("▶", this); m_nextBtn->setFixedWidth(40);
    m_solLabel = new QLabel("", this);
    m_solLabel->setAlignment(Qt::AlignCenter);
    m_prevBtn->setEnabled(false);
    m_nextBtn->setEnabled(false);
    navRow->addWidget(m_prevBtn);
    navRow->addWidget(m_solLabel, 1);
    navRow->addWidget(m_nextBtn);
    vbox->addLayout(navRow);

    // ── Status ─────────────────────────────────────────────────────────────
    m_statusLbl = new QLabel("", this);
    m_statusLbl->setAlignment(Qt::AlignCenter);
    vbox->addWidget(m_statusLbl);

    // ── Solve overlay ──────────────────────────────────────────────────────
    m_overlay = new SolveOverlay(m_board);
    connect(m_overlay, &SolveOverlay::cancelClicked, this, [this]() {
        if (m_worker && m_worker->isRunning()) {
            m_worker->requestCancel();
            m_tickTimer->stop();
            m_overlay->hide();
            m_solLabel->setText("Cancelling…");
        }
    });

    // ── Tick timer ─────────────────────────────────────────────────────────
    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(100);
    connect(m_tickTimer, &QTimer::timeout, this, [this]() {
        m_overlay->updateTime(m_elapsed.elapsed());
    });

    // ── Slideshow timer ────────────────────────────────────────────────────
    m_slideshow = new QTimer(this);
    m_slideshow->setInterval(5 * 60 * 1000);
    connect(m_slideshow, &QTimer::timeout, this, &MainWindow::onSlideshowTick);
    connect(m_alwaysTopAct, &QAction::toggled, this, [this](bool on) {
        setWindowFlag(Qt::WindowStaysOnTopHint, on);
        show();
    });
    connect(m_innerLinesAct, &QAction::toggled, this, [this](bool on) {
        m_board->setShowInnerLines(!on);
    });

    connect(m_slideshowAct, &QAction::toggled, this, [this](bool on) {
        if (on) {
            m_savedFindAll = m_findAllChk->isChecked();
            m_savedAutoMid = m_autoAct->isChecked();
            m_findAllChk->setChecked(true);
            m_autoAct->setChecked(true);
            m_findAllChk->setEnabled(false);
            m_autoAct->setEnabled(false);
            if (m_solutions.size() > 1) {
                m_lcgCycle.init(m_solutions.size(), (uint32_t)std::time(nullptr));
                m_idx = m_lcgCycle.next();
                showSolution(m_idx);
                m_solLabel->setText(QString("Solution %1 / %2").arg(m_idx+1).arg(m_solutions.size()));
                m_slideshow->start();
            }
        } else {
            m_slideshow->stop();
            m_findAllChk->setEnabled(true);
            m_autoAct->setEnabled(true);
            m_findAllChk->setChecked(m_savedFindAll);
            m_autoAct->setChecked(m_savedAutoMid);
        }
    });

    // ── Midnight timer ─────────────────────────────────────────────────────
    m_midnight = new QTimer(this);
    m_midnight->setSingleShot(true);
    connect(m_midnight, &QTimer::timeout, this, &MainWindow::onMidnight);
    scheduleMidnight();

    // ── Debounce timer ──────────────────────────────────────────────────────
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(300);
    connect(m_debounce, &QTimer::timeout, this, &MainWindow::onTriggerSolve);

    // ── Connections ────────────────────────────────────────────────────────
    connect(prevDay,    &QPushButton::clicked, this, [this]() {
        m_dateEdit->setDate(m_dateEdit->date().addDays(-1));
    });
    connect(nextDay,    &QPushButton::clicked, this, [this]() {
        m_dateEdit->setDate(m_dateEdit->date().addDays(1));
    });
    connect(m_todayBtn, &QPushButton::clicked, this, [this]() {
        m_dateEdit->setDate(QDate::currentDate());
    });
    connect(m_dateEdit, &QDateEdit::dateChanged, this, [this](QDate d) {
        m_todayBtn->setEnabled(d != QDate::currentDate());
        updateBoardDate();
        scheduleSolve();
    });
    connect(m_variantCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onVariantChanged(); });
    connect(m_findAllChk, &QCheckBox::toggled, this, [this](bool) { scheduleSolve(); });
    connect(m_flipChk,    &QCheckBox::toggled, this, [this](bool) { scheduleSolve(); });
    connect(m_prevBtn,    &QPushButton::clicked, this, &MainWindow::onPrev);
    connect(m_nextBtn,    &QPushButton::clicked, this, &MainWindow::onNext);

    // ── Restore settings ───────────────────────────────────────────────────
    QSettings s;
    m_savedFindAll = s.value("findAll",      false).toBool();
    m_savedAutoMid = s.value("autoMidnight", false).toBool();
    m_findAllChk->setChecked(m_savedFindAll);
    m_autoAct->setChecked(m_savedAutoMid);
    m_flipChk->setChecked(s.value("allowFlip", true).toBool());
    // Block variant signal during restore; onVariantChanged() is called once below.
    m_variantCombo->blockSignals(true);
    m_variantCombo->setCurrentIndex(s.value("variant", 0).toInt());
    m_variantCombo->blockSignals(false);
    m_slideshowAct->setChecked(s.value("slideshow", false).toBool());
    m_alwaysTopAct->setChecked(s.value("alwaysOnTop", false).toBool());
    if (m_alwaysTopAct->isChecked())
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
    m_innerLinesAct->setChecked(s.value("hideInnerLines", false).toBool());
    m_board->setShowInnerLines(!m_innerLinesAct->isChecked());

    if (m_slideshowAct->isChecked()) {
        m_findAllChk->setChecked(true);
        m_autoAct->setChecked(true);
        m_findAllChk->setEnabled(false);
        m_autoAct->setEnabled(false);
    }

    // ── Initial state ──────────────────────────────────────────────────────
    onVariantChanged();
    m_todayBtn->setEnabled(m_dateEdit->date() != QDate::currentDate());
    updateTodayMarker();

    adjustSize();
    setFixedSize(sizeHint());

    scheduleSolve();
}

// ── helpers ────────────────────────────────────────────────────────────────

int MainWindow::currentWeekdayIdx() const
{
    // Qt dayOfWeek(): 1=Mon..7=Sun → 0=Sun,1=Mon..6=Sat
    return m_dateEdit->date().dayOfWeek() % 7;
}

void MainWindow::updateBoardDate()
{
    int wdIdx = (m_variantCombo->currentIndex() == 2) ? currentWeekdayIdx() : 0;
    m_board->setDate(m_dateEdit->date(), wdIdx);
}

void MainWindow::onVariantChanged()
{
    int v = m_variantCombo->currentIndex();
    m_board->setVariant(v);

    // Block toggled signal while we change state programmatically.
    // Without this, setChecked() fires toggled → scheduleSolve() inside the
    // variant handler, which can race with the setEnabled() call that follows.
    m_flipChk->blockSignals(true);
    if (v == 1) {           // 43v2: single-sided — hide the option entirely
        m_flipChk->setChecked(false);
        m_flipChk->setVisible(false);
    } else if (v == 2) {    // 61-cell: flip always required — hide like v2
        m_flipChk->setChecked(true);
        m_flipChk->setVisible(false);
    } else {                // 43v1: user controls flip
        m_flipChk->setVisible(true);
        m_flipChk->setEnabled(true);
    }
    m_flipChk->blockSignals(false);

    updateBoardDate();
    scheduleSolve();
}

void MainWindow::scheduleSolve()
{
    m_debounce->start();
}

void MainWindow::scheduleMidnight()
{
    QDateTime now  = QDateTime::currentDateTime();
    QDateTime next = QDateTime(now.date().addDays(1), QTime(0, 0, 1));
    m_midnight->start(static_cast<int>(now.msecsTo(next)));
}

void MainWindow::onMidnight()
{
    QCalendarWidget* cal = m_dateEdit->calendarWidget();
    if (cal)
        cal->setDateTextFormat(QDate::currentDate().addDays(-1), QTextCharFormat());
    updateTodayMarker();
    m_todayBtn->setEnabled(m_dateEdit->date() != QDate::currentDate());
    if (m_autoAct->isChecked())
        m_dateEdit->setDate(QDate::currentDate());
    scheduleMidnight();
}

void MainWindow::updateTodayMarker()
{
    QCalendarWidget* cal = m_dateEdit->calendarWidget();
    if (!cal) return;
    QTextCharFormat fmt;
    fmt.setFontWeight(QFont::Bold);
    fmt.setBackground(QColor(255, 200, 50, 160));
    cal->setDateTextFormat(QDate::currentDate(), fmt);
}

// ── solve cycle ────────────────────────────────────────────────────────────

void MainWindow::onTriggerSolve()
{
    // Use m_solving flag instead of isRunning() to avoid a race where the
    // worker has already emitted solved() (queued to main thread) but
    // isRunning() still returns true — causing SingleShotConnection to be
    // added after the signal was emitted, so it never fires.
    if (m_solving) {
        m_pendingSolve = true;
        return;
    }
    m_pendingSolve = false;
    m_solving      = true;

    // Safe to delete here: m_solving was false, so onSolved() has already run.
    delete m_worker;
    m_worker = new SolverWorker(this);
    m_worker->date      = m_dateEdit->date();
    m_worker->variant   = m_variantCombo->currentIndex();
    m_worker->weekday   = (m_variantCombo->currentIndex() == 2)
                          ? currentWeekdayIdx() : 0;
    m_worker->allowFlip = m_flipChk->isChecked();
    m_worker->findAll   = m_findAllChk->isChecked();

    m_solutions.clear();
    m_idx = 0;
    m_prevBtn->setEnabled(false);
    m_nextBtn->setEnabled(false);
    m_solLabel->setText("Solving…");
    m_statusLbl->clear();

    m_elapsed.start();
    m_overlay->updateTime(0);
    m_overlay->setGeometry(m_board->rect());
    m_overlay->show();
    m_overlay->raise();
    m_tickTimer->start();

    connect(m_worker, &SolverWorker::solved, this, &MainWindow::onSolved);
    m_worker->start();
}

void MainWindow::onSolved()
{
    m_tickTimer->stop();
    m_overlay->hide();

    const SolveResult& out  = m_worker->result;
    const QDate        date = m_worker->date;

    for (const HexBoard& b : out.solutions)
        m_solutions.append(b);

    m_idx = 0;

    if (!m_solutions.isEmpty()) {
        showSolution(0);
        bool multi = m_solutions.size() > 1;
        m_prevBtn->setEnabled(multi);
        m_nextBtn->setEnabled(multi);
        m_solLabel->setText(QString("Solution 1 / %1").arg(m_solutions.size()));
        if (multi) m_lcgCycle.init(m_solutions.size(), (uint32_t)std::time(nullptr));
        if (m_slideshowAct->isChecked() && multi) {
            m_idx = m_lcgCycle.next();
            showSolution(m_idx);
            m_solLabel->setText(QString("Solution %1 / %2").arg(m_idx+1).arg(m_solutions.size()));
            m_slideshow->start();
        } else {
            m_slideshow->stop();
        }
    } else {
        m_solLabel->setText("No solution found");
        m_board->setDate(date, m_worker->weekday);
        if (!m_flipChk->isChecked())
            m_statusLbl->setText(
                "No solution found  —  try enabling \"Allow flip\"");
    }

    QString status;
    if (out.cancelled) {
        status = QString("Cancelled  ·  %1 solution(s) found  ·  %2 ms")
                     .arg(m_solutions.size())
                     .arg(out.elapsedMs, 0, 'f', 1);
    } else {
        status = QString("%1 solution(s)  ·  %2 ms")
                     .arg(m_solutions.size())
                     .arg(out.elapsedMs, 0, 'f', 1);
    }
    m_statusLbl->setText(status);

    // Mark solve as done, then honour any solve request that arrived while
    // the worker was running (e.g. the user toggled Find all mid-solve).
    m_solving = false;
    if (m_pendingSolve) {
        m_pendingSolve = false;
        scheduleSolve();
    }
}

void MainWindow::showSolution(int idx)
{
    m_board->setBoard(m_solutions[idx].cells);
}

void MainWindow::onPrev()
{
    if (m_solutions.isEmpty()) return;
    m_idx = (m_idx - 1 + m_solutions.size()) % m_solutions.size();
    showSolution(m_idx);
    m_solLabel->setText(
        QString("Solution %1 / %2").arg(m_idx+1).arg(m_solutions.size()));
    if (m_slideshow->isActive()) m_slideshow->start();
}

void MainWindow::onNext()
{
    if (m_solutions.isEmpty()) return;
    m_idx = (m_idx + 1) % m_solutions.size();
    showSolution(m_idx);
    m_solLabel->setText(
        QString("Solution %1 / %2").arg(m_idx+1).arg(m_solutions.size()));
    if (m_slideshow->isActive()) m_slideshow->start();
}

void MainWindow::onSlideshowTick()
{
    if (m_solutions.size() <= 1) return;
    m_idx = m_lcgCycle.next();
    showSolution(m_idx);
    m_solLabel->setText(
        QString("Solution %1 / %2").arg(m_idx+1).arg(m_solutions.size()));
}

#include "mainwindow.moc"
