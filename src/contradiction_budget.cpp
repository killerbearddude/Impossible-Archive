#include "contradiction_budget_api.h"
#include "archive_views_api.h"
#include "diagnostic_access_policy.h"
#include "knowledge_horizon_api.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>

namespace archive {
namespace {

[[nodiscard]] std::string id_token(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        if (std::isalnum(ch)) {
            return static_cast<char>(std::tolower(ch));
        }
        return '_';
    });
    text.erase(std::unique(text.begin(), text.end(), [](char lhs, char rhs) {
        return lhs == '_' && rhs == '_';
    }), text.end());
    while (!text.empty() && text.front() == '_') {
        text.erase(text.begin());
    }
    while (!text.empty() && text.back() == '_') {
        text.pop_back();
    }
    if (text.empty()) {
        return "unknown";
    }
    return text;
}

[[nodiscard]] bool is_generation_bug(const Contradiction& contradiction) {
    return contradiction.assigned_cause == ContradictionCause::UnresolvedGenerationBug;
}

[[nodiscard]] bool is_unresolved(const Contradiction& contradiction) {
    if (contradiction.assigned_cause == ContradictionCause::None || is_generation_bug(contradiction)) {
        return true;
    }
    std::string text = contradiction.public_resolution_status;
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return contains_substr(text, "unresolved") ||
           contains_substr(text, "under review") ||
           contains_substr(text, "generation bug");
}

[[nodiscard]] bool is_protected_mystery(const Mystery& mystery) {
    return mystery.reveal_mode == RevealMode::NeverFullyResolvable ||
           mystery.reveal_mode == RevealMode::AccessLocked ||
           mystery.reveal_mode == RevealMode::ContradictoryByDesign ||
           can_view(AccessLevel::Public, mystery.min_access) == false;
}

[[nodiscard]] std::vector<const Contradiction*> budget_contradictions(const ArchiveEngineState& state, AccessLevel access) {
    if (can_view(access, AccessLevel::Curator)) {
        std::vector<const Contradiction*> all;
        for (const auto& [id, contradiction] : state.public_archive.contradictions()) {
            (void)id;
            all.push_back(&contradiction);
        }
        std::sort(all.begin(), all.end(), [](const Contradiction* lhs, const Contradiction* rhs) {
            return lhs->id < rhs->id;
        });
        return all;
    }
    return visible_contradictions(state, access, kOpenEndedYear);
}

[[nodiscard]] std::size_t artifact_count_for(const ArchiveEngineState& state, AccessLevel access) {
    if (can_view(access, AccessLevel::Curator)) {
        return state.public_archive.artifacts().size();
    }
    return visible_artifacts(state, access, kOpenEndedYear).size();
}

[[nodiscard]] std::size_t claim_count_for(const ArchiveEngineState& state, AccessLevel access) {
    if (can_view(access, AccessLevel::Curator)) {
        return state.public_archive.claims().size();
    }
    return visible_claims(state, access, kOpenEndedYear).size();
}

[[nodiscard]] std::size_t protected_mystery_count_for(const ArchiveEngineState& state, AccessLevel access) {
    std::size_t count = 0;
    for (const Mystery& mystery : state.mysteries) {
        if (!can_view(access, mystery.min_access)) {
            continue;
        }
        if (is_protected_mystery(mystery)) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::size_t knowledge_horizon_error_count_for(const ArchiveEngineState& state, AccessLevel access) {
    const KnowledgeHorizonReport horizon_report = validate_knowledge_horizon(state, access);
    return horizon_report.errors.size();
}

[[nodiscard]] bool is_finite_nonnegative(double value);

void add_reason_code(ContradictionBudgetBucket& bucket, ContradictionBudgetReasonCode reason_code) {
    if (reason_code == ContradictionBudgetReasonCode::None) {
        return;
    }
    if (std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), reason_code) == bucket.reason_codes.end()) {
        bucket.reason_codes.push_back(reason_code);
    }
}

[[nodiscard]] bool has_reason_code(const ContradictionBudgetBucket& bucket, ContradictionBudgetReasonCode reason_code) {
    return std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), reason_code) != bucket.reason_codes.end();
}

