#include "civilization_fragments_api.h"

namespace archive {
namespace {

[[nodiscard]] bool is_lowercase_snake_or_dot_case(std::string_view value) {
    if (value.empty()) {
        return false;
    }
    const char first = value.front();
    if (first == '_' || first == '.') {
        return false;
    }
    const char last = value.back();
    if (last == '_' || last == '.') {
        return false;
    }
    bool previous_separator = false;
    for (char c : value) {
        const bool separator = c == '_' || c == '.';
        const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || separator;
        if (!ok) {
            return false;
        }
        if (separator && previous_separator) {
            return false;
        }
        previous_separator = separator;
    }
    return first >= 'a' && first <= 'z';
}

[[nodiscard]] bool is_lowercase_snake_case_local(std::string_view value) {
    if (value.empty() || value.front() == '_' || value.back() == '_') {
        return false;
    }
    bool previous_underscore = false;
    for (char c : value) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) {
            return false;
        }
        if (c == '_') {
            if (previous_underscore) {
                return false;
            }
            previous_underscore = true;
        } else {
            previous_underscore = false;
        }
    }
    return value.front() >= 'a' && value.front() <= 'z';
}

[[nodiscard]] CivilizationSpecValidationResult validate_tag_vector(
    const std::vector<std::string>& tags,
    std::string_view field_name
) {
    CivilizationSpecValidationResult result;
    std::set<std::string> seen;
    for (const std::string& tag : tags) {
        if (tag.empty()) {
            result.errors.push_back(std::string(field_name) + " contains an empty value");
            continue;
        }
        if (!seen.insert(tag).second) {
            result.warnings.push_back(std::string(field_name) + " contains duplicate value: " + tag);
        }
        if (!is_lowercase_snake_case_local(tag)) {
            result.warnings.push_back(std::string(field_name) + " contains non-standard tag format: " + tag);
        }
    }
    result.valid = result.errors.empty();
    return result;
}

void merge_result(CivilizationSpecValidationResult& target,
                  const CivilizationSpecValidationResult& source) {
    target.errors.insert(target.errors.end(), source.errors.begin(), source.errors.end());
    target.warnings.insert(target.warnings.end(), source.warnings.begin(), source.warnings.end());
}

enum class PatchPathKind {
    String,
    Integer,
    Double,
    StringList,
    Bool,
};

[[nodiscard]] std::optional<PatchPathKind> patch_path_kind(std::string_view path) {
    static const std::map<std::string, PatchPathKind> registry = {
        {"target_hidden_entity_count", PatchPathKind::Integer},
        {"target_hidden_event_count", PatchPathKind::Integer},
        {"target_public_artifact_count", PatchPathKind::Integer},
        {"target_mystery_count", PatchPathKind::Integer},
        {"tags", PatchPathKind::StringList},
        {"profile.target_depth", PatchPathKind::String},
        {"profile.future_modules", PatchPathKind::StringList},
        {"geographic_features", PatchPathKind::StringList},
        {"environmental_pressures", PatchPathKind::StringList},
        {"major_sites", PatchPathKind::StringList},
        {"economic_pressures", PatchPathKind::StringList},
        {"trade_goods", PatchPathKind::StringList},
        {"institution_archetypes", PatchPathKind::StringList},
        {"social_actor_archetypes", PatchPathKind::StringList},
        {"authority_conflicts", PatchPathKind::StringList},
        {"religious_or_mythic_archetypes", PatchPathKind::StringList},
        {"ritual_pressures", PatchPathKind::StringList},
        {"writing_system_archetypes", PatchPathKind::StringList},
        {"recordkeeping_styles", PatchPathKind::StringList},
        {"artifact_media", PatchPathKind::StringList},
        {"evidence_distortion_modes", PatchPathKind::StringList},
        {"mystery_archetypes", PatchPathKind::StringList},
    };
    const auto it = registry.find(std::string(path));
    if (it == registry.end()) {
        return std::nullopt;
    }
    return it->second;
}

