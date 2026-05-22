#pragma once
#include <QThread>
#include <QDate>
#include <atomic>
#include <vector>
#include "../solver-ffi/hex_solver.h"

struct HexSolverOutput {
    std::vector<HexBoard> solutions;
    double                elapsedMs = 0.0;
    bool                  cancelled = false;
};

class HexSolverWorker : public QThread {
    Q_OBJECT
public:
    explicit HexSolverWorker(QObject* parent = nullptr) : QThread(parent) {}

    // Set before start()
    QDate date;
    int   variant   = 0;   // 0=43v1, 1=43v2, 2=61-cell
    bool  allowFlip = true;
    bool  findAll   = false;

    HexSolverOutput result;

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
