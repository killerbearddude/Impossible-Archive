#pragma once
#include "civilization_spec_model.h"

namespace archive {

[[nodiscard]] CivilizationSpecLoadResult load_civilization_specs_from_json_text(
    std::string_view json_text
);

[[nodiscard]] CivilizationSpecLoadResult load_civilization_specs_from_json_file(
    const std::string& path
);

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_spec(
    const CivilizationSpec& spec
);

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_catalog(
    const CivilizationSpecCatalog& catalog
);

[[nodiscard]] const CivilizationSpec* find_civilization_spec(
    const CivilizationSpecCatalog& catalog,
    std::string_view civilization_id
);

[[nodiscard]] std::string format_civilization_spec_summary(
    const CivilizationSpec& spec
);

[[nodiscard]] std::string format_civilization_catalog_validation(
    const CivilizationSpecCatalog& catalog,
    const CivilizationSpecValidationResult& validation
);

[[nodiscard]] std::string format_civilization_catalog_list(
    const CivilizationSpecCatalog& catalog,
    const CivilizationSpecValidationResult& validation
);

[[nodiscard]] std::vector<std::string> collect_civilization_catalog_tags(
    const CivilizationSpecCatalog& catalog
);

[[nodiscard]] std::vector<const CivilizationSpec*> find_civilization_specs_by_tag(
    const CivilizationSpecCatalog& catalog,
    std::string_view tag
);

[[nodiscard]] std::string format_civilization_catalog_tags(
    const CivilizationSpecCatalog& catalog
);

[[nodiscard]] std::string format_civilization_specs_by_tag(
    const CivilizationSpecCatalog& catalog,
    std::string_view tag
);

[[nodiscard]] std::string format_civilization_catalog_tag_validation(
    const CivilizationSpecCatalog& catalog,
    const CivilizationSpecValidationResult& validation
);

} // namespace archive