[[nodiscard]] bool strategy_allowed_for_path_kind(PatchStrategy strategy, PatchPathKind kind) {
    switch (kind) {
    case PatchPathKind::String:
        return strategy == PatchStrategy::Replace || strategy == PatchStrategy::SetIfAbsent;
    case PatchPathKind::Integer:
        return strategy == PatchStrategy::Replace || strategy == PatchStrategy::Add ||
               strategy == PatchStrategy::Min || strategy == PatchStrategy::Max ||
               strategy == PatchStrategy::Clamp;
    case PatchPathKind::Double:
        return strategy == PatchStrategy::Replace || strategy == PatchStrategy::Add ||
               strategy == PatchStrategy::Scale || strategy == PatchStrategy::Min ||
               strategy == PatchStrategy::Max || strategy == PatchStrategy::Clamp;
    case PatchPathKind::StringList:
        return strategy == PatchStrategy::Replace || strategy == PatchStrategy::AppendUnique ||
               strategy == PatchStrategy::SetIfAbsent;
    case PatchPathKind::Bool:
        return strategy == PatchStrategy::Replace || strategy == PatchStrategy::SetIfAbsent;
    }
    return false;
}

[[nodiscard]] bool value_allowed_for_path_kind(const SpecPatchValue& value, PatchPathKind kind) {
    switch (kind) {
    case PatchPathKind::String:
        return value.kind == SpecPatchValue::Kind::String;
    case PatchPathKind::Integer:
        return value.kind == SpecPatchValue::Kind::Int;
    case PatchPathKind::Double:
        return value.kind == SpecPatchValue::Kind::Double || value.kind == SpecPatchValue::Kind::Int;
    case PatchPathKind::StringList:
        return value.kind == SpecPatchValue::Kind::StringList;
    case PatchPathKind::Bool:
        return value.kind == SpecPatchValue::Kind::Bool;
    }
    return false;
}

[[nodiscard]] std::string patch_path_kind_name(PatchPathKind kind) {
    switch (kind) {
    case PatchPathKind::String: return "string";
    case PatchPathKind::Integer: return "integer";
    case PatchPathKind::Double: return "double";
    case PatchPathKind::StringList: return "string_list";
    case PatchPathKind::Bool: return "bool";
    }
    return "unknown";
}

[[nodiscard]] std::string join_fragment_values(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "none";
    }
    std::ostringstream out;
    for (std::size_t i = 0U; i < values.size(); ++i) {
        if (i > 0U) {
            out << ", ";
        }
        out << values[i];
    }
    return out.str();
}

} // namespace

[[nodiscard]] std::optional<CivilizationFragmentCategory> parse_civilization_fragment_category(std::string_view value) {
    if (value == "geography") return CivilizationFragmentCategory::Geography;
    if (value == "ecology") return CivilizationFragmentCategory::Ecology;
    if (value == "polity") return CivilizationFragmentCategory::Polity;
    if (value == "economy") return CivilizationFragmentCategory::Economy;
    if (value == "religion") return CivilizationFragmentCategory::Religion;
    if (value == "culture") return CivilizationFragmentCategory::Culture;
    if (value == "technology") return CivilizationFragmentCategory::Technology;
    if (value == "artifact_bias") return CivilizationFragmentCategory::ArtifactBias;
    if (value == "historical_density") return CivilizationFragmentCategory::HistoricalDensity;
    if (value == "truth_policy") return CivilizationFragmentCategory::TruthPolicy;
    if (value == "mystery_policy") return CivilizationFragmentCategory::MysteryPolicy;
    if (value == "strangeness_overlay") return CivilizationFragmentCategory::StrangenessOverlay;
    if (value == "preservation_model") return CivilizationFragmentCategory::PreservationModel;
    if (value == "collapse_model") return CivilizationFragmentCategory::CollapseModel;
    if (value == "constraint") return CivilizationFragmentCategory::Constraint;
    return std::nullopt;
}

[[nodiscard]] std::string format_civilization_fragment_category(CivilizationFragmentCategory category) {
    switch (category) {
    case CivilizationFragmentCategory::Geography: return "geography";
    case CivilizationFragmentCategory::Ecology: return "ecology";
    case CivilizationFragmentCategory::Polity: return "polity";
    case CivilizationFragmentCategory::Economy: return "economy";
    case CivilizationFragmentCategory::Religion: return "religion";
    case CivilizationFragmentCategory::Culture: return "culture";
    case CivilizationFragmentCategory::Technology: return "technology";
    case CivilizationFragmentCategory::ArtifactBias: return "artifact_bias";
    case CivilizationFragmentCategory::HistoricalDensity: return "historical_density";
    case CivilizationFragmentCategory::TruthPolicy: return "truth_policy";
    case CivilizationFragmentCategory::MysteryPolicy: return "mystery_policy";
    case CivilizationFragmentCategory::StrangenessOverlay: return "strangeness_overlay";
    case CivilizationFragmentCategory::PreservationModel: return "preservation_model";
    case CivilizationFragmentCategory::CollapseModel: return "collapse_model";
    case CivilizationFragmentCategory::Constraint: return "constraint";
    }
    return "culture";
}