[[nodiscard]] bool contradiction_has_claim_type(const ArchiveEngineState& state,
                                                const Contradiction& contradiction,
                                                ClaimType claim_type) {
    for (const std::string& claim_id : contradiction.involved_claim_ids) {
        const Claim* claim = state.public_archive.find_claim(claim_id);
        if (claim != nullptr && claim->type == claim_type) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool artifact_has_modifier(const ArchiveEngineState& state,
                                          const Contradiction& contradiction,
                                          EvidenceModifier modifier) {
    for (const std::string& artifact_id : contradiction.involved_artifact_ids) {
        const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
        if (artifact != nullptr && std::find(artifact->evidence_modifiers.begin(), artifact->evidence_modifiers.end(), modifier) != artifact->evidence_modifiers.end()) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool contradiction_touches_protected_mystery(const ArchiveEngineState& state,
                                                           const Contradiction& contradiction,
                                                           AccessLevel access) {
    for (const std::string& artifact_id : contradiction.involved_artifact_ids) {
        const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
        if (artifact == nullptr) {
            continue;
        }
        for (const std::string& mystery_id : artifact->mystery_links) {
            const auto it = std::find_if(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
                return mystery.id == mystery_id;
            });
            if (it != state.mysteries.end() && can_view(access, it->min_access) && is_protected_mystery(*it)) {
                return true;
            }
        }
    }
    return false;
}

void finish_bucket(ContradictionBudgetBucket& bucket, const ContradictionBudgetPolicy& policy) {
    bucket.contradiction_density = static_cast<double>(bucket.contradiction_count) /
                                   static_cast<double>(std::max<std::size_t>(1U, bucket.claim_count));
    bucket.unresolved_ratio = static_cast<double>(bucket.unresolved_contradiction_count) /
                              static_cast<double>(std::max<std::size_t>(1U, bucket.contradiction_count));
    bucket.generation_bug_ratio = static_cast<double>(bucket.generation_bug_count) /
                                  static_cast<double>(std::max<std::size_t>(1U, bucket.contradiction_count));

    if (!is_finite_nonnegative(bucket.contradiction_density) ||
        !is_finite_nonnegative(bucket.unresolved_ratio) ||
        !is_finite_nonnegative(bucket.generation_bug_ratio)) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::InvalidMetric);
    }

    if (bucket.contradiction_density >= policy.max_contradiction_density_over_budget) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::OverBudgetDensity);
    } else if (bucket.contradiction_density >= policy.max_contradiction_density_watch) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::WatchDensity);
    }

    if (bucket.unresolved_ratio >= policy.max_unresolved_ratio_over_budget && bucket.contradiction_count > 0U) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::OverBudgetUnresolvedRatio);
    } else if (bucket.unresolved_ratio >= policy.max_unresolved_ratio_watch && bucket.contradiction_count > 0U) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::WatchUnresolvedRatio);
    }

    if (policy.warn_on_generation_bugs &&
        (bucket.generation_bug_count > 0U || bucket.generation_bug_ratio > policy.max_generation_bug_ratio_watch)) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::GenerationBugPressure);
    }

    if (policy.warn_on_too_clean_archive &&
        bucket.claim_count > 0U &&
        bucket.contradiction_count == 0U &&
        bucket.contradiction_density <= policy.max_too_clean_density) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::TooCleanArchive);
    }

    if (bucket.missing_cause_count > 0U) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::MissingContradictionCause);
    }
    if (bucket.knowledge_horizon_error_count > 0U) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::KnowledgeHorizonPressure);
    }

    const bool critical = has_reason_code(bucket, ContradictionBudgetReasonCode::GenerationBugPressure) &&
                          has_reason_code(bucket, ContradictionBudgetReasonCode::OverBudgetUnresolvedRatio);
    const bool over_budget = has_reason_code(bucket, ContradictionBudgetReasonCode::OverBudgetDensity) ||
                             has_reason_code(bucket, ContradictionBudgetReasonCode::OverBudgetUnresolvedRatio) ||
                             has_reason_code(bucket, ContradictionBudgetReasonCode::GenerationBugPressure) ||
                             has_reason_code(bucket, ContradictionBudgetReasonCode::MissingContradictionCause) ||
                             has_reason_code(bucket, ContradictionBudgetReasonCode::InvalidMetric);
    const bool watch = has_reason_code(bucket, ContradictionBudgetReasonCode::WatchDensity) ||
                       has_reason_code(bucket, ContradictionBudgetReasonCode::WatchUnresolvedRatio) ||
                       has_reason_code(bucket, ContradictionBudgetReasonCode::ProtectedMysteryPressure) ||
                       has_reason_code(bucket, ContradictionBudgetReasonCode::TooCleanArchive) ||
                       has_reason_code(bucket, ContradictionBudgetReasonCode::KnowledgeHorizonPressure);

    if (critical) {
        bucket.severity = ContradictionBudgetSeverity::Critical;
        bucket.status = ContradictionBudgetStatus::OverBudget;
    } else if (over_budget) {
        bucket.severity = ContradictionBudgetSeverity::High;
        bucket.status = ContradictionBudgetStatus::OverBudget;
    } else if (watch) {
        bucket.severity = ContradictionBudgetSeverity::Moderate;
        bucket.status = ContradictionBudgetStatus::Watch;
    } else {
        bucket.severity = ContradictionBudgetSeverity::Low;
        bucket.status = ContradictionBudgetStatus::WithinBudget;
    }

    std::sort(bucket.reason_codes.begin(), bucket.reason_codes.end(), [](ContradictionBudgetReasonCode lhs, ContradictionBudgetReasonCode rhs) {
        return static_cast<int>(lhs) < static_cast<int>(rhs);
    });
}

