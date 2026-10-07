#include "solverworker.h"

void SolverWorker::run()
{
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