[[nodiscard]] std::optional<PatchStrategy> parse_patch_strategy(std::string_view value) {
    if (value == "replace") return PatchStrategy::Replace;
    if (value == "set_if_absent") return PatchStrategy::SetIfAbsent;
    if (value == "append_unique") return PatchStrategy::AppendUnique;
    if (value == "add") return PatchStrategy::Add;
    if (value == "scale") return PatchStrategy::Scale;
    if (value == "clamp") return PatchStrategy::Clamp;
    if (value == "min") return PatchStrategy::Min;
    if (value == "max") return PatchStrategy::Max;
    return std::nullopt;
}

[[nodiscard]] std::string format_patch_strategy(PatchStrategy strategy) {
    switch (strategy) {
    case PatchStrategy::Replace: return "replace";
    case PatchStrategy::SetIfAbsent: return "set_if_absent";
    case PatchStrategy::AppendUnique: return "append_unique";
    case PatchStrategy::Add: return "add";
    case PatchStrategy::Scale: return "scale";
    case PatchStrategy::Clamp: return "clamp";
    case PatchStrategy::Min: return "min";
    case PatchStrategy::Max: return "max";
    }
    return "replace";
}

[[nodiscard]] std::string format_spec_patch_value(const SpecPatchValue& value) {
    std::ostringstream out;
    switch (value.kind) {
    case SpecPatchValue::Kind::Bool:
        out << (value.bool_value ? "true" : "false");
        break;
    case SpecPatchValue::Kind::Int:
        out << value.int_value;
        break;
    case SpecPatchValue::Kind::Double:
        out << value.double_value;
        break;
    case SpecPatchValue::Kind::String:
        out << value.string_value;
        break;
    case SpecPatchValue::Kind::StringList:
        out << '[' << join_fragment_values(value.string_list_value) << ']';
        break;
    }
    return out.str();
}

[[nodiscard]] CivilizationSpecValidationResult validate_spec_patch(const SpecPatch& patch) {
    CivilizationSpecValidationResult result;
    if (patch.path.empty()) {
        result.errors.push_back("patch.path is empty");
        result.valid = false;
        return result;
    }
    const std::optional<PatchPathKind> kind = patch_path_kind(patch.path);
    if (!kind.has_value()) {
        result.errors.push_back("patch.path is not in the v28.0 draft registry: " + patch.path);
    } else {
        if (!strategy_allowed_for_path_kind(patch.strategy, *kind)) {
            result.errors.push_back("patch.strategy " + format_patch_strategy(patch.strategy) +
                                    " is not valid for " + patch.path + " (" + patch_path_kind_name(*kind) + ")");
        }
        if (!value_allowed_for_path_kind(patch.value, *kind)) {
            result.errors.push_back("patch.value type is not valid for " + patch.path +
                                    " (expected " + patch_path_kind_name(*kind) + ")");
        }
    }
    result.valid = result.errors.empty();
    return result;
}

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_spec_fragment(const CivilizationSpecFragment& fragment) {
    CivilizationSpecValidationResult result;
    if (fragment.id.empty()) {
        result.errors.push_back("fragment.id is empty");
    } else if (!is_lowercase_snake_or_dot_case(fragment.id)) {
        result.errors.push_back("fragment.id must be stable lowercase snake/dot format: " + fragment.id);
    }
    if (fragment.schema_version != "1.2") {
        result.errors.push_back("fragment " + fragment.id + " schema_version must be 1.2");
    }
    if (fragment.title.empty()) {
        result.errors.push_back("fragment " + fragment.id + " title is empty");
    }
    if (fragment.priority < 0 || fragment.priority > 1000) {
        result.errors.push_back("fragment " + fragment.id + " priority must be in range 0-1000");
    }
    merge_result(result, validate_tag_vector(fragment.tags, fragment.id + ".tags"));
    merge_result(result, validate_tag_vector(fragment.requires_tags, fragment.id + ".requires_tags"));
    merge_result(result, validate_tag_vector(fragment.excludes_tags, fragment.id + ".excludes_tags"));

    std::set<std::string> excludes(fragment.excludes_tags.begin(), fragment.excludes_tags.end());
    for (const std::string& required : fragment.requires_tags) {
        if (!required.empty() && excludes.find(required) != excludes.end()) {
            result.errors.push_back("fragment " + fragment.id + " has overlapping requires/excludes tag: " + required);
        }
    }

    if (fragment.patches.empty() && fragment.category != CivilizationFragmentCategory::Constraint) {
        result.errors.push_back("fragment " + fragment.id + " must contain at least one patch unless category is constraint");
    }
    for (const SpecPatch& patch : fragment.patches) {
        merge_result(result, validate_spec_patch(patch));
    }
    result.valid = result.errors.empty();
    return result;
}

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_fragments(const CivilizationSpecCatalog& catalog) {
    CivilizationSpecValidationResult result;
    std::set<std::string> ids;
    for (const CivilizationSpecFragment& fragment : catalog.fragments) {
        if (!fragment.id.empty() && !ids.insert(fragment.id).second) {
            result.errors.push_back("duplicate fragment id: " + fragment.id);
        }
        const CivilizationSpecValidationResult fragment_result = validate_civilization_spec_fragment(fragment);
        for (const std::string& error : fragment_result.errors) {
            result.errors.push_back(fragment.id.empty() ? error : (fragment.id + ": " + error));
        }
        for (const std::string& warning : fragment_result.warnings) {
            result.warnings.push_back(fragment.id.empty() ? warning : (fragment.id + ": " + warning));
        }
    }
    result.valid = result.errors.empty();
    return result;
}

