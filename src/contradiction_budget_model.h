#pragma once
#include "archive_common.h"

namespace archive {

enum class ContradictionBudgetScope {
    WholeArchive,
    Era,
    ArtifactType,
    ContradictionType,
    ContradictionCause,
    Mystery,
    EvidencePotential,
    KnowledgeHorizon
};

enum class ContradictionBudgetSeverity {
    Low,
    Moderate,
    High,
    Critical
};

enum class ContradictionBudgetStatus {
    WithinBudget,
    Watch,
    OverBudget,
    Unknown
};

enum class ContradictionBudgetReasonCode {
    None,
    ProductiveAmbiguity,
    ValidRitualContradiction,
    ValidLegalFiction,
    ExpectedDamageDisagreement,
    ProtectedMysteryPressure,
    TooCleanArchive,
    WatchDensity,
    OverBudgetDensity,
    WatchUnresolvedRatio,
    OverBudgetUnresolvedRatio,
    GenerationBugPressure,
    KnowledgeHorizonPressure,
    MissingContradictionCause,
    InvalidMetric
};

struct ContradictionBudgetPolicy {
    double max_contradiction_density_watch = 0.10;
    double max_contradiction_density_over_budget = 0.25;

    double max_unresolved_ratio_watch = 0.25;
    double max_unresolved_ratio_over_budget = 0.50;

    double max_generation_bug_ratio_watch = 0.0;
    double max_generation_bug_ratio_over_budget = 0.0;

    double min_productive_ambiguity_density = 0.02;
    double max_too_clean_density = 0.0;

    bool warn_on_too_clean_archive = true;
    bool warn_on_generation_bugs = true;
    bool separate_protected_mystery_pressure = true;
    bool separate_ritual_or_legal_contradictions = true;
};

struct ContradictionBudgetBucket {
    std::string id;

    ContradictionBudgetScope scope = ContradictionBudgetScope::WholeArchive;
    std::string scope_id;

    std::size_t artifact_count = 0;
    std::size_t claim_count = 0;
    std::size_t contradiction_count = 0;
    std::size_t unresolved_contradiction_count = 0;
    std::size_t generation_bug_count = 0;
    std::size_t missing_cause_count = 0;
    std::size_t protected_mystery_count = 0;
    std::size_t protected_mystery_pressure_count = 0;
    std::size_t productive_ambiguity_count = 0;
    std::size_t knowledge_horizon_error_count = 0;

    double contradiction_density = 0.0;
    double unresolved_ratio = 0.0;
    double generation_bug_ratio = 0.0;

    ContradictionBudgetSeverity severity = ContradictionBudgetSeverity::Low;
    ContradictionBudgetStatus status = ContradictionBudgetStatus::WithinBudget;

    std::vector<ContradictionBudgetReasonCode> reason_codes;
    std::vector<std::string> representative_contradiction_ids;
    std::vector<std::string> notes;
};

struct ContradictionBudgetReport {
    std::vector<ContradictionBudgetBucket> buckets;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

[[nodiscard]] std::string to_string(ContradictionBudgetScope scope);
[[nodiscard]] std::string to_string(ContradictionBudgetSeverity severity);
[[nodiscard]] std::string to_string(ContradictionBudgetStatus status);
[[nodiscard]] std::string to_string(ContradictionBudgetReasonCode reason_code);
[[nodiscard]] ContradictionBudgetPolicy default_contradiction_budget_policy();

} // namespace archive
