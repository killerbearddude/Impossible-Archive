#pragma once
#include "public_archive_model.h"

namespace archive {

enum class EvidencePotentialSourceType {
    HiddenEvent,
    HiddenEntity,
    Site,
    Office,
    Mystery,
};

enum class EvidencePotentialTraceType {
    InscriptionTrace,
    LedgerTrace,
    OralTraditionTrace,
    RitualTrace,
    LegalRecordTrace,
    MaterialDepositTrace,
    CopyTraditionTrace,
    AbsenceTrace,
};

enum class EvidencePotentialStrength {
    Weak,
    Moderate,
    Strong,
};

struct EvidencePotential {
    std::string id;

    EvidencePotentialSourceType source_type = EvidencePotentialSourceType::HiddenEvent;
    std::string source_id;

    EvidencePotentialTraceType trace_type = EvidencePotentialTraceType::InscriptionTrace;
    ArtifactType likely_artifact_type = ArtifactType::Inscription;

    std::string subject;
    std::string rationale;

    int earliest_possible_year = 0;
    int latest_possible_year = 0;

    std::vector<std::string> likely_site_ids;
    std::vector<std::string> likely_creator_ids;
    std::vector<EvidenceModifier> likely_distortions;

    EvidencePotentialStrength strength = EvidencePotentialStrength::Moderate;

    bool discoverable = true;
    bool public_safe = true;
    AccessLevel min_access = AccessLevel::Curator;
};

[[nodiscard]] std::string to_string(EvidencePotentialSourceType type);
[[nodiscard]] std::string to_string(EvidencePotentialTraceType type);
[[nodiscard]] std::string to_string(EvidencePotentialStrength strength);

} // namespace archive