[[nodiscard]] const CivilizationSpecFragment* find_civilization_fragment(const CivilizationSpecCatalog& catalog,
                                                                         std::string_view fragment_id) {
    const auto it = std::find_if(catalog.fragments.begin(), catalog.fragments.end(), [&](const CivilizationSpecFragment& fragment) {
        return fragment.id == fragment_id;
    });
    if (it == catalog.fragments.end()) {
        return nullptr;
    }
    return &(*it);
}

[[nodiscard]] std::string format_civilization_fragment_list(const CivilizationSpecCatalog& catalog) {
    std::ostringstream out;
    out << "CivilizationSpec fragments:\n";
    if (catalog.fragments.empty()) {
        out << "- none\n";
        return out.str();
    }
    for (const CivilizationSpecFragment& fragment : catalog.fragments) {
        out << "- " << fragment.id << ": " << fragment.title << " ["
            << format_civilization_fragment_category(fragment.category) << "]\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_civilization_spec_fragment(const CivilizationSpecFragment& fragment) {
    std::ostringstream out;
    out << "CivilizationSpec fragment:\n";
    out << "- id: " << fragment.id << "\n";
    out << "- title: " << fragment.title << "\n";
    out << "- schema_version: " << fragment.schema_version << "\n";
    out << "- category: " << format_civilization_fragment_category(fragment.category) << "\n";
    out << "- priority: " << fragment.priority << "\n";
    out << "- tags: " << join_fragment_values(fragment.tags) << "\n";
    out << "- requires_tags: " << join_fragment_values(fragment.requires_tags) << "\n";
    out << "- excludes_tags: " << join_fragment_values(fragment.excludes_tags) << "\n";
    out << "- patches:\n";
    if (fragment.patches.empty()) {
        out << "  - none\n";
    } else {
        for (const SpecPatch& patch : fragment.patches) {
            out << "  - " << patch.path << ' ' << format_patch_strategy(patch.strategy)
                << ' ' << format_spec_patch_value(patch.value) << "\n";
        }
    }
    return out.str();
}

[[nodiscard]] std::string format_civilization_fragment_validation(const CivilizationSpecCatalog& catalog,
                                                                  const CivilizationSpecValidationResult& validation) {
    std::size_t valid_count = 0U;
    for (const CivilizationSpecFragment& fragment : catalog.fragments) {
        if (validate_civilization_spec_fragment(fragment).valid) {
            ++valid_count;
        }
    }
    std::ostringstream out;
    out << "CivilizationSpec fragment validation:\n";
    out << "- loaded fragments: " << catalog.fragments.size() << "\n";
    out << "- valid fragments: " << valid_count << "\n";
    out << "- errors: " << validation.errors.size() << "\n";
    out << "- warnings: " << validation.warnings.size() << "\n";
    for (const std::string& error : validation.errors) {
        out << "  error: " << error << "\n";
    }
    for (const std::string& warning : validation.warnings) {
        out << "  warning: " << warning << "\n";
    }
    return out.str();
}

} // namespace archive