void add_contradiction_to_bucket(ContradictionBudgetBucket& bucket,
                                 const ArchiveEngineState& state,
                                 const Contradiction& contradiction,
                                 AccessLevel access) {
    ++bucket.contradiction_count;
    if (is_unresolved(contradiction)) {
        ++bucket.unresolved_contradiction_count;
    }
    if (is_generation_bug(contradiction)) {
        ++bucket.generation_bug_count;
        add_reason_code(bucket, ContradictionBudgetReasonCode::GenerationBugPressure);
    }
    if (contradiction.assigned_cause == ContradictionCause::None) {
        ++bucket.missing_cause_count;
        add_reason_code(bucket, ContradictionBudgetReasonCode::MissingContradictionCause);
    }
    if (contradiction.assigned_cause == ContradictionCause::MythologizedMemory) {
        ++bucket.productive_ambiguity_count;
        add_reason_code(bucket, ContradictionBudgetReasonCode::ProductiveAmbiguity);
    }
    if (contradiction.type == ContradictionType::RitualContradiction ||
        contradiction.assigned_cause == ContradictionCause::RitualAnachronism ||
        artifact_has_modifier(state, contradiction, EvidenceModifier::RitualAnachronism)) {
        ++bucket.productive_ambiguity_count;
        add_reason_code(bucket, ContradictionBudgetReasonCode::ValidRitualContradiction);
    }
    if (contradiction_has_claim_type(state, contradiction, ClaimType::LegalFiction)) {
        ++bucket.productive_ambiguity_count;
        add_reason_code(bucket, ContradictionBudgetReasonCode::ValidLegalFiction);
    }
    if (contradiction.assigned_cause == ContradictionCause::Damage ||
        contradiction.assigned_cause == ContradictionCause::CalendarConversionError ||
        artifact_has_modifier(state, contradiction, EvidenceModifier::Damage) ||
        artifact_has_modifier(state, contradiction, EvidenceModifier::Mistranslation) ||
        artifact_has_modifier(state, contradiction, EvidenceModifier::LaterCopy) ||
        artifact_has_modifier(state, contradiction, EvidenceModifier::Interpolation) ||
        artifact_has_modifier(state, contradiction, EvidenceModifier::CalendarError)) {
        add_reason_code(bucket, ContradictionBudgetReasonCode::ExpectedDamageDisagreement);
    }
    if (contradiction_touches_protected_mystery(state, contradiction, access)) {
        ++bucket.protected_mystery_pressure_count;
        add_reason_code(bucket, ContradictionBudgetReasonCode::ProtectedMysteryPressure);
    }
    if (can_view(access, AccessLevel::Curator)) {
        add_unique_string(bucket.representative_contradiction_ids, contradiction.id);
    }
}

[[nodiscard]] ContradictionBudgetBucket base_bucket(const ArchiveEngineState& state,
                                                    AccessLevel access,
                                                    ContradictionBudgetScope scope,
                                                    std::string id,
                                                    std::string scope_id) {
    ContradictionBudgetBucket bucket;
    bucket.id = std::move(id);
    bucket.scope = scope;
    bucket.scope_id = std::move(scope_id);
    bucket.artifact_count = artifact_count_for(state, access);
    bucket.claim_count = claim_count_for(state, access);
    bucket.protected_mystery_count = protected_mystery_count_for(state, access);
    bucket.knowledge_horizon_error_count = knowledge_horizon_error_count_for(state, access);
    return bucket;
}

[[nodiscard]] std::vector<std::string> contradiction_mystery_ids(const ArchiveEngineState& state, const Contradiction& contradiction) {
    std::vector<std::string> mystery_ids;
    for (const std::string& artifact_id : contradiction.involved_artifact_ids) {
        const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
        if (artifact == nullptr) {
            continue;
        }
        for (const std::string& mystery_id : artifact->mystery_links) {
            add_unique_string(mystery_ids, mystery_id);
        }
    }
    return mystery_ids;
}

[[nodiscard]] bool has_archive_bucket(const ContradictionBudgetReport& report) {
    return std::any_of(report.buckets.begin(), report.buckets.end(), [](const ContradictionBudgetBucket& bucket) {
        return bucket.id == "contradiction_budget.archive" && bucket.scope == ContradictionBudgetScope::WholeArchive;
    });
}

[[nodiscard]] bool is_finite_nonnegative(double value) {
    return std::isfinite(value) && value >= 0.0;
}

