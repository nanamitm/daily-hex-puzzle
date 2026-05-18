#pragma once
#include <QThread>
#include <QDate>
#include <atomic>
#include <vector>
#include "../solver-ffi/hex_solver.h"

struct SolveResult {
    std::vector<HexBoard> solutions;
    double                elapsedMs = 0.0;
    bool                  cancelled = false;
};

class SolverWorker : public QThread {
    Q_OBJECT
public:
    explicit SolverWorker(QObject* parent = nullptr) : QThread(parent) {}

    // Set before calling start()
    QDate date;
    int   variant   = 0;  // 0=43v1, 1=43v2, 2=61-cell
    int   weekday   = 0;  // 0=Sun..6=Sat (used for variant==2)
    bool  allowFlip = true;
    bool  findAll   = false;

    SolveResult result;

    void requestCancel() {
        hex_cancel();
        m_cancelled.store(true, std::memory_order_relaxed);
    }

signals:
    void solved();

protected:
    void run() override;

private:
    std::atomic<bool> m_cancelled{false};
};
