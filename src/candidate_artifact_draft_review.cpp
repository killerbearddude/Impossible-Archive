#include "candidate_artifact_draft_review_api.h"
#include "candidate_artifact_draft_api.h"
#include "diagnostic_access_policy.h"

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <sstream>

namespace archive {
namespace {


[[nodiscard]] bool draft_has_disabled_boundary_violation(const CandidateArtifactDraft& draft) {
    return draft.current_artifact_insertion_enabled ||
           draft.current_public_claim_insertion_enabled ||
           draft.current_discovery_scheduling_enabled ||
           draft.hidden_truth_mutation_enabled ||
           draft.public_archive_mutation_enabled ||
           draft.persistence_enabled;
}

[[nodiscard]] bool review_has_disabled_boundary_violation(const CandidateArtifactDraftReview& review) {
    return review.current_artifact_insertion_enabled ||
           review.current_public_claim_insertion_enabled ||
           review.current_discovery_scheduling_enabled ||
           review.hidden_truth_mutation_enabled ||
           review.public_archive_mutation_enabled ||
           review.persistence_enabled ||
           review.final_artifact_prose_generation_enabled;
}

void add_reason(CandidateArtifactDraftReview& review, const std::string& code, const std::string& revision) {
    add_unique_string(review.reason_codes, code);
    if (!revision.empty()) {
        add_unique_string(review.required_revisions, revision);
    }
}

[[nodiscard]] std::vector<CandidateArtifactDraft> drafts_for_review(const ArchiveEngineState& state) {
    return state.candidate_artifact_drafts.empty()
        ? derive_candidate_artifact_drafts(state, AccessLevel::Curator).drafts
        : state.candidate_artifact_drafts;
}

void append_counts(std::ostringstream& out, const std::map<std::string, std::size_t>& counts) {
    for (const auto& [label, count] : counts) {
        out << "- " << label << ": " << count << "\n";
    }
}

} // namespace

[[nodiscard]] std::string to_string(CandidateArtifactDraftReviewDecision decision) {
    switch (decision) {
        case CandidateArtifactDraftReviewDecision::Pass: return "pass";
        case CandidateArtifactDraftReviewDecision::NeedsRevision: return "needs_revision";
        case CandidateArtifactDraftReviewDecision::NeedsCuratorReview: return "needs_curator_review";
        case CandidateArtifactDraftReviewDecision::Blocked: return "blocked";
        case CandidateArtifactDraftReviewDecision::Invalid: return "invalid";
    }
    return "unknown";
}

[[nodiscard]] CandidateArtifactDraftReview review_candidate_artifact_draft(const ArchiveEngineState& state, const CandidateArtifactDraft& draft, AccessLevel access) {
    (void)state;
    (void)access;
    CandidateArtifactDraftReview review;
    review.id = "candidate_artifact_draft_review." + draft.id;
    review.draft_id = draft.id;
    review.proposal_id = draft.proposal_id;
    review.audit_id = draft.audit_id;
    review.outline_completeness_score = clamp01((draft.outline_title.empty() ? 0.0 : 0.30) + (draft.claim_outline_lines.empty() ? 0.0 : 0.35) + (draft.required_validation_gates.empty() ? 0.0 : 0.35));
    review.traceability_score = clamp01((draft.proposal_id.empty() ? 0.0 : 0.25) + (draft.audit_id.empty() ? 0.0 : 0.25) + (draft.source_chain_ids.size() >= 2U ? 0.30 : 0.0) + (draft.source_evidence_potential_id.empty() ? 0.0 : 0.20));
    review.specificity_score = clamp01((draft.target_topic.empty() ? 0.0 : 0.25) + (draft.intended_creation_year == 0 ? 0.0 : 0.25) + (draft.intended_discovery_year == 0 ? 0.0 : 0.20) + (draft.outline_title.empty() ? 0.0 : 0.30));
    review.safety_score = draft_has_disabled_boundary_violation(draft) ? 0.0 : clamp01(1.0 - (draft.touches_protected_mystery ? 0.35 : 0.0) - (draft.public_safe_summary_lines.empty() ? 0.25 : 0.0));
    review.revision_pressure_score = clamp01((1.0 - review.outline_completeness_score) * 0.35 + (1.0 - review.traceability_score) * 0.25 + (1.0 - review.specificity_score) * 0.20 + (1.0 - review.safety_score) * 0.20);
    if (draft.outline_title.empty()) { add_reason(review, "missing_outline_title", "add an outline title"); }
    if (draft.claim_outline_lines.empty()) { add_reason(review, "missing_claim_outline_lines", "add claim outline lines"); }
    if (draft.required_validation_gates.empty()) { add_reason(review, "missing_validation_gates", "add required validation gates"); }
    if (draft.source_chain_ids.size() < 2U) { add_reason(review, "low_traceability", "complete proposal/audit source chain"); }
    if (review.specificity_score < 0.70) { add_reason(review, "low_specificity", "improve topic/year/title specificity"); }
    if (draft.touches_protected_mystery) { add_reason(review, "protected_mystery", "curator must review protected mystery handling"); }
    if (draft_has_disabled_boundary_violation(draft)) { add_reason(review, "disabled_boundary_violation", "disable all insertion/mutation/persistence flags"); }
    if (review.revision_pressure_score >= 0.25) { add_reason(review, "revision_pressure", "reduce revision pressure below review threshold"); }
    if (draft.status != CandidateArtifactDraftStatus::ReadyForOutline) { add_reason(review, "draft_not_ready", "resolve source draft status before review can pass"); }
    if (draft_has_disabled_boundary_violation(draft)) { review.decision = CandidateArtifactDraftReviewDecision::Invalid; }
    else if (draft.status == CandidateArtifactDraftStatus::Blocked || draft.status == CandidateArtifactDraftStatus::Invalid) { review.decision = CandidateArtifactDraftReviewDecision::Blocked; }
    else if (draft.status == CandidateArtifactDraftStatus::NeedsRevision || review.revision_pressure_score >= 0.25) { review.decision = CandidateArtifactDraftReviewDecision::NeedsRevision; }
    else if (draft.status == CandidateArtifactDraftStatus::NeedsCuratorReview || draft.touches_protected_mystery) { review.decision = CandidateArtifactDraftReviewDecision::NeedsCuratorReview; }
    else { review.decision = CandidateArtifactDraftReviewDecision::Pass; add_unique_string(review.reason_codes, "outline_review_passed"); }
    add_unique_string(review.public_safe_summary_lines, "Draft review only: " + draft.outline_title);
    add_unique_string(review.curator_notes, "derived from CandidateArtifactDraft; review pass does not authorize artifact generation or insertion");
    return review;
}

[[nodiscard]] CandidateArtifactDraftReviewReport review_candidate_artifact_drafts(const ArchiveEngineState& state, AccessLevel access) {
    CandidateArtifactDraftReviewReport report;
    const std::vector<CandidateArtifactDraft> drafts = drafts_for_review(state);
    for (const CandidateArtifactDraft& draft : drafts) {
        report.reviews.push_back(review_candidate_artifact_draft(state, draft, access));
    }
    std::sort(report.reviews.begin(), report.reviews.end(), [](const CandidateArtifactDraftReview& lhs, const CandidateArtifactDraftReview& rhs) { return lhs.id < rhs.id; });
    ArchiveEngineState validation_state = state;
    validation_state.candidate_artifact_drafts = drafts;
    validation_state.candidate_artifact_draft_reviews = report.reviews;
    report.errors = validate_candidate_artifact_draft_reviews(validation_state);
    return report;
}

void review_candidate_artifact_drafts_into_state(ArchiveEngineState& state, AccessLevel access) {
    (void)access;
    state.candidate_artifact_draft_reviews = review_candidate_artifact_drafts(state, AccessLevel::Curator).reviews;
}

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_draft_reviews(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> seen_ids;
    std::vector<std::string> draft_ids;
    for (const CandidateArtifactDraft& draft : state.candidate_artifact_drafts) { draft_ids.push_back(draft.id); }
    for (const CandidateArtifactDraftReview& review : state.candidate_artifact_draft_reviews) {
        const std::string label = review.id.empty() ? std::string{"<empty review id>"} : review.id;
        if (review.id.empty()) { errors.push_back("CandidateArtifactDraftReview has empty id"); }
        else if (!seen_ids.insert(review.id).second) { errors.push_back("CandidateArtifactDraftReview has duplicate id: " + review.id); }
        if (review.draft_id.empty() || std::find(draft_ids.begin(), draft_ids.end(), review.draft_id) == draft_ids.end()) { errors.push_back("CandidateArtifactDraftReview references missing draft: " + label); }
        if (review.proposal_id.empty()) { errors.push_back("CandidateArtifactDraftReview has empty proposal id: " + label); }
        if (review.audit_id.empty()) { errors.push_back("CandidateArtifactDraftReview has empty audit id: " + label); }
        for (const double score : std::array<double, 5>{review.outline_completeness_score, review.traceability_score, review.safety_score, review.specificity_score, review.revision_pressure_score}) {
            if (score < 0.0 || score > 1.0) { errors.push_back("CandidateArtifactDraftReview score out of range: " + label); }
        }
        if (review_has_disabled_boundary_violation(review)) { errors.push_back("CandidateArtifactDraftReview enables generation, insertion, mutation, or persistence: " + label); }
        if (review.decision != CandidateArtifactDraftReviewDecision::Pass && review.required_revisions.empty()) { errors.push_back("CandidateArtifactDraftReview non-pass decision lacks required revisions: " + label); }
        if (review.decision == CandidateArtifactDraftReviewDecision::Pass && review.revision_pressure_score >= 0.25) { errors.push_back("CandidateArtifactDraftReview pass has excessive revision pressure: " + label); }
        if (review.public_safe_summary_lines.empty()) { errors.push_back("CandidateArtifactDraftReview lacks public-safe summary: " + label); }
    }
    return errors;
}

[[nodiscard]] std::string format_candidate_artifact_draft_review_summary(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactDraftReviewReport report = review_candidate_artifact_drafts(state, access);
    std::map<std::string, std::size_t> by_decision;
    for (const CandidateArtifactDraftReview& review : report.reviews) { ++by_decision[to_string(review.decision)]; }
    std::ostringstream out;
    out << "CandidateArtifactDraftReview summary:\n";
    out << "- behavior: advisory review only; no Artifact insertion, PublicClaim insertion, discovery scheduling, hidden truth mutation, PublicArchive mutation, persistence, resolver/composition, or final artifact prose generation is introduced in v29.1.\n";
    out << "- total_reviews: " << report.reviews.size() << "\n";
    out << "- validation_errors: " << report.errors.size() << "\n";
    out << "- generation_insertion_mutation_or_persistence_enabled: 0\n";
    out << "Decision counts:\n";
    append_counts(out, by_decision);
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraftReview)) {
        out << "- details: aggregate-only at this access level; review IDs, scores, reason codes, and required revisions are restricted.\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_draft_review_validation(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactDraftReviewReport report = review_candidate_artifact_drafts(state, access);
    std::ostringstream out;
    out << "CandidateArtifactDraftReview validation:\n- result: " << (report.errors.empty() ? "passed" : "failed") << "\n- reviews: " << report.reviews.size() << "\n- errors: " << report.errors.size() << "\n";
    if (!report.errors.empty() && can_view_diagnostic_detail(access, DiagnosticDetailSurface::ValidationErrors)) {
        for (const std::string& error : report.errors) { out << "- " << error << "\n"; }
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_draft_review_list(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactDraftReviewReport report = review_candidate_artifact_drafts(state, access);
    std::ostringstream out;
    out << "CandidateArtifactDraftReviews visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraftReview)) {
        out << "- total_reviews: " << report.reviews.size() << "\n- details: restricted\n";
        return out.str();
    }
    for (const CandidateArtifactDraftReview& review : report.reviews) {
        out << "- " << review.id << ": draft_id=" << review.draft_id << " decision=" << to_string(review.decision) << " revision_pressure=" << review.revision_pressure_score << " mutation_enabled=false\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_draft_review_detail(const ArchiveEngineState& state, AccessLevel access, const std::string& review_id) {
    const CandidateArtifactDraftReviewReport report = review_candidate_artifact_drafts(state, access);
    const auto it = std::find_if(report.reviews.begin(), report.reviews.end(), [&](const CandidateArtifactDraftReview& review) { return review.id == review_id; });
    std::ostringstream out;
    out << "CandidateArtifactDraftReview detail:\n";
    if (it == report.reviews.end() || !can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraftReview)) {
        out << "- found: false\n";
        return out.str();
    }
    out << "- found: true\n- id: " << it->id << "\n- draft_id: " << it->draft_id << "\n- proposal_id: " << it->proposal_id << "\n- audit_id: " << it->audit_id << "\n- decision: " << to_string(it->decision) << "\n";
    out << "- outline_completeness_score: " << it->outline_completeness_score << "\n- traceability_score: " << it->traceability_score << "\n- safety_score: " << it->safety_score << "\n- specificity_score: " << it->specificity_score << "\n- revision_pressure_score: " << it->revision_pressure_score << "\n";
    out << "- current_artifact_insertion_enabled: false\n- current_public_claim_insertion_enabled: false\n- current_discovery_scheduling_enabled: false\n- hidden_truth_mutation_enabled: false\n- public_archive_mutation_enabled: false\n- persistence_enabled: false\n- final_artifact_prose_generation_enabled: false\n";
    for (const std::string& code : it->reason_codes) { out << "- reason_code: " << code << "\n"; }
    for (const std::string& revision : it->required_revisions) { out << "- required_revision: " << revision << "\n"; }
    return out.str();
}

} // namespace archive