void format_bucket_common(std::ostringstream& out, const ContradictionBudgetBucket& bucket) {
    out << "- id: " << bucket.id << "\n";
    out << "- scope: " << to_string(bucket.scope) << "\n";
    if (!bucket.scope_id.empty()) {
        out << "- scope_id: " << bucket.scope_id << "\n";
    }
    out << "- artifact_count: " << bucket.artifact_count << "\n";
    out << "- claim_count: " << bucket.claim_count << "\n";
    out << "- contradiction_count: " << bucket.contradiction_count << "\n";
    out << "- unresolved_contradiction_count: " << bucket.unresolved_contradiction_count << "\n";
    out << "- generation_bug_count: " << bucket.generation_bug_count << "\n";
    out << "- missing_cause_count: " << bucket.missing_cause_count << "\n";
    out << "- protected_mystery_count: " << bucket.protected_mystery_count << "\n";
    out << "- protected_mystery_pressure_count: " << bucket.protected_mystery_pressure_count << "\n";
    out << "- productive_ambiguity_count: " << bucket.productive_ambiguity_count << "\n";
    out << "- knowledge_horizon_error_count: " << bucket.knowledge_horizon_error_count << "\n";
    out << std::fixed << std::setprecision(3);
    out << "- contradiction_density: " << bucket.contradiction_density << "\n";
    out << "- unresolved_ratio: " << bucket.unresolved_ratio << "\n";
    out << "- generation_bug_ratio: " << bucket.generation_bug_ratio << "\n";
    out << "- severity: " << to_string(bucket.severity) << "\n";
    out << "- status: " << to_string(bucket.status) << "\n";
    out << "- reason_codes:";
    if (bucket.reason_codes.empty()) {
        out << " none";
    } else {
        for (ContradictionBudgetReasonCode reason_code : bucket.reason_codes) {
            out << " " << to_string(reason_code);
        }
    }
    out << "\n";
}

void format_policy_common(std::ostringstream& out, const ContradictionBudgetPolicy& policy) {
    out << "Policy thresholds:\n";
    out << std::fixed << std::setprecision(3);
    out << "- max_contradiction_density_watch: " << policy.max_contradiction_density_watch << "\n";
    out << "- max_contradiction_density_over_budget: " << policy.max_contradiction_density_over_budget << "\n";
    out << "- max_unresolved_ratio_watch: " << policy.max_unresolved_ratio_watch << "\n";
    out << "- max_unresolved_ratio_over_budget: " << policy.max_unresolved_ratio_over_budget << "\n";
    out << "- max_generation_bug_ratio_watch: " << policy.max_generation_bug_ratio_watch << "\n";
    out << "- max_generation_bug_ratio_over_budget: " << policy.max_generation_bug_ratio_over_budget << "\n";
    out << "- min_productive_ambiguity_density: " << policy.min_productive_ambiguity_density << "\n";
    out << "- max_too_clean_density: " << policy.max_too_clean_density << "\n";
    out << "- warn_on_too_clean_archive: " << (policy.warn_on_too_clean_archive ? 1 : 0) << "\n";
    out << "- warn_on_generation_bugs: " << (policy.warn_on_generation_bugs ? 1 : 0) << "\n";
}

} // namespace

