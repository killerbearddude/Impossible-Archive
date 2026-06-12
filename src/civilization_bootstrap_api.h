#pragma once
#include "archive_engine_state.h"
#include "civilization_spec_model.h"

namespace archive {

struct CivilizationBootstrapResult {
    ArchiveEngineState state;
    bool ok = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::string summary;
};

[[nodiscard]] CivilizationBootstrapResult bootstrap_archive_state_from_civilization_spec(
    const CivilizationSpec& spec,
    std::string_view catalog_id,
    std::string_view schema_version
);

[[nodiscard]] std::string format_civilization_bootstrap_summary(
    const ArchiveEngineState& state,
    AccessLevel access
);

} // namespace archive
