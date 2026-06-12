#pragma once
#include "archive_engine_state.h"
#include "contradiction_budget_model.h"

namespace archive {

[[nodiscard]] ContradictionBudgetBucket compute_archive_contradiction_budget(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::vector<ContradictionBudgetBucket> compute_contradiction_budget_by_cause(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::vector<ContradictionBudgetBucket> compute_contradiction_budget_by_type(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::vector<ContradictionBudgetBucket> compute_contradiction_budget_by_mystery(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] ContradictionBudgetReport compute_contradiction_budget(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::vector<std::string> validate_contradiction_budget_report(
    const ContradictionBudgetReport& report
);

[[nodiscard]] std::string format_contradiction_budget_summary(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_contradiction_budget_validation(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_contradiction_budget_buckets(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_contradiction_budget_bucket_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& bucket_id
);

} // namespace archive
