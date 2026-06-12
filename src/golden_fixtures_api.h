#pragma once
#include "golden_fixture_model.h"

namespace archive {

[[nodiscard]] std::vector<GoldenFixtureWorldDefinition> list_golden_fixture_worlds();

[[nodiscard]] const GoldenFixtureWorldDefinition* find_golden_fixture_world(
    std::string_view fixture_id
);

[[nodiscard]] GoldenFixtureBuildResult build_golden_fixture_world(
    const GoldenFixtureWorldDefinition& definition
);

[[nodiscard]] GoldenFixtureBuildResult build_golden_fixture_world(
    std::string_view fixture_id
);

[[nodiscard]] std::string format_golden_fixture_worlds();

[[nodiscard]] std::string format_golden_fixture_world(
    const GoldenFixtureWorldDefinition& definition
);

} // namespace archive
