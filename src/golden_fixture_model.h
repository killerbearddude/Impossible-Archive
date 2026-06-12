#pragma once
#include "archive_common.h"
#include "archive_engine_state.h"

namespace archive {

struct GoldenFixtureWorldDefinition {
    std::string id;
    std::string description;
    std::string runtime_mode;
    std::string spec_file;
    std::string civilization_id;
    std::uint64_t seed = 0;
    int archive_year = 0;
};

struct GoldenFixtureBuildResult {
    ArchiveEngineState state;
    GoldenFixtureWorldDefinition definition;
    bool ok = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

} // namespace archive