[[nodiscard]] std::string to_string(ContradictionBudgetScope scope) {
    switch (scope) {
        case ContradictionBudgetScope::WholeArchive: return "whole_archive";
        case ContradictionBudgetScope::Era: return "era";
        case ContradictionBudgetScope::ArtifactType: return "artifact_type";
        case ContradictionBudgetScope::ContradictionType: return "contradiction_type";
        case ContradictionBudgetScope::ContradictionCause: return "contradiction_cause";
        case ContradictionBudgetScope::Mystery: return "mystery";
        case ContradictionBudgetScope::EvidencePotential: return "evidence_potential";
        case ContradictionBudgetScope::KnowledgeHorizon: return "knowledge_horizon";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ContradictionBudgetSeverity severity) {
    switch (severity) {
        case ContradictionBudgetSeverity::Low: return "low";
        case ContradictionBudgetSeverity::Moderate: return "moderate";
        case ContradictionBudgetSeverity::High: return "high";
        case ContradictionBudgetSeverity::Critical: return "critical";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ContradictionBudgetStatus status) {
    switch (status) {
        case ContradictionBudgetStatus::WithinBudget: return "within_budget";
        case ContradictionBudgetStatus::Watch: return "watch";
        case ContradictionBudgetStatus::OverBudget: return "over_budget";
        case ContradictionBudgetStatus::Unknown: return "unknown";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(ContradictionBudgetReasonCode reason_code) {
    switch (reason_code) {
        case ContradictionBudgetReasonCode::None: return "none";
        case ContradictionBudgetReasonCode::ProductiveAmbiguity: return "productive_ambiguity";
        case ContradictionBudgetReasonCode::ValidRitualContradiction: return "valid_ritual_contradiction";
        case ContradictionBudgetReasonCode::ValidLegalFiction: return "valid_legal_fiction";
        case ContradictionBudgetReasonCode::ExpectedDamageDisagreement: return "expected_damage_disagreement";
        case ContradictionBudgetReasonCode::ProtectedMysteryPressure: return "protected_mystery_pressure";
        case ContradictionBudgetReasonCode::TooCleanArchive: return "too_clean_archive";
        case ContradictionBudgetReasonCode::WatchDensity: return "watch_density";
        case ContradictionBudgetReasonCode::OverBudgetDensity: return "over_budget_density";
        case ContradictionBudgetReasonCode::WatchUnresolvedRatio: return "watch_unresolved_ratio";
        case ContradictionBudgetReasonCode::OverBudgetUnresolvedRatio: return "over_budget_unresolved_ratio";
        case ContradictionBudgetReasonCode::GenerationBugPressure: return "generation_bug_pressure";
        case ContradictionBudgetReasonCode::KnowledgeHorizonPressure: return "knowledge_horizon_pressure";
        case ContradictionBudgetReasonCode::MissingContradictionCause: return "missing_contradiction_cause";
        case ContradictionBudgetReasonCode::InvalidMetric: return "invalid_metric";
    }
    return "unknown";
}

[[nodiscard]] ContradictionBudgetPolicy default_contradiction_budget_policy() {
    return ContradictionBudgetPolicy{};
}

[[nodiscard]] ContradictionBudgetBucket compute_archive_contradiction_budget(const ArchiveEngineState& state,
                                                                             AccessLevel access) {
    ContradictionBudgetBucket bucket = base_bucket(
        state,
        access,
        ContradictionBudgetScope::WholeArchive,
        "contradiction_budget.archive",
        "archive"
    );
    for (const Contradiction* contradiction : budget_contradictions(state, access)) {
        add_contradiction_to_bucket(bucket, state, *contradiction, access);
    }
    bucket.notes.push_back("Telemetry only; this bucket does not reject, repair, mutate, or materialize archive state.");
    finish_bucket(bucket, default_contradiction_budget_policy());
    return bucket;
}

[[nodiscard]] std::vector<ContradictionBudgetBucket> compute_contradiction_budget_by_cause(const ArchiveEngineState& state,
                                                                                            AccessLevel access) {
    if (!can_view(access, AccessLevel::Curator)) {
        return {};
    }
    std::map<std::string, ContradictionBudgetBucket> buckets;
    for (const Contradiction* contradiction : budget_contradictions(state, access)) {
        const std::string cause = to_string(contradiction->assigned_cause);
        const std::string bucket_id = "contradiction_budget.cause." + id_token(cause);
        auto [it, inserted] = buckets.emplace(bucket_id, base_bucket(state, access, ContradictionBudgetScope::ContradictionCause, bucket_id, cause));
        (void)inserted;
        add_contradiction_to_bucket(it->second, state, *contradiction, access);
    }
    std::vector<ContradictionBudgetBucket> result;
    for (auto& [id, bucket] : buckets) {
        (void)id;
        finish_bucket(bucket, default_contradiction_budget_policy());
        result.push_back(std::move(bucket));
    }
    return result;
}

[[nodiscard]] std::vector<ContradictionBudgetBucket> compute_contradiction_budget_by_type(const ArchiveEngineState& state,
                                                                                           AccessLevel access) {
    std::map<std::string, ContradictionBudgetBucket> buckets;
    for (const Contradiction* contradiction : budget_contradictions(state, access)) {
        const std::string type = to_string(contradiction->type);
        const std::string bucket_id = "contradiction_budget.type." + id_token(type);
        auto [it, inserted] = buckets.emplace(bucket_id, base_bucket(state, access, ContradictionBudgetScope::ContradictionType, bucket_id, type));
        (void)inserted;
        add_contradiction_to_bucket(it->second, state, *contradiction, access);
    }
    std::vector<ContradictionBudgetBucket> result;
    for (auto& [id, bucket] : buckets) {
        (void)id;
        finish_bucket(bucket, default_contradiction_budget_policy());
        result.push_back(std::move(bucket));
    }
    return result;
}

[[nodiscard]] std::vector<ContradictionBudgetBucket> compute_contradiction_budget_by_mystery(const ArchiveEngineState& state,
                                                                                              AccessLevel access) {
    if (!can_view(access, AccessLevel::Curator)) {
        return {};
    }
    std::map<std::string, ContradictionBudgetBucket> buckets;
    for (const Contradiction* contradiction : budget_contradictions(state, access)) {
        for (const std::string& mystery_id : contradiction_mystery_ids(state, *contradiction)) {
            const std::string bucket_id = "contradiction_budget.mystery." + id_token(mystery_id);
            auto [it, inserted] = buckets.emplace(bucket_id, base_bucket(state, access, ContradictionBudgetScope::Mystery, bucket_id, mystery_id));
            (void)inserted;
            add_contradiction_to_bucket(it->second, state, *contradiction, access);
        }
    }
    std::vector<ContradictionBudgetBucket> result;
    for (auto& [id, bucket] : buckets) {
        (void)id;
        finish_bucket(bucket, default_contradiction_budget_policy());
        result.push_back(std::move(bucket));
    }
    return result;
}

[[nodiscard]] ContradictionBudgetReport compute_contradiction_budget(const ArchiveEngineState& state,
                                                                     AccessLevel access) {
    ContradictionBudgetReport report;
    report.buckets.push_back(compute_archive_contradiction_budget(state, access));
    for (ContradictionBudgetBucket bucket : compute_contradiction_budget_by_cause(state, access)) {
        report.buckets.push_back(std::move(bucket));
    }
    for (ContradictionBudgetBucket bucket : compute_contradiction_budget_by_type(state, access)) {
        report.buckets.push_back(std::move(bucket));
    }
    for (ContradictionBudgetBucket bucket : compute_contradiction_budget_by_mystery(state, access)) {
        report.buckets.push_back(std::move(bucket));
    }
    std::sort(report.buckets.begin(), report.buckets.end(), [](const ContradictionBudgetBucket& lhs, const ContradictionBudgetBucket& rhs) {
        return lhs.id < rhs.id;
    });
    report.errors = validate_contradiction_budget_report(report);
    return report;
}

[[nodiscard]] std::vector<std::string> validate_contradiction_budget_report(const ContradictionBudgetReport& report) {
    const ContradictionBudgetPolicy policy = default_contradiction_budget_policy();
    std::vector<std::string> errors;
    std::set<std::string> seen_ids;
    for (const ContradictionBudgetBucket& bucket : report.buckets) {
        if (bucket.id.empty()) {
            errors.push_back("ContradictionBudget bucket has empty id");
        } else if (!seen_ids.insert(bucket.id).second) {
            errors.push_back("ContradictionBudget bucket has duplicate id: " + bucket.id);
        }

        const bool invalid_metric = !is_finite_nonnegative(bucket.contradiction_density) ||
                                    !is_finite_nonnegative(bucket.unresolved_ratio) ||
                                    !is_finite_nonnegative(bucket.generation_bug_ratio);
        if (invalid_metric) {
            errors.push_back("ContradictionBudget bucket has invalid density/ratio value: " + bucket.id);
            if (!has_reason_code(bucket, ContradictionBudgetReasonCode::InvalidMetric)) {
                errors.push_back("ContradictionBudget invalid metric bucket lacks invalid_metric reason code: " + bucket.id);
            }
        }
        if (std::find(bucket.reason_codes.begin(), bucket.reason_codes.end(), ContradictionBudgetReasonCode::None) != bucket.reason_codes.end()) {
            errors.push_back("ContradictionBudget bucket carries non-informative none reason code: " + bucket.id);
        }
        if (bucket.unresolved_contradiction_count > bucket.contradiction_count) {
            errors.push_back("ContradictionBudget unresolved count exceeds contradiction count: " + bucket.id);
        }
        if (bucket.generation_bug_count > bucket.contradiction_count) {
            errors.push_back("ContradictionBudget generation bug count exceeds contradiction count: " + bucket.id);
        }
        if (bucket.missing_cause_count > bucket.contradiction_count) {
            errors.push_back("ContradictionBudget missing cause count exceeds contradiction count: " + bucket.id);
        }
        if (bucket.severity == ContradictionBudgetSeverity::Critical && bucket.status != ContradictionBudgetStatus::OverBudget) {
            errors.push_back("ContradictionBudget critical severity must be over_budget: " + bucket.id);
        }
        if (bucket.severity == ContradictionBudgetSeverity::Low && bucket.status == ContradictionBudgetStatus::OverBudget) {
            errors.push_back("ContradictionBudget low severity cannot be over_budget: " + bucket.id);
        }
        if (bucket.status == ContradictionBudgetStatus::Unknown && bucket.contradiction_count > 0U) {
            errors.push_back("ContradictionBudget unknown status is invalid for nonempty bucket: " + bucket.id);
        }
        if ((bucket.status == ContradictionBudgetStatus::Watch || bucket.status == ContradictionBudgetStatus::OverBudget) &&
            bucket.reason_codes.empty()) {
            errors.push_back("ContradictionBudget non-within bucket lacks reason code: " + bucket.id);
        }
        if (bucket.generation_bug_count > 0U &&
            !has_reason_code(bucket, ContradictionBudgetReasonCode::GenerationBugPressure)) {
            errors.push_back("ContradictionBudget generation bug bucket lacks generation_bug_pressure reason code: " + bucket.id);
        }
        if (bucket.contradiction_density >= policy.max_contradiction_density_over_budget &&
            !has_reason_code(bucket, ContradictionBudgetReasonCode::OverBudgetDensity)) {
            errors.push_back("ContradictionBudget over-budget density bucket lacks over_budget_density reason code: " + bucket.id);
        }
        if (bucket.contradiction_density >= policy.max_contradiction_density_watch &&
            bucket.contradiction_density < policy.max_contradiction_density_over_budget &&
            !has_reason_code(bucket, ContradictionBudgetReasonCode::WatchDensity)) {
            errors.push_back("ContradictionBudget watch density bucket lacks watch_density reason code: " + bucket.id);
        }
        if (bucket.unresolved_ratio >= policy.max_unresolved_ratio_over_budget &&
            bucket.contradiction_count > 0U &&
            !has_reason_code(bucket, ContradictionBudgetReasonCode::OverBudgetUnresolvedRatio)) {
            errors.push_back("ContradictionBudget over-budget unresolved bucket lacks over_budget_unresolved_ratio reason code: " + bucket.id);
        }
        if (bucket.unresolved_ratio >= policy.max_unresolved_ratio_watch &&
            bucket.unresolved_ratio < policy.max_unresolved_ratio_over_budget &&
            bucket.contradiction_count > 0U &&
            !has_reason_code(bucket, ContradictionBudgetReasonCode::WatchUnresolvedRatio)) {
            errors.push_back("ContradictionBudget watch unresolved bucket lacks watch_unresolved_ratio reason code: " + bucket.id);
        }
        if (policy.warn_on_too_clean_archive &&
            bucket.claim_count > 0U &&
            bucket.contradiction_count == 0U &&
            !has_reason_code(bucket, ContradictionBudgetReasonCode::TooCleanArchive)) {
            errors.push_back("ContradictionBudget too-clean bucket lacks too_clean_archive reason code: " + bucket.id);
        }
        if (bucket.missing_cause_count > 0U &&
            !has_reason_code(bucket, ContradictionBudgetReasonCode::MissingContradictionCause)) {
            errors.push_back("ContradictionBudget missing-cause bucket lacks missing_contradiction_cause reason code: " + bucket.id);
        }
        if (bucket.knowledge_horizon_error_count > 0U &&
            !has_reason_code(bucket, ContradictionBudgetReasonCode::KnowledgeHorizonPressure)) {
            errors.push_back("ContradictionBudget knowledge-horizon pressure lacks knowledge_horizon_pressure reason code: " + bucket.id);
        }
    }
    if (!has_archive_bucket(report)) {
        errors.push_back("ContradictionBudget report is missing archive-level bucket");
    }
    return errors;
}

[[nodiscard]] std::string format_contradiction_budget_summary(const ArchiveEngineState& state, AccessLevel access) {
    const ContradictionBudgetReport report = compute_contradiction_budget(state, access);
    const auto archive_it = std::find_if(report.buckets.begin(), report.buckets.end(), [](const ContradictionBudgetBucket& bucket) {
        return bucket.id == "contradiction_budget.archive";
    });
    std::ostringstream out;
    out << "ContradictionBudget summary:\n";
    out << "- behavior: advisory telemetry/validation only; no rejection, repair, generation, mutation, materialization, discovery, resolution, persistence, resolver/composition, or session state is introduced in v28.11.\n";
    out << "- bucket_count: " << report.buckets.size() << "\n";
    out << "- validation_errors: " << report.errors.size() << "\n";
    if (archive_it != report.buckets.end()) {
        const ContradictionBudgetBucket& bucket = *archive_it;
        out << "Archive bucket:\n";
        out << "- contradiction_count: " << bucket.contradiction_count << "\n";
        out << "- unresolved_contradiction_count: " << bucket.unresolved_contradiction_count << "\n";
        out << "- generation_bug_count: " << bucket.generation_bug_count << "\n";
        out << "- missing_cause_count: " << bucket.missing_cause_count << "\n";
        out << "- protected_mystery_pressure_count: " << bucket.protected_mystery_pressure_count << "\n";
        out << "- productive_ambiguity_count: " << bucket.productive_ambiguity_count << "\n";
        out << std::fixed << std::setprecision(3);
        out << "- contradiction_density: " << bucket.contradiction_density << "\n";
        out << "- unresolved_ratio: " << bucket.unresolved_ratio << "\n";
        out << "- generation_bug_ratio: " << bucket.generation_bug_ratio << "\n";
        out << "- severity: " << to_string(bucket.severity) << "\n";
        out << "- status: " << to_string(bucket.status) << "\n";
        if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::ContradictionBudgetBucket)) {
            out << "- reason_codes:";
            if (bucket.reason_codes.empty()) {
                out << " none";
            } else {
                for (ContradictionBudgetReasonCode reason_code : bucket.reason_codes) {
                    out << " " << to_string(reason_code);
                }
            }
            out << "\n";
        }
    }
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::ContradictionBudgetBucket)) {
        out << "- details: aggregate-only at this access level; bucket IDs, representative contradiction IDs, hidden causes, and notes are restricted.\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_contradiction_budget_validation(const ArchiveEngineState& state, AccessLevel access) {
    const ContradictionBudgetReport report = compute_contradiction_budget(state, access);
    std::ostringstream out;
    out << "ContradictionBudget validation:\n";
    out << "- result: " << (report.errors.empty() ? "passed" : "failed") << "\n";
    out << "- buckets: " << report.buckets.size() << "\n";
    out << "- errors: " << report.errors.size() << "\n";
    if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::ContradictionBudgetPolicy)) {
        format_policy_common(out, default_contradiction_budget_policy());
    }
    if (!report.errors.empty()) {
        if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)) {
            out << "Validation errors:\n";
            for (const std::string& error : report.errors) {
                out << "- " << error << "\n";
            }
        } else {
            out << "- details: restricted\n";
        }
    }
    return out.str();
}

