#pragma once
#include "civilization_spec_model.h"

namespace archive {

[[nodiscard]] std::optional<CivilizationFragmentCategory> parse_civilization_fragment_category(
    std::string_view value
);

[[nodiscard]] std::string format_civilization_fragment_category(
    CivilizationFragmentCategory category
);

[[nodiscard]] std::optional<PatchStrategy> parse_patch_strategy(
    std::string_view value
);

[[nodiscard]] std::string format_patch_strategy(PatchStrategy strategy);

[[nodiscard]] std::string format_spec_patch_value(const SpecPatchValue& value);

[[nodiscard]] CivilizationSpecValidationResult validate_spec_patch(const SpecPatch& patch);

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_spec_fragment(
    const CivilizationSpecFragment& fragment
);

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_fragments(
    const CivilizationSpecCatalog& catalog
);

[[nodiscard]] const CivilizationSpecFragment* find_civilization_fragment(
    const CivilizationSpecCatalog& catalog,
    std::string_view fragment_id
);

[[nodiscard]] std::string format_civilization_fragment_list(
    const CivilizationSpecCatalog& catalog
);

[[nodiscard]] std::string format_civilization_spec_fragment(
    const CivilizationSpecFragment& fragment
);

[[nodiscard]] std::string format_civilization_fragment_validation(
    const CivilizationSpecCatalog& catalog,
    const CivilizationSpecValidationResult& validation
);

} // namespace archive
