#pragma once
#include <QThread>
#include <QDate>
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
        hex_request_cancel(&m_cancel);
    }

signals:
    void solved();

protected:
    void run() override;

private:
    HexCancelToken m_cancel{0};
};