[[nodiscard]] std::string format_contradiction_budget_buckets(const ArchiveEngineState& state, AccessLevel access) {
    const ContradictionBudgetReport report = compute_contradiction_budget(state, access);
    std::ostringstream out;
    out << "ContradictionBudget buckets visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::ContradictionBudgetBucket)) {
        out << "- aggregate-only at this access level; detailed bucket IDs and diagnostic notes are restricted.\n";
        const auto archive_it = std::find_if(report.buckets.begin(), report.buckets.end(), [](const ContradictionBudgetBucket& bucket) {
            return bucket.id == "contradiction_budget.archive";
        });
        if (archive_it != report.buckets.end()) {
            out << "- archive_status: " << to_string(archive_it->status) << "\n";
            out << "- archive_severity: " << to_string(archive_it->severity) << "\n";
            out << "- contradiction_count: " << archive_it->contradiction_count << "\n";
        }
        return out.str();
    }
    for (const ContradictionBudgetBucket& bucket : report.buckets) {
        out << "- " << bucket.id << ": " << to_string(bucket.scope)
            << " status=" << to_string(bucket.status)
            << " severity=" << to_string(bucket.severity)
            << " contradictions=" << bucket.contradiction_count
            << " unresolved=" << bucket.unresolved_contradiction_count
            << " generation_bugs=" << bucket.generation_bug_count
            << " reasons=";
        if (bucket.reason_codes.empty()) {
            out << "none";
        } else {
            for (std::size_t index = 0; index < bucket.reason_codes.size(); ++index) {
                if (index > 0U) {
                    out << ",";
                }
                out << to_string(bucket.reason_codes[index]);
            }
        }
        out << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_contradiction_budget_bucket_detail(const ArchiveEngineState& state,
                                                                    AccessLevel access,
                                                                    const std::string& bucket_id) {
    const ContradictionBudgetReport report = compute_contradiction_budget(state, access);
    const auto it = std::find_if(report.buckets.begin(), report.buckets.end(), [&](const ContradictionBudgetBucket& bucket) {
        return bucket.id == bucket_id;
    });
    std::ostringstream out;
    out << "ContradictionBudget bucket:\n";
    if (it == report.buckets.end()) {
        out << "- found: false\n";
        return out.str();
    }
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::ContradictionBudgetBucket)) {
        if (!can_view_contradiction_budget_public_bucket_summary(access, bucket_id)) {
            out << "- found: false\n";
            return out.str();
        }
        out << "- found: true\n";
        out << "- scope: " << to_string(it->scope) << "\n";
        out << "- contradiction_count: " << it->contradiction_count << "\n";
        out << "- unresolved_contradiction_count: " << it->unresolved_contradiction_count << "\n";
        out << "- generation_bug_count: " << it->generation_bug_count << "\n";
        out << "- severity: " << to_string(it->severity) << "\n";
        out << "- status: " << to_string(it->status) << "\n";
        out << "- details: restricted\n";
        return out.str();
    }
    out << "- found: true\n";
    format_bucket_common(out, *it);
    format_policy_common(out, default_contradiction_budget_policy());
    if (!it->representative_contradiction_ids.empty()) {
        out << "- representative_contradiction_ids:";
        for (const std::string& id : it->representative_contradiction_ids) {
            out << " " << id;
        }
        out << "\n";
    }
    if (!it->notes.empty()) {
        out << "Notes:\n";
        for (const std::string& note : it->notes) {
            out << "- " << note << "\n";
        }
    }
    return out.str();
}

} // namespace archive
