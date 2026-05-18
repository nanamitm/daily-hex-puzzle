#include "solverworker.h"

void SolverWorker::run()
{
    m_cancelled.store(false, std::memory_order_relaxed);

    HexSolveResult r = hex_solve(
        date.month(),
        date.day(),
        weekday,
        variant,
        allowFlip,
        findAll
    );

    result.elapsedMs = r.elapsed_ms;
    result.cancelled = m_cancelled.load(std::memory_order_relaxed);
    result.solutions.clear();
    for (size_t i = 0; i < r.count; ++i)
        result.solutions.push_back(r.solutions[i]);

    hex_free_result(r);
    emit solved();
}
