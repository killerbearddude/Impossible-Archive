#pragma once
#include "hidden_truth_model.h"
#include "public_archive_model.h"
#include "candidate_model.h"
#include "evidence_potential_model.h"
#include "candidate_artifact_plan_model.h"
#include "candidate_artifact_plan_evaluation_model.h"
#include "candidate_artifact_proposal_model.h"
#include "candidate_artifact_proposal_audit_model.h"
#include "control_layer_audit_model.h"

namespace archive {

struct CivilizationRuntimeSource {
    std::string civilization_id;
    std::string display_name;
    std::uint64_t seed = 0;
    int earliest_year = 0;
    int latest_year = 0;
    std::string catalog_id;
    std::string schema_version;
};

struct ArchiveEngineState {
    std::uint64_t seed = 0;
    bool include_seeded_calendar_dispute = false;
    HiddenTruthGraph hidden_truth;
    PublicArchive public_archive;
    std::vector<Discovery> discovery_log;
    std::vector<Mystery> mysteries;
    std::vector<HiddenTruthMutationRecord> hidden_truth_mutations;
    std::vector<EvidencePotential> evidence_potentials;
    std::vector<CandidateArtifactPlan> candidate_artifact_plans;
    std::vector<CandidateArtifactPlanEvaluation> candidate_artifact_plan_evaluations;
    std::vector<CandidateArtifactProposal> candidate_artifact_proposals;
    std::vector<CandidateArtifactProposalAudit> candidate_artifact_proposal_audits;
    std::vector<ControlLayerAuditEntry> control_layer_audit_entries;
    std::optional<CivilizationRuntimeSource> civilization_source;
    std::size_t civilization_spec_count = 0;
    std::size_t civilization_fragment_count = 0;
};

struct EvidenceCitation {
    std::string artifact_id;
    std::optional<std::string> claim_id;
    double weight = 0.0;
};

struct AnswerBlock {
    std::string heading;
    std::string summary;
    std::vector<EvidenceCitation> supporting_evidence;
    std::vector<std::string> contradiction_ids;
    std::vector<std::string> hidden_event_ids;
    AccessLevel min_access = AccessLevel::Public;
};

struct Answer {
    AccessLevel access = AccessLevel::Public;
    int archive_year = kOpenEndedYear;
    std::vector<AnswerBlock> blocks;
};

struct MysteryAssessment {
    std::string mystery_id;
    std::string title;
    std::string status;
    std::string summary;
    double raw_confidence = 0.0;
    double capped_confidence = 0.0;
    double confidence_cap = 1.0;
    std::vector<EvidenceCitation> supporting_evidence;
    std::vector<EvidenceCitation> context_evidence;
    std::vector<EvidenceCitation> misleading_evidence;
    std::vector<std::string> contradiction_ids;
    std::vector<std::string> visible_clue_artifact_ids;
    std::vector<std::string> visible_misleading_artifact_ids;
    int visible_core_clues = 0;
    int visible_context_clues = 0;
    int visible_misleading_clues = 0;
};

enum class EpistemicStyle {
    BureaucraticMinimalist,
    RitualFormalist,
    AntiDynasticRevisionist,
};

struct Interpreter {
    std::string id;
    std::string name;
    EpistemicStyle style = EpistemicStyle::BureaucraticMinimalist;
    AccessLevel access = AccessLevel::Scholar;
};

struct Theory {
    std::string id;
    std::string interpreter_id;
    std::string interpreter_name;
    EpistemicStyle style = EpistemicStyle::BureaucraticMinimalist;
    AccessLevel min_access = AccessLevel::Scholar;
    std::string summary;
    std::vector<EvidenceCitation> supporting_evidence;
    std::vector<std::string> contradiction_ids;
    double confidence = 0.0;
};

struct VoiceClaimView {
    const Claim* claim = nullptr;
    ArtifactVoiceClaimRole role = ArtifactVoiceClaimRole::PrimaryLine;
    double weight = 1.0;
};

struct ScoredClaim {
    const Claim* claim = nullptr;
    double score = 0.0;
};


[[nodiscard]] std::string to_string(EpistemicStyle style);
void add_mystery(ArchiveEngineState& state, Mystery mystery);
[[nodiscard]] ArchiveEngineState initialize_archive_engine(std::uint64_t seed);
[[nodiscard]] std::vector<Contradiction> detect_contradictions(const ArchiveEngineState& state);
void add_detected_contradictions_to_archive(ArchiveEngineState& state);
[[nodiscard]] std::vector<AnachronismReport> detect_anachronisms(const ArchiveEngineState& state);

} // namespace archive
