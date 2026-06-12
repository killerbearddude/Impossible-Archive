#pragma once
#include "archive_common.h"

namespace archive {

enum class KnowledgeSubjectType {
    HiddenEntity,
    HiddenEvent,
    PublicArtifact,
    PublicClaim,
    Mystery,
    EvidencePotential,
    CivilizationSpec,
    CivilizationFragment,
    Term,
    Office,
    Technology,
    Script,
    PlaceName,
};

enum class KnowledgeContextType {
    ArtifactCreator,
    Interpreter,
    PublicArchive,
    CuratorAudit,
    EvidencePotentialDerivation,
    SnapshotValidation,
};

enum class KnowledgeHorizonStatus {
    Valid,
    ValidByTransmission,
    ValidByForgery,
    ValidByLaterCopy,
    ValidByRitualConvention,
    ValidByPublicEvidence,
    ValidByCuratorAccess,
    InvalidUnavailable,
    InvalidFutureKnowledge,
    InvalidAccessLeak,
    InvalidUnmediatedHiddenTruth,
};

struct KnowledgeHorizonFinding {
    std::string id;

    KnowledgeContextType context_type = KnowledgeContextType::ArtifactCreator;
    std::string context_id;

    KnowledgeSubjectType subject_type = KnowledgeSubjectType::HiddenEntity;
    std::string subject_id;

    int context_year = 0;
    int earliest_available_year = 0;

    KnowledgeHorizonStatus status = KnowledgeHorizonStatus::Valid;

    std::string explanation;
    std::vector<std::string> mediating_artifact_ids;
    std::vector<std::string> validation_errors;
};

struct KnowledgeHorizonReport {
    std::vector<KnowledgeHorizonFinding> findings;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

[[nodiscard]] std::string to_string(KnowledgeSubjectType type);
[[nodiscard]] std::string to_string(KnowledgeContextType type);
[[nodiscard]] std::string to_string(KnowledgeHorizonStatus status);
[[nodiscard]] bool is_invalid(KnowledgeHorizonStatus status);

} // namespace archive
