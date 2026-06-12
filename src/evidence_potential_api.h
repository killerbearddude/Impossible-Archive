#pragma once
#include "archive_engine_state.h"

namespace archive {

[[nodiscard]] std::vector<EvidencePotential> derive_evidence_potentials(
    const ArchiveEngineState& state
);

void derive_evidence_potentials_into_state(ArchiveEngineState& state);

[[nodiscard]] std::vector<std::string> validate_evidence_potentials(
    const ArchiveEngineState& state
);

[[nodiscard]] std::string format_evidence_potential_summary(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_evidence_potential_list(
    const ArchiveEngineState& state,
    AccessLevel access
);

[[nodiscard]] std::string format_evidence_potential_detail(
    const ArchiveEngineState& state,
    AccessLevel access,
    const std::string& evidence_potential_id
);

[[nodiscard]] std::string format_evidence_potential_validation(
    const ArchiveEngineState& state,
    AccessLevel access
);

} // namespace archive
