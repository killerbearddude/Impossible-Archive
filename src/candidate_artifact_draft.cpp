#include "candidate_artifact_draft_api.h"
#include "candidate_artifact_proposal_api.h"
#include "candidate_artifact_proposal_audit_api.h"
#include "diagnostic_access_policy.h"

#include <map>
#include <set>
#include <sstream>

namespace archive {
namespace {

[[nodiscard]] const CandidateArtifactProposalAudit* find_audit_for_proposal(const std::vector<CandidateArtifactProposalAudit>& audits,
                                                                            const std::string& proposal_id) {
    const auto it = std::find_if(audits.begin(), audits.end(), [&](const CandidateArtifactProposalAudit& audit) {
        return audit.proposal_id == proposal_id;
    });
    return it == audits.end() ? nullptr : &*it;
}

[[nodiscard]] std::vector<CandidateArtifactProposal> proposals_for_drafts(const ArchiveEngineState& state, AccessLevel access) {
    if (!state.candidate_artifact_proposals.empty()) {
        return state.candidate_artifact_proposals;
    }
    return draft_candidate_artifact_proposals(state, access).proposals;
}

[[nodiscard]] std::vector<CandidateArtifactProposalAudit> audits_for_drafts(const ArchiveEngineState& state, AccessLevel access) {
    if (!state.candidate_artifact_proposal_audits.empty()) {
        return state.candidate_artifact_proposal_audits;
    }
    return audit_candidate_artifact_proposals(state, access).audits;
}

[[nodiscard]] CandidateArtifactDraftStatus status_for(const CandidateArtifactProposal& proposal,
                                                      const CandidateArtifactProposalAudit* audit) {
    if (audit == nullptr) {
        return CandidateArtifactDraftStatus::Invalid;
    }
    if (proposal.decision == CandidateArtifactProposalDecision::Invalid ||
        audit->decision == CandidateArtifactProposalAuditDecision::Invalid) {
        return CandidateArtifactDraftStatus::Invalid;
    }
    if (proposal.decision == CandidateArtifactProposalDecision::Blocked ||
        audit->decision == CandidateArtifactProposalAuditDecision::Blocked) {
        return CandidateArtifactDraftStatus::Blocked;
    }
    if (audit->decision == CandidateArtifactProposalAuditDecision::NeedsRevision) {
        return CandidateArtifactDraftStatus::NeedsRevision;
    }
    if (proposal.decision == CandidateArtifactProposalDecision::NeedsCuratorReview ||
        audit->decision == CandidateArtifactProposalAuditDecision::NeedsCuratorReview) {
        return CandidateArtifactDraftStatus::NeedsCuratorReview;
    }
    if (proposal.decision == CandidateArtifactProposalDecision::Draftable &&
        audit->decision == CandidateArtifactProposalAuditDecision::Pass) {
        return CandidateArtifactDraftStatus::ReadyForOutline;
    }
    return CandidateArtifactDraftStatus::NeedsCuratorReview;
}

[[nodiscard]] CandidateArtifactDraftVisibilityClass visibility_for(const CandidateArtifactProposal& proposal) {
    switch (proposal.visibility_class) {
        case CandidateArtifactProposalVisibilityClass::PublicEligible:
            return CandidateArtifactDraftVisibilityClass::PublicSummary;
        case CandidateArtifactProposalVisibilityClass::ScholarEligible:
            return CandidateArtifactDraftVisibilityClass::ScholarSummary;
        case CandidateArtifactProposalVisibilityClass::DebugOnly:
            return CandidateArtifactDraftVisibilityClass::DebugOnly;
        case CandidateArtifactProposalVisibilityClass::CuratorOnly:
            break;
    }
    return CandidateArtifactDraftVisibilityClass::CuratorOnly;
}

void append_counts(std::ostringstream& out, const std::map<std::string, std::size_t>& counts) {
    for (const auto& [label, count] : counts) {
        out << "- " << label << ": " << count << "\n";
    }
}

[[nodiscard]] bool id_in(const std::vector<std::string>& values, const std::string& id) {
    return std::find(values.begin(), values.end(), id) != values.end();
}

} // namespace

[[nodiscard]] std::string to_string(CandidateArtifactDraftStatus status) {
    switch (status) {
        case CandidateArtifactDraftStatus::ReadyForOutline: return "ready_for_outline";
        case CandidateArtifactDraftStatus::NeedsRevision: return "needs_revision";
        case CandidateArtifactDraftStatus::NeedsCuratorReview: return "needs_curator_review";
        case CandidateArtifactDraftStatus::Blocked: return "blocked";
        case CandidateArtifactDraftStatus::Invalid: return "invalid";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(CandidateArtifactDraftVisibilityClass visibility_class) {
    switch (visibility_class) {
        case CandidateArtifactDraftVisibilityClass::PublicSummary: return "public_summary";
        case CandidateArtifactDraftVisibilityClass::ScholarSummary: return "scholar_summary";
        case CandidateArtifactDraftVisibilityClass::CuratorOnly: return "curator_only";
        case CandidateArtifactDraftVisibilityClass::DebugOnly: return "debug_only";
    }
    return "unknown";
}

[[nodiscard]] bool candidate_artifact_draft_visible_to(const CandidateArtifactDraft& draft, AccessLevel access) {
    if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraft)) {
        return true;
    }
    if (draft.visibility_class == CandidateArtifactDraftVisibilityClass::PublicSummary) {
        return true;
    }
    return draft.visibility_class == CandidateArtifactDraftVisibilityClass::ScholarSummary && can_view(access, AccessLevel::Scholar);
}

[[nodiscard]] CandidateArtifactDraft derive_candidate_artifact_draft(
    const ArchiveEngineState& state,
    const CandidateArtifactProposal& proposal,
    const CandidateArtifactProposalAudit* audit,
    AccessLevel access
) {
    (void)state;
    (void)access;

    CandidateArtifactDraft draft;
    draft.id = "candidate_artifact_draft." + proposal.id;
    draft.proposal_id = proposal.id;
    draft.audit_id = audit == nullptr ? std::string{} : audit->id;
    draft.plan_id = proposal.plan_id;
    draft.evaluation_id = proposal.evaluation_id;
    draft.source_evidence_potential_id = proposal.source_evidence_potential_id;

    draft.status = status_for(proposal, audit);
    draft.visibility_class = visibility_for(proposal);
    draft.intended_artifact_type = proposal.proposed_artifact_type;
    draft.intended_voice_register = proposal.proposed_voice_register;
    draft.outline_title = proposal.proposed_title.empty()
        ? "Candidate artifact draft outline"
        : proposal.proposed_title;
    draft.target_topic = proposal.target_topic;
    draft.intended_creation_year = proposal.proposed_creation_year;
    draft.intended_discovery_year = proposal.proposed_discovery_year;

    add_unique_string(draft.source_chain_ids, proposal.id);
    if (audit != nullptr) { add_unique_string(draft.source_chain_ids, audit->id); }
    if (!proposal.plan_id.empty()) { add_unique_string(draft.source_chain_ids, proposal.plan_id); }
    if (!proposal.evaluation_id.empty()) { add_unique_string(draft.source_chain_ids, proposal.evaluation_id); }

    for (const std::string& skeleton : proposal.proposed_claim_skeletons) {
        add_unique_string(draft.claim_outline_lines, skeleton + " -> outline line only; no PublicClaim insertion");
    }
    if (draft.claim_outline_lines.empty()) {
        add_unique_string(draft.claim_outline_lines, "outline placeholder only; no PublicClaim insertion");
    }

    for (const std::string& gate : proposal.proposed_validation_gates) {
        add_unique_string(draft.required_validation_gates, gate);
    }
    if (audit != nullptr) {
        for (const std::string& revision : audit->required_revisions) {
            add_unique_string(draft.required_validation_gates, "required_revision: " + revision);
        }
    }
    add_unique_string(draft.required_validation_gates, "draft outline remains non-mutating in v29.0");
    add_unique_string(draft.required_validation_gates, "artifact insertion disabled");
    add_unique_string(draft.required_validation_gates, "PublicClaim insertion disabled");
    add_unique_string(draft.required_validation_gates, "discovery scheduling disabled");

    add_unique_string(draft.public_safe_summary_lines, "Draft outline only: " + draft.outline_title);
    add_unique_string(draft.public_safe_summary_lines, "No artifact, public claim, discovery, hidden truth, or public archive mutation is enabled.");

    draft.contains_hidden_source_reference = !proposal.source_evidence_potential_id.empty();
    draft.contains_curator_diagnostics = proposal.contains_curator_diagnostics ||
                                         audit == nullptr ||
                                         (audit != nullptr && !audit->findings.empty());
    draft.touches_protected_mystery = proposal.touches_protected_mystery;
    add_unique_string(draft.curator_notes, "derived from proposal/audit chain; source EvidencePotential id is restricted below curator/debug access");
    if (audit == nullptr) {
        add_unique_string(draft.warnings, "missing CandidateArtifactProposalAudit for proposal " + proposal.id);
    }
    if (draft.status == CandidateArtifactDraftStatus::ReadyForOutline && draft.visibility_class == CandidateArtifactDraftVisibilityClass::CuratorOnly) {
        draft.status = CandidateArtifactDraftStatus::NeedsCuratorReview;
        add_unique_string(draft.warnings, "ready outline downgraded to curator review because visibility is curator-only");
    }

    draft.current_artifact_insertion_enabled = false;
    draft.current_public_claim_insertion_enabled = false;
    draft.current_discovery_scheduling_enabled = false;
    draft.hidden_truth_mutation_enabled = false;
    draft.public_archive_mutation_enabled = false;
    draft.persistence_enabled = false;
    return draft;
}

[[nodiscard]] CandidateArtifactDraftReport derive_candidate_artifact_drafts(const ArchiveEngineState& state, AccessLevel access) {
    CandidateArtifactDraftReport report;
    const std::vector<CandidateArtifactProposal> proposals = proposals_for_drafts(state, AccessLevel::Curator);
    const std::vector<CandidateArtifactProposalAudit> audits = audits_for_drafts(state, AccessLevel::Curator);

    for (const CandidateArtifactProposal& proposal : proposals) {
        const CandidateArtifactProposalAudit* audit = find_audit_for_proposal(audits, proposal.id);
        report.drafts.push_back(derive_candidate_artifact_draft(state, proposal, audit, access));
    }

    std::sort(report.drafts.begin(), report.drafts.end(), [](const CandidateArtifactDraft& lhs, const CandidateArtifactDraft& rhs) {
        return lhs.id < rhs.id;
    });

    ArchiveEngineState validation_state = state;
    validation_state.candidate_artifact_proposals = proposals;
    validation_state.candidate_artifact_proposal_audits = audits;
    validation_state.candidate_artifact_drafts = report.drafts;
    report.errors = validate_candidate_artifact_drafts(validation_state);
    return report;
}

void derive_candidate_artifact_drafts_into_state(ArchiveEngineState& state, AccessLevel access) {
    (void)access;
    state.candidate_artifact_drafts = derive_candidate_artifact_drafts(state, AccessLevel::Curator).drafts;
}

[[nodiscard]] std::vector<std::string> validate_candidate_artifact_drafts(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> seen_ids;
    std::vector<std::string> proposal_ids;
    std::vector<std::string> audit_ids;

    for (const CandidateArtifactProposal& proposal : state.candidate_artifact_proposals) {
        proposal_ids.push_back(proposal.id);
    }
    for (const CandidateArtifactProposalAudit& audit : state.candidate_artifact_proposal_audits) {
        audit_ids.push_back(audit.id);
    }

    for (const CandidateArtifactDraft& draft : state.candidate_artifact_drafts) {
        const std::string label = draft.id.empty() ? std::string{"<empty draft id>"} : draft.id;
        if (draft.id.empty()) {
            errors.push_back("CandidateArtifactDraft has empty id");
        } else if (!seen_ids.insert(draft.id).second) {
            errors.push_back("CandidateArtifactDraft has duplicate id: " + draft.id);
        }
        if (draft.proposal_id.empty()) {
            errors.push_back("CandidateArtifactDraft has empty proposal id: " + label);
        } else if (!id_in(proposal_ids, draft.proposal_id)) {
            errors.push_back("CandidateArtifactDraft references missing proposal: " + label + " -> " + draft.proposal_id);
        }
        if (draft.audit_id.empty()) {
            errors.push_back("CandidateArtifactDraft has empty audit id: " + label);
        } else if (!id_in(audit_ids, draft.audit_id)) {
            errors.push_back("CandidateArtifactDraft references missing audit: " + label + " -> " + draft.audit_id);
        }
        if (draft.outline_title.empty()) {
            errors.push_back("CandidateArtifactDraft has empty outline title: " + label);
        }
        if (draft.source_chain_ids.empty()) {
            errors.push_back("CandidateArtifactDraft has empty source chain: " + label);
        }
        if (draft.claim_outline_lines.empty()) {
            errors.push_back("CandidateArtifactDraft has no claim outline lines: " + label);
        }
        if (draft.required_validation_gates.empty()) {
            errors.push_back("CandidateArtifactDraft has no validation gates: " + label);
        }
        if (draft.current_artifact_insertion_enabled) {
            errors.push_back("CandidateArtifactDraft enables Artifact insertion: " + label);
        }
        if (draft.current_public_claim_insertion_enabled) {
            errors.push_back("CandidateArtifactDraft enables PublicClaim insertion: " + label);
        }
        if (draft.current_discovery_scheduling_enabled) {
            errors.push_back("CandidateArtifactDraft enables discovery scheduling: " + label);
        }
        if (draft.hidden_truth_mutation_enabled) {
            errors.push_back("CandidateArtifactDraft enables hidden truth mutation: " + label);
        }
        if (draft.public_archive_mutation_enabled) {
            errors.push_back("CandidateArtifactDraft enables public archive mutation: " + label);
        }
        if (draft.persistence_enabled) {
            errors.push_back("CandidateArtifactDraft enables persistence: " + label);
        }
        if ((draft.visibility_class == CandidateArtifactDraftVisibilityClass::PublicSummary ||
             draft.visibility_class == CandidateArtifactDraftVisibilityClass::ScholarSummary) &&
            draft.public_safe_summary_lines.empty()) {
            errors.push_back("CandidateArtifactDraft public/scholar-visible draft lacks public-safe summary: " + label);
        }
    }

    return errors;
}

[[nodiscard]] std::string format_candidate_artifact_draft_summary(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactDraftReport report = derive_candidate_artifact_drafts(state, access);
    std::map<std::string, std::size_t> by_status;
    std::map<std::string, std::size_t> by_visibility;
    std::size_t insertion_enabled = 0;
    std::size_t public_claim_enabled = 0;
    std::size_t discovery_enabled = 0;
    std::size_t hidden_mutation_enabled = 0;
    std::size_t archive_mutation_enabled = 0;
    std::size_t persistence_enabled = 0;

    for (const CandidateArtifactDraft& draft : report.drafts) {
        ++by_status[to_string(draft.status)];
        ++by_visibility[to_string(draft.visibility_class)];
        if (draft.current_artifact_insertion_enabled) { ++insertion_enabled; }
        if (draft.current_public_claim_insertion_enabled) { ++public_claim_enabled; }
        if (draft.current_discovery_scheduling_enabled) { ++discovery_enabled; }
        if (draft.hidden_truth_mutation_enabled) { ++hidden_mutation_enabled; }
        if (draft.public_archive_mutation_enabled) { ++archive_mutation_enabled; }
        if (draft.persistence_enabled) { ++persistence_enabled; }
    }

    std::ostringstream out;
    out << "CandidateArtifactDraft summary:\n";
    out << "- behavior: draft outline only; no Artifact insertion, PublicClaim insertion, discovery scheduling, hidden truth mutation, PublicArchive mutation, persistence, resolver/composition, or final artifact prose generation is introduced in v29.0.\n";
    out << "- total_drafts: " << report.drafts.size() << "\n";
    out << "- validation_errors: " << report.errors.size() << "\n";
    out << "- current_artifact_insertion_enabled: " << insertion_enabled << "\n";
    out << "- current_public_claim_insertion_enabled: " << public_claim_enabled << "\n";
    out << "- current_discovery_scheduling_enabled: " << discovery_enabled << "\n";
    out << "- hidden_truth_mutation_enabled: " << hidden_mutation_enabled << "\n";
    out << "- public_archive_mutation_enabled: " << archive_mutation_enabled << "\n";
    out << "- persistence_enabled: " << persistence_enabled << "\n";
    out << "Status counts:\n";
    append_counts(out, by_status);
    out << "Visibility counts:\n";
    append_counts(out, by_visibility);
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraft)) {
        out << "- details: aggregate-only at this access level; proposal/audit IDs, source chains, validation gates, and curator notes are restricted.\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_draft_validation(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactDraftReport report = derive_candidate_artifact_drafts(state, access);
    std::ostringstream out;
    out << "CandidateArtifactDraft validation:\n";
    out << "- result: " << (report.errors.empty() ? "passed" : "failed") << "\n";
    out << "- drafts: " << report.drafts.size() << "\n";
    out << "- errors: " << report.errors.size() << "\n";
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

[[nodiscard]] std::string format_candidate_artifact_draft_list(const ArchiveEngineState& state, AccessLevel access) {
    const CandidateArtifactDraftReport report = derive_candidate_artifact_drafts(state, access);
    std::ostringstream out;
    out << "CandidateArtifactDrafts visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraft)) {
        std::size_t visible = 0;
        for (const CandidateArtifactDraft& draft : report.drafts) {
            if (candidate_artifact_draft_visible_to(draft, access)) { ++visible; }
        }
        out << "- visible_drafts: " << visible << "\n";
        out << "- details: public/scholar output is summary-only; source chains, validation gates, proposal/audit IDs, and curator notes are restricted.\n";
        for (const CandidateArtifactDraft& draft : report.drafts) {
            if (!candidate_artifact_draft_visible_to(draft, access)) { continue; }
            out << "- " << draft.outline_title
                << ": status=" << to_string(draft.status)
                << " visibility=" << to_string(draft.visibility_class)
                << " mutation_enabled=false\n";
        }
        return out.str();
    }

    for (const CandidateArtifactDraft& draft : report.drafts) {
        out << "- " << draft.id
            << ": proposal_id=" << draft.proposal_id
            << " audit_id=" << draft.audit_id
            << " status=" << to_string(draft.status)
            << " visibility=" << to_string(draft.visibility_class)
            << " outline_lines=" << draft.claim_outline_lines.size()
            << " validation_gates=" << draft.required_validation_gates.size()
            << " mutation_enabled=false\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_candidate_artifact_draft_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& draft_id
) {
    const CandidateArtifactDraftReport report = derive_candidate_artifact_drafts(state, access);
    const auto it = std::find_if(report.drafts.begin(), report.drafts.end(), [&](const CandidateArtifactDraft& draft) {
        return draft.id == draft_id;
    });
    std::ostringstream out;
    out << "CandidateArtifactDraft detail:\n";
    if (it == report.drafts.end() || !candidate_artifact_draft_visible_to(*it, access)) {
        out << "- found: false\n";
        return out.str();
    }
    out << "- found: true\n";
    out << "- outline_title: " << it->outline_title << "\n";
    out << "- status: " << to_string(it->status) << "\n";
    out << "- intended_artifact_type: " << to_string(it->intended_artifact_type) << "\n";
    out << "- intended_voice_register: " << to_string(it->intended_voice_register) << "\n";
    for (const std::string& line : it->public_safe_summary_lines) {
        out << "- public_summary: " << line << "\n";
    }
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::CandidateArtifactDraft)) {
        out << "- details: restricted\n";
        return out.str();
    }
    out << "- id: " << it->id << "\n";
    out << "- proposal_id: " << it->proposal_id << "\n";
    out << "- audit_id: " << it->audit_id << "\n";
    out << "- plan_id: " << it->plan_id << "\n";
    out << "- evaluation_id: " << it->evaluation_id << "\n";
    out << "- source_evidence_potential_id: " << it->source_evidence_potential_id << "\n";
    out << "- intended_creation_year: " << it->intended_creation_year << "\n";
    out << "- intended_discovery_year: " << it->intended_discovery_year << "\n";
    out << "- current_artifact_insertion_enabled: " << (it->current_artifact_insertion_enabled ? "true" : "false") << "\n";
    out << "- current_public_claim_insertion_enabled: " << (it->current_public_claim_insertion_enabled ? "true" : "false") << "\n";
    out << "- current_discovery_scheduling_enabled: " << (it->current_discovery_scheduling_enabled ? "true" : "false") << "\n";
    out << "- hidden_truth_mutation_enabled: " << (it->hidden_truth_mutation_enabled ? "true" : "false") << "\n";
    out << "- public_archive_mutation_enabled: " << (it->public_archive_mutation_enabled ? "true" : "false") << "\n";
    out << "- persistence_enabled: " << (it->persistence_enabled ? "true" : "false") << "\n";
    auto dump = [&](std::string_view label, const std::vector<std::string>& values) {
        if (values.empty()) { return; }
        out << label << ":\n";
        for (const std::string& value : values) {
            out << "- " << value << "\n";
        }
    };
    dump("Source chain IDs", it->source_chain_ids);
    dump("Claim outline lines", it->claim_outline_lines);
    dump("Required validation gates", it->required_validation_gates);
    dump("Curator notes", it->curator_notes);
    dump("Warnings", it->warnings);
    return out.str();
}

} // namespace archive
