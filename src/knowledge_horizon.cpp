#include "knowledge_horizon_api.h"
#include "archive_views_api.h"
#include "diagnostic_access_policy.h"

#include <iomanip>
#include <sstream>

namespace archive {
namespace {

struct SubjectAvailability {
    bool exists = false;
    int earliest_year = 0;
    AccessLevel min_access = AccessLevel::Public;
    bool hidden_only = false;
};

[[nodiscard]] std::string lower_copy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

[[nodiscard]] const Artifact* artifact_for_context(const ArchiveEngineState& state, const std::string& context_id) {
    return state.public_archive.find_artifact(context_id);
}

[[nodiscard]] bool has_modifier(const Artifact& artifact, EvidenceModifier modifier) {
    return std::find(artifact.evidence_modifiers.begin(), artifact.evidence_modifiers.end(), modifier) != artifact.evidence_modifiers.end();
}

[[nodiscard]] bool has_context_mediation(const ArchiveEngineState& state,
                                         KnowledgeContextType context_type,
                                         const std::string& context_id,
                                         KnowledgeHorizonStatus& mediated_status,
                                         std::vector<std::string>& mediating_artifact_ids) {
    if (context_type != KnowledgeContextType::ArtifactCreator &&
        context_type != KnowledgeContextType::PublicArchive &&
        context_type != KnowledgeContextType::Interpreter) {
        return false;
    }

    const Artifact* artifact = artifact_for_context(state, context_id);
    if (artifact == nullptr && context_type == KnowledgeContextType::Interpreter) {
        // Interpreter findings use a theory as context. Mediation is handled at
        // the cited artifact level, so a missing artifact context here is not a
        // mediation path.
        return false;
    }
    if (artifact == nullptr) {
        return false;
    }

    mediating_artifact_ids.push_back(artifact->id);
    if (has_modifier(*artifact, EvidenceModifier::LaterCopy) || has_modifier(*artifact, EvidenceModifier::Interpolation)) {
        mediated_status = KnowledgeHorizonStatus::ValidByLaterCopy;
        return true;
    }
    if (has_modifier(*artifact, EvidenceModifier::Forgery)) {
        mediated_status = KnowledgeHorizonStatus::ValidByForgery;
        return true;
    }
    if (has_modifier(*artifact, EvidenceModifier::RitualAnachronism) || has_modifier(*artifact, EvidenceModifier::MythicCompression)) {
        mediated_status = KnowledgeHorizonStatus::ValidByRitualConvention;
        return true;
    }
    if (!artifact->transmission_history.empty()) {
        const std::string transmission = lower_copy(artifact->transmission_history);
        if (contains_substr(transmission, "copy") || contains_substr(transmission, "copied") || contains_substr(transmission, "transmission")) {
            mediated_status = KnowledgeHorizonStatus::ValidByTransmission;
            return true;
        }
    }
    return false;
}

[[nodiscard]] SubjectAvailability subject_availability(const ArchiveEngineState& state,
                                                       KnowledgeSubjectType subject_type,
                                                       const std::string& subject_id) {
    SubjectAvailability availability;
    switch (subject_type) {
        case KnowledgeSubjectType::HiddenEntity: {
            const Entity* entity = state.hidden_truth.find_entity(subject_id);
            if (entity == nullptr) {
                return availability;
            }
            availability.exists = true;
            availability.earliest_year = entity->existence_interval.start_year;
            availability.min_access = entity->min_access;
            availability.hidden_only = entity->min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::Office: {
            const Entity* entity = state.hidden_truth.find_entity(subject_id);
            if (entity == nullptr || entity->type != EntityType::Office) {
                return availability;
            }
            availability.exists = true;
            availability.earliest_year = entity->existence_interval.start_year;
            availability.min_access = entity->min_access;
            availability.hidden_only = entity->min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::Technology: {
            const Entity* entity = state.hidden_truth.find_entity(subject_id);
            if (entity == nullptr || entity->type != EntityType::Technology) {
                return availability;
            }
            availability.exists = true;
            availability.earliest_year = entity->existence_interval.start_year;
            availability.min_access = entity->min_access;
            availability.hidden_only = entity->min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::Script: {
            const Entity* entity = state.hidden_truth.find_entity(subject_id);
            if (entity == nullptr || entity->type != EntityType::Script) {
                return availability;
            }
            availability.exists = true;
            availability.earliest_year = entity->existence_interval.start_year;
            availability.min_access = entity->min_access;
            availability.hidden_only = entity->min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::PlaceName: {
            const Entity* entity = state.hidden_truth.find_entity(subject_id);
            if (entity == nullptr || (entity->type != EntityType::Site && entity->type != EntityType::Settlement)) {
                return availability;
            }
            availability.exists = true;
            availability.earliest_year = entity->existence_interval.start_year;
            availability.min_access = entity->min_access;
            availability.hidden_only = entity->min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::HiddenEvent: {
            const Event* event = state.hidden_truth.find_event(subject_id);
            if (event == nullptr) {
                return availability;
            }
            availability.exists = true;
            availability.earliest_year = event->start_year;
            availability.min_access = event->min_access;
            availability.hidden_only = event->min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::PublicArtifact: {
            const Artifact* artifact = state.public_archive.find_artifact(subject_id);
            if (artifact == nullptr) {
                return availability;
            }
            availability.exists = true;
            availability.earliest_year = artifact->discovery_year;
            availability.min_access = artifact->min_access;
            availability.hidden_only = artifact->min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::PublicClaim: {
            const Claim* claim = state.public_archive.find_claim(subject_id);
            if (claim == nullptr) {
                return availability;
            }
            const Artifact* artifact = state.public_archive.find_artifact(claim->source_artifact_id);
            availability.exists = true;
            availability.earliest_year = artifact == nullptr ? 0 : artifact->discovery_year;
            availability.min_access = claim->min_access;
            if (artifact != nullptr && artifact->min_access > availability.min_access) {
                availability.min_access = artifact->min_access;
            }
            availability.hidden_only = availability.min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::Mystery: {
            const auto it = std::find_if(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
                return mystery.id == subject_id;
            });
            if (it == state.mysteries.end()) {
                return availability;
            }
            availability.exists = true;
            availability.earliest_year = 0;
            availability.min_access = it->min_access;
            availability.hidden_only = it->min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::EvidencePotential: {
            const auto it = std::find_if(state.evidence_potentials.begin(), state.evidence_potentials.end(), [&](const EvidencePotential& potential) {
                return potential.id == subject_id;
            });
            if (it == state.evidence_potentials.end()) {
                return availability;
            }
            availability.exists = true;
            availability.earliest_year = it->earliest_possible_year;
            availability.min_access = it->min_access;
            availability.hidden_only = it->min_access > AccessLevel::Scholar;
            return availability;
        }
        case KnowledgeSubjectType::CivilizationSpec:
            availability.exists = state.civilization_spec_count > 0U || !state.civilization_source.has_value() || subject_id.empty();
            availability.earliest_year = 0;
            availability.min_access = AccessLevel::Public;
            availability.hidden_only = false;
            return availability;
        case KnowledgeSubjectType::CivilizationFragment:
            availability.exists = false;
            availability.earliest_year = 0;
            availability.min_access = AccessLevel::Curator;
            availability.hidden_only = true;
            return availability;
        case KnowledgeSubjectType::Term:
            availability.exists = !subject_id.empty();
            availability.earliest_year = 0;
            availability.min_access = AccessLevel::Public;
            availability.hidden_only = false;
            return availability;
    }
    return availability;
}

[[nodiscard]] bool context_should_enforce_viewer_access(KnowledgeContextType context_type) {
    return context_type == KnowledgeContextType::PublicArchive ||
           context_type == KnowledgeContextType::Interpreter ||
           context_type == KnowledgeContextType::SnapshotValidation ||
           context_type == KnowledgeContextType::CuratorAudit;
}

[[nodiscard]] KnowledgeHorizonFinding make_finding(KnowledgeContextType context_type,
                                                   std::string context_id,
                                                   KnowledgeSubjectType subject_type,
                                                   std::string subject_id,
                                                   int context_year) {
    KnowledgeHorizonFinding finding;
    finding.context_type = context_type;
    finding.context_id = std::move(context_id);
    finding.subject_type = subject_type;
    finding.subject_id = std::move(subject_id);
    finding.context_year = context_year;
    return finding;
}

void add_finding(KnowledgeHorizonReport& report, KnowledgeHorizonFinding finding) {
    if (finding.id.empty()) {
        std::ostringstream id;
        id << "knowledge_horizon." << std::setfill('0') << std::setw(4) << report.findings.size();
        finding.id = id.str();
    }
    if (is_invalid(finding.status)) {
        std::ostringstream error;
        error << finding.id << " " << to_string(finding.status) << " context=" << to_string(finding.context_type)
              << ":" << finding.context_id << " subject=" << to_string(finding.subject_type) << ":" << finding.subject_id;
        report.errors.push_back(error.str());
    }
    report.findings.push_back(std::move(finding));
}

void evaluate_and_add(KnowledgeHorizonReport& report,
                      const ArchiveEngineState& state,
                      KnowledgeContextType context_type,
                      const std::string& context_id,
                      KnowledgeSubjectType subject_type,
                      const std::string& subject_id,
                      int context_year,
                      AccessLevel access) {
    if (subject_id.empty()) {
        return;
    }
    add_finding(report, evaluate_knowledge_reference(state, context_type, context_id, subject_type, subject_id, context_year, access));
}

void validate_artifact_creator_knowledge(KnowledgeHorizonReport& report, const ArchiveEngineState& state, AccessLevel access) {
    for (const auto& [artifact_id, artifact] : state.public_archive.artifacts()) {
        const int context_year = artifact.true_creation_year;
        evaluate_and_add(report, state, KnowledgeContextType::ArtifactCreator, artifact_id, KnowledgeSubjectType::HiddenEntity, artifact.creator_id, context_year, access);
        evaluate_and_add(report, state, KnowledgeContextType::ArtifactCreator, artifact_id, KnowledgeSubjectType::PlaceName, artifact.location_created, context_year, access);
        evaluate_and_add(report, state, KnowledgeContextType::ArtifactCreator, artifact_id, KnowledgeSubjectType::PlaceName, artifact.location_found, context_year, access);
        evaluate_and_add(report, state, KnowledgeContextType::ArtifactCreator, artifact_id, KnowledgeSubjectType::Script, artifact.script_id, context_year, access);
        evaluate_and_add(report, state, KnowledgeContextType::ArtifactCreator, artifact_id, KnowledgeSubjectType::HiddenEntity, artifact.language_id, context_year, access);
        evaluate_and_add(report, state, KnowledgeContextType::ArtifactCreator, artifact_id, KnowledgeSubjectType::HiddenEntity, artifact.dialect_id, context_year, access);
        for (const std::string& entity_id : artifact.referenced_entity_ids) {
            evaluate_and_add(report, state, KnowledgeContextType::ArtifactCreator, artifact_id, KnowledgeSubjectType::HiddenEntity, entity_id, context_year, access);
        }
        for (const std::string& event_id : artifact.hidden_event_links) {
            evaluate_and_add(report, state, KnowledgeContextType::ArtifactCreator, artifact_id, KnowledgeSubjectType::HiddenEvent, event_id, context_year, access);
        }
    }
}

void validate_theory_knowledge(KnowledgeHorizonReport& report, const ArchiveEngineState& state, AccessLevel access) {
    const int archive_year = kOpenEndedYear;
    for (const Theory& theory : build_theories(state, archive_year)) {
        if (!can_view(access, theory.min_access)) {
            continue;
        }
        for (const EvidenceCitation& citation : theory.supporting_evidence) {
            evaluate_and_add(report, state, KnowledgeContextType::Interpreter, theory.id, KnowledgeSubjectType::PublicArtifact, citation.artifact_id, archive_year, access);
            if (citation.claim_id.has_value()) {
                evaluate_and_add(report, state, KnowledgeContextType::Interpreter, theory.id, KnowledgeSubjectType::PublicClaim, *citation.claim_id, archive_year, access);
            }
        }
    }
}

[[nodiscard]] bool potential_source_is_hidden_only(const ArchiveEngineState& state, const EvidencePotential& potential) {
    switch (potential.source_type) {
        case EvidencePotentialSourceType::HiddenEvent: {
            const Event* event = state.hidden_truth.find_event(potential.source_id);
            return event != nullptr && event->min_access > AccessLevel::Scholar;
        }
        case EvidencePotentialSourceType::HiddenEntity:
        case EvidencePotentialSourceType::Site:
        case EvidencePotentialSourceType::Office: {
            const Entity* entity = state.hidden_truth.find_entity(potential.source_id);
            return entity != nullptr && entity->min_access > AccessLevel::Scholar;
        }
        case EvidencePotentialSourceType::Mystery: {
            const auto it = std::find_if(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
                return mystery.id == potential.source_id;
            });
            return it != state.mysteries.end() && it->min_access > AccessLevel::Scholar;
        }
    }
    return false;
}

void validate_evidence_potential_knowledge(KnowledgeHorizonReport& report, const ArchiveEngineState& state, AccessLevel access) {
    for (const EvidencePotential& potential : state.evidence_potentials) {
        evaluate_and_add(report, state, KnowledgeContextType::EvidencePotentialDerivation, potential.id, KnowledgeSubjectType::EvidencePotential, potential.id, potential.earliest_possible_year, AccessLevel::Curator);
        KnowledgeSubjectType source_subject = KnowledgeSubjectType::HiddenEntity;
        switch (potential.source_type) {
            case EvidencePotentialSourceType::HiddenEvent: source_subject = KnowledgeSubjectType::HiddenEvent; break;
            case EvidencePotentialSourceType::HiddenEntity: source_subject = KnowledgeSubjectType::HiddenEntity; break;
            case EvidencePotentialSourceType::Site: source_subject = KnowledgeSubjectType::PlaceName; break;
            case EvidencePotentialSourceType::Office: source_subject = KnowledgeSubjectType::Office; break;
            case EvidencePotentialSourceType::Mystery: source_subject = KnowledgeSubjectType::Mystery; break;
        }
        evaluate_and_add(report, state, KnowledgeContextType::EvidencePotentialDerivation, potential.id, source_subject, potential.source_id, potential.earliest_possible_year, AccessLevel::Curator);
        if (potential.public_safe && potential_source_is_hidden_only(state, potential)) {
            KnowledgeHorizonFinding finding = make_finding(
                KnowledgeContextType::EvidencePotentialDerivation,
                potential.id,
                source_subject,
                potential.source_id,
                potential.earliest_possible_year
            );
            finding.earliest_available_year = potential.earliest_possible_year;
            finding.status = KnowledgeHorizonStatus::InvalidAccessLeak;
            finding.explanation = "EvidencePotential is marked public_safe even though its source remains hidden-only.";
            add_finding(report, std::move(finding));
        }
        (void)access;
    }
}

[[nodiscard]] bool finding_visible_to(const KnowledgeHorizonFinding& finding, AccessLevel access) {
    if (can_view_diagnostic_detail(access, DiagnosticDetailSurface::KnowledgeHorizonFinding)) {
        return true;
    }
    return finding.context_type == KnowledgeContextType::Interpreter ||
           finding.subject_type == KnowledgeSubjectType::PublicArtifact ||
           finding.subject_type == KnowledgeSubjectType::PublicClaim;
}

void sort_report(KnowledgeHorizonReport& report) {
    std::sort(report.findings.begin(), report.findings.end(), [](const KnowledgeHorizonFinding& lhs, const KnowledgeHorizonFinding& rhs) {
        return lhs.id < rhs.id;
    });
    std::sort(report.errors.begin(), report.errors.end());
    std::sort(report.warnings.begin(), report.warnings.end());
}

} // namespace

[[nodiscard]] std::string to_string(KnowledgeSubjectType type) {
    switch (type) {
        case KnowledgeSubjectType::HiddenEntity: return "hidden_entity";
        case KnowledgeSubjectType::HiddenEvent: return "hidden_event";
        case KnowledgeSubjectType::PublicArtifact: return "public_artifact";
        case KnowledgeSubjectType::PublicClaim: return "public_claim";
        case KnowledgeSubjectType::Mystery: return "mystery";
        case KnowledgeSubjectType::EvidencePotential: return "evidence_potential";
        case KnowledgeSubjectType::CivilizationSpec: return "civilization_spec";
        case KnowledgeSubjectType::CivilizationFragment: return "civilization_fragment";
        case KnowledgeSubjectType::Term: return "term";
        case KnowledgeSubjectType::Office: return "office";
        case KnowledgeSubjectType::Technology: return "technology";
        case KnowledgeSubjectType::Script: return "script";
        case KnowledgeSubjectType::PlaceName: return "place_name";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(KnowledgeContextType type) {
    switch (type) {
        case KnowledgeContextType::ArtifactCreator: return "artifact_creator";
        case KnowledgeContextType::Interpreter: return "interpreter";
        case KnowledgeContextType::PublicArchive: return "public_archive";
        case KnowledgeContextType::CuratorAudit: return "curator_audit";
        case KnowledgeContextType::EvidencePotentialDerivation: return "evidence_potential_derivation";
        case KnowledgeContextType::SnapshotValidation: return "snapshot_validation";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(KnowledgeHorizonStatus status) {
    switch (status) {
        case KnowledgeHorizonStatus::Valid: return "valid";
        case KnowledgeHorizonStatus::ValidByTransmission: return "valid_by_transmission";
        case KnowledgeHorizonStatus::ValidByForgery: return "valid_by_forgery";
        case KnowledgeHorizonStatus::ValidByLaterCopy: return "valid_by_later_copy";
        case KnowledgeHorizonStatus::ValidByRitualConvention: return "valid_by_ritual_convention";
        case KnowledgeHorizonStatus::ValidByPublicEvidence: return "valid_by_public_evidence";
        case KnowledgeHorizonStatus::ValidByCuratorAccess: return "valid_by_curator_access";
        case KnowledgeHorizonStatus::InvalidUnavailable: return "invalid_unavailable";
        case KnowledgeHorizonStatus::InvalidFutureKnowledge: return "invalid_future_knowledge";
        case KnowledgeHorizonStatus::InvalidAccessLeak: return "invalid_access_leak";
        case KnowledgeHorizonStatus::InvalidUnmediatedHiddenTruth: return "invalid_unmediated_hidden_truth";
    }
    return "unknown";
}

[[nodiscard]] bool is_invalid(KnowledgeHorizonStatus status) {
    return status == KnowledgeHorizonStatus::InvalidUnavailable ||
           status == KnowledgeHorizonStatus::InvalidFutureKnowledge ||
           status == KnowledgeHorizonStatus::InvalidAccessLeak ||
           status == KnowledgeHorizonStatus::InvalidUnmediatedHiddenTruth;
}

[[nodiscard]] KnowledgeHorizonFinding evaluate_knowledge_reference(
    const ArchiveEngineState& state,
    KnowledgeContextType context_type,
    const std::string& context_id,
    KnowledgeSubjectType subject_type,
    const std::string& subject_id,
    int context_year,
    AccessLevel access
) {
    KnowledgeHorizonFinding finding = make_finding(context_type, context_id, subject_type, subject_id, context_year);
    const SubjectAvailability availability = subject_availability(state, subject_type, subject_id);
    finding.earliest_available_year = availability.earliest_year;

    if (subject_id.empty() || !availability.exists) {
        finding.status = KnowledgeHorizonStatus::InvalidUnavailable;
        finding.explanation = "Referenced subject is not present in the current archive state.";
        finding.validation_errors.push_back("missing subject");
        return finding;
    }

    if (availability.earliest_year > context_year) {
        KnowledgeHorizonStatus mediated_status = KnowledgeHorizonStatus::Valid;
        std::vector<std::string> mediators;
        if (has_context_mediation(state, context_type, context_id, mediated_status, mediators)) {
            finding.status = mediated_status;
            finding.mediating_artifact_ids = std::move(mediators);
            finding.explanation = "Reference postdates the context but is mediated by transmission, forgery, later copy, or ritual convention.";
            return finding;
        }
        finding.status = KnowledgeHorizonStatus::InvalidFutureKnowledge;
        finding.explanation = "Referenced subject was not available by the context year.";
        finding.validation_errors.push_back("future knowledge");
        return finding;
    }

    if (context_should_enforce_viewer_access(context_type) && !can_view(access, availability.min_access)) {
        KnowledgeHorizonStatus mediated_status = KnowledgeHorizonStatus::Valid;
        std::vector<std::string> mediators;
        if (has_context_mediation(state, context_type, context_id, mediated_status, mediators)) {
            finding.status = mediated_status;
            finding.mediating_artifact_ids = std::move(mediators);
            finding.explanation = "Restricted subject is mediated rather than exposed as direct knowledge.";
            return finding;
        }
        finding.status = availability.hidden_only ? KnowledgeHorizonStatus::InvalidUnmediatedHiddenTruth : KnowledgeHorizonStatus::InvalidAccessLeak;
        finding.explanation = "Referenced subject requires a higher access level than the validation context provides.";
        finding.validation_errors.push_back("access leak");
        return finding;
    }

    if (availability.hidden_only && context_should_enforce_viewer_access(context_type) && can_view(access, AccessLevel::Curator)) {
        finding.status = KnowledgeHorizonStatus::ValidByCuratorAccess;
        finding.explanation = "Hidden-only subject is available because the validation context has curator/debug visibility.";
        return finding;
    }

    finding.status = KnowledgeHorizonStatus::Valid;
    finding.explanation = "Referenced subject is available within the context year and access horizon.";
    return finding;
}

[[nodiscard]] KnowledgeHorizonReport validate_knowledge_horizon(const ArchiveEngineState& state, AccessLevel access) {
    KnowledgeHorizonReport report;
    validate_artifact_creator_knowledge(report, state, access);
    validate_theory_knowledge(report, state, access);
    validate_evidence_potential_knowledge(report, state, access);
    sort_report(report);
    return report;
}

[[nodiscard]] std::string format_knowledge_horizon_summary(const ArchiveEngineState& state, AccessLevel access) {
    const KnowledgeHorizonReport report = validate_knowledge_horizon(state, access);
    std::map<std::string, std::size_t> by_status;
    std::map<std::string, std::size_t> by_context;
    for (const KnowledgeHorizonFinding& finding : report.findings) {
        ++by_status[to_string(finding.status)];
        ++by_context[to_string(finding.context_type)];
    }
    std::ostringstream out;
    out << "KnowledgeHorizon summary:\n";
    out << "- total_findings: " << report.findings.size() << "\n";
    out << "- errors: " << report.errors.size() << "\n";
    out << "- behavior: validation only; no artifacts, discoveries, candidates, fragments, or specs are generated or resolved by KnowledgeHorizon in v28.4.\n";
    out << "Status counts:\n";
    for (const auto& [status, count] : by_status) {
        out << "- " << status << ": " << count << "\n";
    }
    out << "Context counts:\n";
    for (const auto& [context, count] : by_context) {
        out << "- " << context << ": " << count << "\n";
    }
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::KnowledgeHorizonFinding)) {
        out << "- details: restricted; use curator/debug access for finding IDs and source diagnostics.\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_knowledge_horizon_validation(const ArchiveEngineState& state, AccessLevel access) {
    const KnowledgeHorizonReport report = validate_knowledge_horizon(state, access);
    std::ostringstream out;
    out << "KnowledgeHorizon validation:\n";
    out << "- result: " << (report.errors.empty() ? "passed" : "failed") << "\n";
    out << "- findings: " << report.findings.size() << "\n";
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

[[nodiscard]] std::string format_knowledge_horizon_findings(const ArchiveEngineState& state, AccessLevel access) {
    const KnowledgeHorizonReport report = validate_knowledge_horizon(state, access);
    std::ostringstream out;
    out << "KnowledgeHorizon findings visible to " << to_string(access) << ":\n";
    if (!can_view_diagnostic_detail(access, DiagnosticDetailSurface::KnowledgeHorizonFinding)) {
        out << "- aggregate-only at this access level; hidden IDs and explanations are restricted.\n";
        out << "- total_findings: " << report.findings.size() << "\n";
        out << "- errors: " << report.errors.size() << "\n";
        return out.str();
    }
    bool any = false;
    for (const KnowledgeHorizonFinding& finding : report.findings) {
        if (!finding_visible_to(finding, access)) {
            continue;
        }
        any = true;
        out << "- " << finding.id << ": " << to_string(finding.status)
            << " context=" << to_string(finding.context_type) << ":" << finding.context_id
            << " subject=" << to_string(finding.subject_type) << ":" << finding.subject_id
            << " year=" << finding.context_year
            << " earliest=" << finding.earliest_available_year << "\n";
    }
    if (!any) {
        out << "- none\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_knowledge_horizon_finding_detail(const ArchiveEngineState& state,
                                                                   AccessLevel access,
                                                                   const std::string& finding_id) {
    const KnowledgeHorizonReport report = validate_knowledge_horizon(state, access);
    const auto it = std::find_if(report.findings.begin(), report.findings.end(), [&](const KnowledgeHorizonFinding& finding) {
        return finding.id == finding_id;
    });
    std::ostringstream out;
    out << "KnowledgeHorizon finding:\n";
    if (it == report.findings.end() ||
        !can_view_diagnostic_detail(access, DiagnosticDetailSurface::KnowledgeHorizonFinding) ||
        !finding_visible_to(*it, access)) {
        out << "- found: false\n";
        return out.str();
    }

    out << "- found: true\n";
    out << "- id: " << it->id << "\n";
    out << "- status: " << to_string(it->status) << "\n";
    out << "- context_type: " << to_string(it->context_type) << "\n";
    out << "- subject_type: " << to_string(it->subject_type) << "\n";
    out << "- context_year: " << it->context_year << "\n";
    out << "- earliest_available_year: " << it->earliest_available_year << "\n";
    out << "- context_id: " << it->context_id << "\n";
    out << "- subject_id: " << it->subject_id << "\n";
    out << "- explanation: " << it->explanation << "\n";
    if (!it->mediating_artifact_ids.empty()) {
        out << "- mediating_artifact_ids:";
        for (const std::string& id : it->mediating_artifact_ids) {
            out << " " << id;
        }
        out << "\n";
    }
    return out.str();
}

} // namespace archive
