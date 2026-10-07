#include "HexSolverWorker.h"

void HexSolverWorker::run()
{
    // Qt dayOfWeek(): 1=Mon..7=Sun → 0=Sun..6=Sat
    int weekday = (variant == 2) ? (date.dayOfWeek() % 7) : 0;

    HexSolveResult r = hex_solve_cancellable(
        date.month(),
        date.day(),
        weekday,
        variant,
        allowFlip,
        findAll,
        &m_cancel
    );

    result.elapsedMs = r.elapsed_ms;
    result.cancelled = r.cancelled;
    result.solutions.clear();
    for (size_t i = 0; i < r.count; ++i)
        result.solutions.push_back(r.solutions[i]);

    hex_free_result(r);
    emit solved();
}
