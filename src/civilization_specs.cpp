#include "civilization_specs_api.h"
#include "civilization_fragments_api.h"

#include <fstream>
#include <limits>

namespace archive {
namespace {

enum class JsonKind {
    Object,
    Array,
    String,
    Integer,
    Double,
    Bool,
};

struct JsonValue {
    JsonKind kind = JsonKind::String;
    std::map<std::string, JsonValue> object_values;
    std::vector<JsonValue> array_values;
    std::string string_value;
    std::int64_t integer_value = 0;
    double double_value = 0.0;
    bool bool_value = false;
};

class JsonParser {
public:
    explicit JsonParser(std::string_view input) : input_(input) {}

    [[nodiscard]] JsonValue parse() {
        JsonValue value;
        skip_ws();
        if (!parse_value(value)) {
            return JsonValue{};
        }
        skip_ws();
        if (errors_.empty() && pos_ != input_.size()) {
            add_error("unexpected trailing content after JSON document");
        }
        return value;
    }

    [[nodiscard]] const std::vector<std::string>& errors() const {
        return errors_;
    }

private:
    std::string_view input_;
    std::size_t pos_ = 0U;
    std::vector<std::string> errors_;

    void add_error(const std::string& message) {
        if (errors_.empty()) {
            std::ostringstream out;
            out << "json parse error at byte " << pos_ << ": " << message;
            errors_.push_back(out.str());
        }
    }

    [[nodiscard]] bool eof() const {
        return pos_ >= input_.size();
    }

    [[nodiscard]] char peek() const {
        return eof() ? '\0' : input_[pos_];
    }

    void skip_ws() {
        while (!eof()) {
            const char c = peek();
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    [[nodiscard]] bool consume(char expected) {
        if (peek() != expected) {
            std::string message = "expected '";
            message.push_back(expected);
            message.push_back('\'');
            add_error(message);
            return false;
        }
        ++pos_;
        return true;
    }

    [[nodiscard]] bool parse_value(JsonValue& value) {
        skip_ws();
        if (eof()) {
            add_error("unexpected end of input while parsing value");
            return false;
        }
        const char c = peek();
        if (c == '{') {
            return parse_object(value);
        }
        if (c == '[') {
            return parse_array(value);
        }
        if (c == '"') {
            std::string text;
            if (!parse_string(text)) {
                return false;
            }
            value.kind = JsonKind::String;
            value.string_value = std::move(text);
            return true;
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c)) != 0) {
            return parse_number(value);
        }
        if (c == 't' || c == 'f') {
            return parse_bool(value);
        }
        if (c == '/' || c == 'n') {
            add_error("unsupported JSON token; comments and null are not accepted by CivilizationSpec intake");
            return false;
        }
        add_error("unexpected token while parsing value");
        return false;
    }

    [[nodiscard]] bool parse_object(JsonValue& value) {
        value.kind = JsonKind::Object;
        value.object_values.clear();
        if (!consume('{')) {
            return false;
        }
        skip_ws();
        if (peek() == '}') {
            ++pos_;
            return true;
        }
        while (!eof()) {
            skip_ws();
            std::string key;
            if (!parse_string(key)) {
                return false;
            }
            skip_ws();
            if (!consume(':')) {
                return false;
            }
            JsonValue child;
            if (!parse_value(child)) {
                return false;
            }
            if (value.object_values.find(key) != value.object_values.end()) {
                add_error("duplicate object key: " + key);
                return false;
            }
            value.object_values.emplace(std::move(key), std::move(child));
            skip_ws();
            if (peek() == '}') {
                ++pos_;
                return true;
            }
            if (!consume(',')) {
                return false;
            }
            skip_ws();
            if (peek() == '}') {
                add_error("trailing comma in object is not accepted");
                return false;
            }
        }
        add_error("unterminated object");
        return false;
    }

    [[nodiscard]] bool parse_array(JsonValue& value) {
        value.kind = JsonKind::Array;
        value.array_values.clear();
        if (!consume('[')) {
            return false;
        }
        skip_ws();
        if (peek() == ']') {
            ++pos_;
            return true;
        }
        while (!eof()) {
            JsonValue child;
            if (!parse_value(child)) {
                return false;
            }
            value.array_values.push_back(std::move(child));
            skip_ws();
            if (peek() == ']') {
                ++pos_;
                return true;
            }
            if (!consume(',')) {
                return false;
            }
            skip_ws();
            if (peek() == ']') {
                add_error("trailing comma in array is not accepted");
                return false;
            }
        }
        add_error("unterminated array");
        return false;
    }

    [[nodiscard]] bool parse_string(std::string& out) {
        if (!consume('"')) {
            return false;
        }
        out.clear();
        while (!eof()) {
            const char c = input_[pos_++];
            if (c == '"') {
                return true;
            }
            if (static_cast<unsigned char>(c) < 0x20U) {
                add_error("control character inside string");
                return false;
            }
            if (c == '\\') {
                if (eof()) {
                    add_error("unterminated string escape");
                    return false;
                }
                const char esc = input_[pos_++];
                switch (esc) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u':
                    add_error("unicode escapes are not accepted by the narrow CivilizationSpec parser");
                    return false;
                default:
                    add_error("unknown string escape");
                    return false;
                }
            } else {
                out.push_back(c);
            }
        }
        add_error("unterminated string");
        return false;
    }

    [[nodiscard]] bool parse_number(JsonValue& value) {
        const std::size_t start = pos_;
        if (peek() == '-') {
            ++pos_;
        }
        if (eof() || std::isdigit(static_cast<unsigned char>(peek())) == 0) {
            add_error("invalid integer literal");
            return false;
        }
        if (peek() == '0') {
            ++pos_;
            if (!eof() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                add_error("leading zeroes are not accepted in integer literals");
                return false;
            }
        } else {
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                ++pos_;
            }
        }
        bool is_double = false;
        if (!eof() && peek() == '.') {
            is_double = true;
            ++pos_;
            if (eof() || std::isdigit(static_cast<unsigned char>(peek())) == 0) {
                add_error("invalid floating point literal");
                return false;
            }
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                ++pos_;
            }
        }
        if (!eof() && (peek() == 'e' || peek() == 'E')) {
            is_double = true;
            ++pos_;
            if (!eof() && (peek() == '+' || peek() == '-')) {
                ++pos_;
            }
            if (eof() || std::isdigit(static_cast<unsigned char>(peek())) == 0) {
                add_error("invalid floating point exponent");
                return false;
            }
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                ++pos_;
            }
        }
        const std::string token{input_.substr(start, pos_ - start)};
        try {
            std::size_t consumed = 0U;
            if (is_double) {
                const double parsed = std::stod(token, &consumed);
                if (consumed != token.size()) {
                    add_error("invalid floating point literal");
                    return false;
                }
                value.kind = JsonKind::Double;
                value.double_value = parsed;
                return true;
            }
            const long long parsed = std::stoll(token, &consumed, 10);
            if (consumed != token.size()) {
                add_error("invalid integer literal");
                return false;
            }
            value.kind = JsonKind::Integer;
            value.integer_value = static_cast<std::int64_t>(parsed);
            return true;
        } catch (const std::exception&) {
            add_error(is_double ? "floating point literal out of range" : "integer literal out of range");
            return false;
        }
    }

    [[nodiscard]] bool parse_bool(JsonValue& value) {
        if (input_.substr(pos_, 4U) == "true") {
            pos_ += 4U;
            value.kind = JsonKind::Bool;
            value.bool_value = true;
            return true;
        }
        if (input_.substr(pos_, 5U) == "false") {
            pos_ += 5U;
            value.kind = JsonKind::Bool;
            value.bool_value = false;
            return true;
        }
        add_error("invalid boolean literal");
        return false;
    }
};

[[nodiscard]] const JsonValue* object_field(const JsonValue& object, std::string_view field_name) {
    if (object.kind != JsonKind::Object) {
        return nullptr;
    }
    const auto it = object.object_values.find(std::string(field_name));
    if (it == object.object_values.end()) {
        return nullptr;
    }
    return &it->second;
}

void require_string(const JsonValue& object,
                    std::string_view field_name,
                    std::string& target,
                    std::vector<std::string>& errors,
                    bool required = true) {
    const JsonValue* value = object_field(object, field_name);
    if (value == nullptr) {
        if (required) {
            errors.push_back("missing required string field: " + std::string(field_name));
        }
        return;
    }
    if (value->kind != JsonKind::String) {
        errors.push_back("field " + std::string(field_name) + " must be a string");
        return;
    }
    target = value->string_value;
}

void require_int(const JsonValue& object,
                 std::string_view field_name,
                 int& target,
                 std::vector<std::string>& errors) {
    const JsonValue* value = object_field(object, field_name);
    if (value == nullptr) {
        errors.push_back("missing required integer field: " + std::string(field_name));
        return;
    }
    if (value->kind != JsonKind::Integer) {
        errors.push_back("field " + std::string(field_name) + " must be an integer");
        return;
    }
    if (value->integer_value < 0 || value->integer_value > static_cast<std::int64_t>(kOpenEndedYear)) {
        errors.push_back("field " + std::string(field_name) + " is outside the supported year/count range");
        return;
    }
    target = static_cast<int>(value->integer_value);
}

void require_optional_int(const JsonValue& object,
                          std::string_view field_name,
                          int& target,
                          std::vector<std::string>& errors) {
    const JsonValue* value = object_field(object, field_name);
    if (value == nullptr) {
        return;
    }
    if (value->kind != JsonKind::Integer) {
        errors.push_back("field " + std::string(field_name) + " must be an integer");
        return;
    }
    if (value->integer_value < 0 || value->integer_value > 1000000) {
        errors.push_back("field " + std::string(field_name) + " is outside the supported integer range");
        return;
    }
    target = static_cast<int>(value->integer_value);
}

void require_u64(const JsonValue& object,
                 std::string_view field_name,
                 std::uint64_t& target,
                 std::vector<std::string>& errors) {
    const JsonValue* value = object_field(object, field_name);
    if (value == nullptr) {
        errors.push_back("missing required integer field: " + std::string(field_name));
        return;
    }
    if (value->kind != JsonKind::Integer) {
        errors.push_back("field " + std::string(field_name) + " must be an integer");
        return;
    }
    if (value->integer_value < 0) {
        errors.push_back("field " + std::string(field_name) + " must be unsigned");
        return;
    }
    target = static_cast<std::uint64_t>(value->integer_value);
}

void require_string_vector(const JsonValue& object,
                           std::string_view field_name,
                           std::vector<std::string>& target,
                           std::vector<std::string>& errors) {
    const JsonValue* value = object_field(object, field_name);
    if (value == nullptr) {
        errors.push_back("missing required string-array field: " + std::string(field_name));
        return;
    }
    if (value->kind != JsonKind::Array) {
        errors.push_back("field " + std::string(field_name) + " must be an array");
        return;
    }
    target.clear();
    for (std::size_t i = 0U; i < value->array_values.size(); ++i) {
        const JsonValue& item = value->array_values[i];
        if (item.kind != JsonKind::String) {
            std::ostringstream out;
            out << "field " << field_name << '[' << i << "] must be a string";
            errors.push_back(out.str());
            continue;
        }
        target.push_back(item.string_value);
    }
}

void require_optional_string_vector(const JsonValue& object,
                                    std::string_view field_name,
                                    std::vector<std::string>& target,
                                    std::vector<std::string>& errors) {
    const JsonValue* value = object_field(object, field_name);
    if (value == nullptr) {
        target.clear();
        return;
    }
    if (value->kind != JsonKind::Array) {
        errors.push_back("field " + std::string(field_name) + " must be an array");
        return;
    }
    target.clear();
    for (std::size_t i = 0U; i < value->array_values.size(); ++i) {
        const JsonValue& item = value->array_values[i];
        if (item.kind != JsonKind::String) {
            std::ostringstream out;
            out << "field " << field_name << '[' << i << "] must be a string";
            errors.push_back(out.str());
            continue;
        }
        target.push_back(item.string_value);
    }
}

[[nodiscard]] bool known_profile_field(std::string_view field_name) {
    return field_name == "target_depth" || field_name == "future_modules";
}

[[nodiscard]] std::optional<CivilizationSpecProfile> optional_profile_from_json_object(
    const JsonValue& object,
    std::vector<std::string>& errors
) {
    const JsonValue* value = object_field(object, "profile");
    if (value == nullptr) {
        return std::nullopt;
    }
    if (value->kind != JsonKind::Object) {
        errors.push_back("field profile must be an object");
        return std::nullopt;
    }
    for (const auto& [key, child] : value->object_values) {
        (void)child;
        if (!known_profile_field(key)) {
            errors.push_back("field profile has unknown field: " + key);
        }
    }
    CivilizationSpecProfile profile;
    require_string(*value, "target_depth", profile.target_depth, errors, false);
    require_optional_string_vector(*value, "future_modules", profile.future_modules, errors);
    return profile;
}

[[nodiscard]] bool known_spec_field(std::string_view field_name) {
    static const std::set<std::string> known = {
        "id", "display_name", "description", "earliest_year", "latest_year",
        "geographic_features", "environmental_pressures", "major_sites",
        "economic_pressures", "trade_goods", "institution_archetypes",
        "social_actor_archetypes", "authority_conflicts",
        "religious_or_mythic_archetypes", "ritual_pressures",
        "writing_system_archetypes", "recordkeeping_styles", "artifact_media",
        "evidence_distortion_modes", "mystery_archetypes",
        "target_hidden_entity_count", "target_hidden_event_count",
        "target_public_artifact_count", "target_mystery_count", "seed",
        "tags", "profile"
    };
    return known.find(std::string(field_name)) != known.end();
}

[[nodiscard]] bool known_catalog_field(std::string_view field_name) {
    return field_name == "schema_version" || field_name == "catalog_id" || field_name == "civilizations" || field_name == "fragments";
}

[[nodiscard]] CivilizationSpec spec_from_json_object(const JsonValue& object, std::vector<std::string>& errors, std::size_t index) {
    CivilizationSpec spec;
    if (object.kind != JsonKind::Object) {
        errors.push_back("catalog.civilizations[" + std::to_string(index) + "] must be an object");
        return spec;
    }
    for (const auto& [key, value] : object.object_values) {
        (void)value;
        if (!known_spec_field(key)) {
            errors.push_back("catalog.civilizations[" + std::to_string(index) + "] has unknown field: " + key);
        }
    }

    require_string(object, "id", spec.id, errors);
    require_string(object, "display_name", spec.display_name, errors);
    require_string(object, "description", spec.description, errors);
    require_optional_string_vector(object, "tags", spec.tags, errors);
    spec.profile = optional_profile_from_json_object(object, errors);
    require_int(object, "earliest_year", spec.earliest_year, errors);
    require_int(object, "latest_year", spec.latest_year, errors);
    require_string_vector(object, "geographic_features", spec.geographic_features, errors);
    require_string_vector(object, "environmental_pressures", spec.environmental_pressures, errors);
    require_string_vector(object, "major_sites", spec.major_sites, errors);
    require_string_vector(object, "economic_pressures", spec.economic_pressures, errors);
    require_string_vector(object, "trade_goods", spec.trade_goods, errors);
    require_string_vector(object, "institution_archetypes", spec.institution_archetypes, errors);
    require_string_vector(object, "social_actor_archetypes", spec.social_actor_archetypes, errors);
    require_string_vector(object, "authority_conflicts", spec.authority_conflicts, errors);
    require_string_vector(object, "religious_or_mythic_archetypes", spec.religious_or_mythic_archetypes, errors);
    require_string_vector(object, "ritual_pressures", spec.ritual_pressures, errors);
    require_string_vector(object, "writing_system_archetypes", spec.writing_system_archetypes, errors);
    require_string_vector(object, "recordkeeping_styles", spec.recordkeeping_styles, errors);
    require_string_vector(object, "artifact_media", spec.artifact_media, errors);
    require_string_vector(object, "evidence_distortion_modes", spec.evidence_distortion_modes, errors);
    require_string_vector(object, "mystery_archetypes", spec.mystery_archetypes, errors);
    require_int(object, "target_hidden_entity_count", spec.target_hidden_entity_count, errors);
    require_int(object, "target_hidden_event_count", spec.target_hidden_event_count, errors);
    require_int(object, "target_public_artifact_count", spec.target_public_artifact_count, errors);
    require_int(object, "target_mystery_count", spec.target_mystery_count, errors);
    require_u64(object, "seed", spec.seed, errors);

    return spec;
}

[[nodiscard]] bool known_patch_field(std::string_view field_name) {
    return field_name == "path" || field_name == "strategy" || field_name == "value";
}

[[nodiscard]] bool known_fragment_field(std::string_view field_name) {
    static const std::set<std::string> known = {
        "schema_version", "kind", "id", "title", "category", "priority",
        "tags", "requires_tags", "excludes_tags", "patches"
    };
    return known.find(std::string(field_name)) != known.end();
}

[[nodiscard]] SpecPatchValue patch_value_from_json_value(const JsonValue& value,
                                                         std::vector<std::string>& errors,
                                                         std::string_view context) {
    SpecPatchValue patch_value;
    if (value.kind == JsonKind::Bool) {
        patch_value.kind = SpecPatchValue::Kind::Bool;
        patch_value.bool_value = value.bool_value;
        return patch_value;
    }
    if (value.kind == JsonKind::Integer) {
        patch_value.kind = SpecPatchValue::Kind::Int;
        if (value.integer_value < static_cast<std::int64_t>(std::numeric_limits<int>::min()) ||
            value.integer_value > static_cast<std::int64_t>(std::numeric_limits<int>::max())) {
            errors.push_back(std::string(context) + ".value integer is out of int range");
            return patch_value;
        }
        patch_value.int_value = static_cast<int>(value.integer_value);
        return patch_value;
    }
    if (value.kind == JsonKind::Double) {
        patch_value.kind = SpecPatchValue::Kind::Double;
        patch_value.double_value = value.double_value;
        return patch_value;
    }
    if (value.kind == JsonKind::String) {
        patch_value.kind = SpecPatchValue::Kind::String;
        patch_value.string_value = value.string_value;
        return patch_value;
    }
    if (value.kind == JsonKind::Array) {
        patch_value.kind = SpecPatchValue::Kind::StringList;
        for (std::size_t i = 0U; i < value.array_values.size(); ++i) {
            const JsonValue& item = value.array_values[i];
            if (item.kind != JsonKind::String) {
                std::ostringstream out;
                out << context << ".value[" << i << "] must be a string";
                errors.push_back(out.str());
                continue;
            }
            patch_value.string_list_value.push_back(item.string_value);
        }
        return patch_value;
    }
    errors.push_back(std::string(context) + ".value has unsupported object type");
    return patch_value;
}

[[nodiscard]] SpecPatch patch_from_json_object(const JsonValue& object,
                                               std::vector<std::string>& errors,
                                               std::string_view context) {
    SpecPatch patch;
    if (object.kind != JsonKind::Object) {
        errors.push_back(std::string(context) + " must be an object");
        return patch;
    }
    for (const auto& [key, child] : object.object_values) {
        (void)child;
        if (!known_patch_field(key)) {
            errors.push_back(std::string(context) + " has unknown field: " + key);
        }
    }
    require_string(object, "path", patch.path, errors);
    std::string strategy_text = "replace";
    require_string(object, "strategy", strategy_text, errors, false);
    const std::optional<PatchStrategy> strategy = parse_patch_strategy(strategy_text);
    if (strategy.has_value()) {
        patch.strategy = *strategy;
    } else {
        errors.push_back(std::string(context) + ".strategy is unknown: " + strategy_text);
    }
    const JsonValue* value = object_field(object, "value");
    if (value == nullptr) {
        errors.push_back(std::string(context) + " missing required field: value");
    } else {
        patch.value = patch_value_from_json_value(*value, errors, context);
    }
    return patch;
}

[[nodiscard]] CivilizationSpecFragment fragment_from_json_object(const JsonValue& object,
                                                                 std::vector<std::string>& errors,
                                                                 std::size_t index) {
    CivilizationSpecFragment fragment;
    const std::string context = "catalog.fragments[" + std::to_string(index) + "]";
    if (object.kind != JsonKind::Object) {
        errors.push_back(context + " must be an object");
        return fragment;
    }
    for (const auto& [key, value] : object.object_values) {
        (void)value;
        if (!known_fragment_field(key)) {
            errors.push_back(context + " has unknown field: " + key);
        }
    }
    std::string kind;
    require_string(object, "kind", kind, errors);
    if (!kind.empty() && kind != "civilization_fragment") {
        errors.push_back(context + ".kind must be civilization_fragment");
    }
    require_string(object, "schema_version", fragment.schema_version, errors, false);
    require_string(object, "id", fragment.id, errors);
    require_string(object, "title", fragment.title, errors);
    std::string category_text;
    require_string(object, "category", category_text, errors);
    const std::optional<CivilizationFragmentCategory> category = parse_civilization_fragment_category(category_text);
    if (category.has_value()) {
        fragment.category = *category;
    } else if (!category_text.empty()) {
        errors.push_back(context + ".category is unknown: " + category_text);
    }
    require_optional_int(object, "priority", fragment.priority, errors);
    require_optional_string_vector(object, "tags", fragment.tags, errors);
    require_optional_string_vector(object, "requires_tags", fragment.requires_tags, errors);
    require_optional_string_vector(object, "excludes_tags", fragment.excludes_tags, errors);

    const JsonValue* patches = object_field(object, "patches");
    if (patches != nullptr) {
        if (patches->kind != JsonKind::Array) {
            errors.push_back(context + ".patches must be an array");
        } else {
            for (std::size_t i = 0U; i < patches->array_values.size(); ++i) {
                std::ostringstream patch_context;
                patch_context << context << ".patches[" << i << "]";
                fragment.patches.push_back(patch_from_json_object(patches->array_values[i], errors, patch_context.str()));
            }
        }
    }
    return fragment;
}


[[nodiscard]] bool is_lowercase_snake_case(std::string_view value) {
    if (value.empty()) {
        return false;
    }
    if (value.front() == '_' || value.back() == '_') {
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

void validate_vector_count(const std::vector<std::string>& values,
                           std::string_view field_name,
                           std::size_t min_count,
                           std::size_t max_count,
                           CivilizationSpecValidationResult& result) {
    if (values.size() < min_count || values.size() > max_count) {
        std::ostringstream out;
        out << field_name << " must contain " << min_count << '-' << max_count << " entries; found " << values.size();
        result.errors.push_back(out.str());
    }
    std::set<std::string> seen;
    for (const std::string& value : values) {
        if (value.empty()) {
            result.errors.push_back(std::string(field_name) + " contains an empty value");
        }
        if (!seen.insert(value).second) {
            result.errors.push_back(std::string(field_name) + " contains duplicate value " + value);
        }
    }
}

void validate_count_range(int value,
                          std::string_view field_name,
                          int min_value,
                          int max_value,
                          CivilizationSpecValidationResult& result) {
    if (value < min_value || value > max_value) {
        std::ostringstream out;
        out << field_name << " must be in range " << min_value << '-' << max_value << "; found " << value;
        result.errors.push_back(out.str());
    }
}

void merge_validation_result(CivilizationSpecValidationResult& target,
                             const CivilizationSpecValidationResult& source) {
    target.errors.insert(target.errors.end(), source.errors.begin(), source.errors.end());
    target.warnings.insert(target.warnings.end(), source.warnings.begin(), source.warnings.end());
}

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_spec_tags(const CivilizationSpec& spec) {
    CivilizationSpecValidationResult result;
    std::set<std::string> seen;
    for (const std::string& tag : spec.tags) {
        if (tag.empty()) {
            result.errors.push_back(spec.id + ".tags contains an empty value");
            continue;
        }
        if (!seen.insert(tag).second) {
            result.warnings.push_back(spec.id + ".tags contains duplicate value: " + tag);
        }
        if (!is_lowercase_snake_case(tag)) {
            result.warnings.push_back(spec.id + ".tags contains non-standard tag format: " + tag);
        }
    }
    result.valid = result.errors.empty();
    return result;
}

[[nodiscard]] bool is_known_target_depth(std::string_view target_depth) {
    static const std::set<std::string> known = {"prototype", "mvp", "deep_archive"};
    return known.find(std::string(target_depth)) != known.end();
}

[[nodiscard]] bool is_known_future_module(std::string_view module) {
    static const std::set<std::string> known = {
        "fragment_composition",
        "language_drift",
        "artifact_voice",
        "hidden_mutation",
        "public_archive",
        "interpreter_model",
        "mystery_preservation",
        "chronology",
        "originality"
    };
    return known.find(std::string(module)) != known.end();
}

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_spec_profile(const CivilizationSpec& spec) {
    CivilizationSpecValidationResult result;
    if (!spec.profile.has_value()) {
        result.valid = true;
        return result;
    }
    const CivilizationSpecProfile& profile = *spec.profile;
    if (profile.target_depth.empty()) {
        result.errors.push_back(spec.id + ".profile.target_depth is empty");
    } else if (!is_known_target_depth(profile.target_depth)) {
        result.warnings.push_back(spec.id + ".profile.target_depth is unknown: " + profile.target_depth);
    }
    std::set<std::string> seen_modules;
    for (const std::string& module : profile.future_modules) {
        if (module.empty()) {
            result.errors.push_back(spec.id + ".profile.future_modules contains an empty value");
            continue;
        }
        if (!seen_modules.insert(module).second) {
            result.warnings.push_back(spec.id + ".profile.future_modules contains duplicate value: " + module);
        }
        if (!is_known_future_module(module)) {
            result.warnings.push_back(spec.id + ".profile.future_modules contains unknown value: " + module);
        }
    }
    result.valid = result.errors.empty();
    return result;
}

[[nodiscard]] bool contains_actor(const CivilizationSpec& spec, const std::string& actor) {
    auto contains = [&](const std::vector<std::string>& values) {
        return std::find(values.begin(), values.end(), actor) != values.end();
    };
    return contains(spec.institution_archetypes) || contains(spec.social_actor_archetypes) || contains(spec.major_sites);
}

[[nodiscard]] std::string join_values(const std::vector<std::string>& values) {
    std::ostringstream out;
    for (std::size_t i = 0U; i < values.size(); ++i) {
        if (i > 0U) {
            out << ", ";
        }
        out << values[i];
    }
    return out.str();
}

[[nodiscard]] std::string format_tags_for_summary(const std::vector<std::string>& tags) {
    if (tags.empty()) {
        return "none";
    }
    return join_values(tags);
}

[[nodiscard]] std::string format_profile_for_summary(const std::optional<CivilizationSpecProfile>& profile) {
    if (!profile.has_value()) {
        return "unspecified";
    }
    std::ostringstream out;
    out << "target_depth=" << profile->target_depth;
    if (profile->future_modules.empty()) {
        out << "; future_modules=none";
    } else {
        out << "; future_modules=" << join_values(profile->future_modules);
    }
    return out.str();
}

[[nodiscard]] std::map<std::string, std::size_t> count_catalog_tags(const CivilizationSpecCatalog& catalog) {
    std::map<std::string, std::size_t> counts;
    for (const CivilizationSpec& spec : catalog.civilizations) {
        std::set<std::string> unique_for_spec;
        for (const std::string& tag : spec.tags) {
            if (!tag.empty()) {
                unique_for_spec.insert(tag);
            }
        }
        for (const std::string& tag : unique_for_spec) {
            ++counts[tag];
        }
    }
    return counts;
}

} // namespace

[[nodiscard]] CivilizationSpecLoadResult load_civilization_specs_from_json_text(std::string_view json_text) {
    CivilizationSpecLoadResult result;
    JsonParser parser(json_text);
    const JsonValue root = parser.parse();
    if (!parser.errors().empty()) {
        result.errors = parser.errors();
        return result;
    }
    if (root.kind != JsonKind::Object) {
        result.errors.push_back("CivilizationSpec catalog root must be an object");
        return result;
    }
    for (const auto& [key, value] : root.object_values) {
        (void)value;
        if (!known_catalog_field(key)) {
            result.errors.push_back("catalog has unknown field: " + key);
        }
    }
    require_string(root, "schema_version", result.catalog.schema_version, result.errors, false);
    require_string(root, "catalog_id", result.catalog.catalog_id, result.errors, false);
    const JsonValue* civilizations = object_field(root, "civilizations");
    if (civilizations == nullptr) {
        result.errors.push_back("missing required array field: civilizations");
        return result;
    }
    if (civilizations->kind != JsonKind::Array) {
        result.errors.push_back("field civilizations must be an array");
        return result;
    }
    for (std::size_t i = 0U; i < civilizations->array_values.size(); ++i) {
        result.catalog.civilizations.push_back(spec_from_json_object(civilizations->array_values[i], result.errors, i));
    }

    const JsonValue* fragments = object_field(root, "fragments");
    if (fragments != nullptr) {
        if (fragments->kind != JsonKind::Array) {
            result.errors.push_back("field fragments must be an array");
            return result;
        }
        for (std::size_t i = 0U; i < fragments->array_values.size(); ++i) {
            result.catalog.fragments.push_back(fragment_from_json_object(fragments->array_values[i], result.errors, i));
        }
    }
    return result;
}

[[nodiscard]] CivilizationSpecLoadResult load_civilization_specs_from_json_file(const std::string& path) {
    CivilizationSpecLoadResult result;
    std::ifstream input(path);
    if (!input) {
        result.errors.push_back("could not open CivilizationSpec file: " + path);
        return result;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad()) {
        result.errors.push_back("could not read CivilizationSpec file: " + path);
        return result;
    }
    return load_civilization_specs_from_json_text(buffer.str());
}

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_spec(const CivilizationSpec& spec) {
    CivilizationSpecValidationResult result;
    if (spec.id.empty()) {
        result.errors.push_back("spec.id is empty");
    } else if (!is_lowercase_snake_case(spec.id)) {
        result.errors.push_back("spec.id must be lowercase snake_case: " + spec.id);
    }
    if (spec.display_name.empty()) {
        result.errors.push_back("spec.display_name is empty");
    }
    if (spec.description.empty()) {
        result.errors.push_back("spec.description is empty");
    }
    if (spec.earliest_year >= spec.latest_year) {
        result.errors.push_back("spec.earliest_year must be less than spec.latest_year");
    }

    validate_vector_count(spec.geographic_features, "geographic_features", 4U, 8U, result);
    validate_vector_count(spec.environmental_pressures, "environmental_pressures", 3U, 7U, result);
    validate_vector_count(spec.major_sites, "major_sites", 4U, 8U, result);
    validate_vector_count(spec.economic_pressures, "economic_pressures", 3U, 7U, result);
    validate_vector_count(spec.trade_goods, "trade_goods", 4U, 10U, result);
    validate_vector_count(spec.institution_archetypes, "institution_archetypes", 4U, 8U, result);
    validate_vector_count(spec.social_actor_archetypes, "social_actor_archetypes", 2U, 8U, result);
    validate_vector_count(spec.authority_conflicts, "authority_conflicts", 2U, 5U, result);
    validate_vector_count(spec.religious_or_mythic_archetypes, "religious_or_mythic_archetypes", 3U, 7U, result);
    validate_vector_count(spec.ritual_pressures, "ritual_pressures", 2U, 6U, result);
    validate_vector_count(spec.writing_system_archetypes, "writing_system_archetypes", 2U, 5U, result);
    validate_vector_count(spec.recordkeeping_styles, "recordkeeping_styles", 4U, 8U, result);
    validate_vector_count(spec.artifact_media, "artifact_media", 4U, 8U, result);
    validate_vector_count(spec.evidence_distortion_modes, "evidence_distortion_modes", 4U, 8U, result);
    validate_vector_count(spec.mystery_archetypes, "mystery_archetypes", 4U, 8U, result);

    validate_count_range(spec.target_hidden_entity_count, "target_hidden_entity_count", 15, 25, result);
    validate_count_range(spec.target_hidden_event_count, "target_hidden_event_count", 20, 35, result);
    validate_count_range(spec.target_public_artifact_count, "target_public_artifact_count", 12, 20, result);
    validate_count_range(spec.target_mystery_count, "target_mystery_count", 4, 6, result);

    merge_validation_result(result, validate_civilization_spec_tags(spec));
    merge_validation_result(result, validate_civilization_spec_profile(spec));

    std::set<std::string> conflicts;
    for (const std::string& conflict : spec.authority_conflicts) {
        if (!conflicts.insert(conflict).second) {
            result.errors.push_back("authority_conflicts contains duplicate conflict " + conflict);
        }
        const std::string token = "_vs_";
        const std::size_t first = conflict.find(token);
        if (first == std::string::npos || conflict.find(token, first + token.size()) != std::string::npos) {
            result.errors.push_back("authority conflict must contain exactly one _vs_: " + conflict);
            continue;
        }
        const std::string left = conflict.substr(0U, first);
        const std::string right = conflict.substr(first + token.size());
        if (left.empty() || right.empty()) {
            result.errors.push_back("authority conflict has an empty participant: " + conflict);
            continue;
        }
        if (!contains_actor(spec, left)) {
            result.errors.push_back("authority conflict left participant is unknown: " + left);
        }
        if (!contains_actor(spec, right)) {
            result.errors.push_back("authority conflict right participant is unknown: " + right);
        }
    }

    result.valid = result.errors.empty();
    return result;
}

[[nodiscard]] CivilizationSpecValidationResult validate_civilization_catalog(const CivilizationSpecCatalog& catalog) {
    CivilizationSpecValidationResult result;
    if (catalog.civilizations.empty()) {
        result.errors.push_back("catalog.civilizations must contain at least one CivilizationSpec");
    }
    std::set<std::string> ids;
    for (const CivilizationSpec& spec : catalog.civilizations) {
        if (!spec.id.empty() && !ids.insert(spec.id).second) {
            result.errors.push_back("duplicate civilization id: " + spec.id);
        }
        CivilizationSpecValidationResult spec_result = validate_civilization_spec(spec);
        for (const std::string& error : spec_result.errors) {
            result.errors.push_back(spec.id.empty() ? error : (spec.id + ": " + error));
        }
        for (const std::string& warning : spec_result.warnings) {
            result.warnings.push_back(spec.id.empty() ? warning : (spec.id + ": " + warning));
        }
    }
    CivilizationSpecValidationResult fragment_result = validate_civilization_fragments(catalog);
    for (const std::string& error : fragment_result.errors) {
        result.errors.push_back(error);
    }
    for (const std::string& warning : fragment_result.warnings) {
        result.warnings.push_back(warning);
    }
    result.valid = result.errors.empty();
    return result;
}

[[nodiscard]] const CivilizationSpec* find_civilization_spec(const CivilizationSpecCatalog& catalog,
                                                             std::string_view civilization_id) {
    const auto it = std::find_if(catalog.civilizations.begin(), catalog.civilizations.end(), [&](const CivilizationSpec& spec) {
        return spec.id == civilization_id;
    });
    if (it == catalog.civilizations.end()) {
        return nullptr;
    }
    return &(*it);
}

[[nodiscard]] std::string format_civilization_spec_summary(const CivilizationSpec& spec) {
    std::ostringstream out;
    out << "CivilizationSpec: " << spec.id << "\n";
    out << "- display_name: " << spec.display_name << "\n";
    out << "- description: " << spec.description << "\n";
    out << "- tags: " << format_tags_for_summary(spec.tags) << "\n";
    out << "- profile: " << format_profile_for_summary(spec.profile) << "\n";
    out << "- year_range: " << spec.earliest_year << '-' << spec.latest_year << "\n";
    out << "- major_sites: " << join_values(spec.major_sites) << "\n";
    out << "- institution_archetypes: " << join_values(spec.institution_archetypes) << "\n";
    out << "- social_actor_archetypes: " << join_values(spec.social_actor_archetypes) << "\n";
    out << "- authority_conflicts: " << join_values(spec.authority_conflicts) << "\n";
    out << "- artifact_media: " << join_values(spec.artifact_media) << "\n";
    out << "- evidence_distortion_modes: " << join_values(spec.evidence_distortion_modes) << "\n";
    out << "- mystery_archetypes: " << join_values(spec.mystery_archetypes) << "\n";
    out << "- target_hidden_entity_count: " << spec.target_hidden_entity_count << "\n";
    out << "- target_hidden_event_count: " << spec.target_hidden_event_count << "\n";
    out << "- target_public_artifact_count: " << spec.target_public_artifact_count << "\n";
    out << "- target_mystery_count: " << spec.target_mystery_count << "\n";
    out << "- seed: " << spec.seed << "\n";
    return out.str();
}

[[nodiscard]] std::string format_civilization_catalog_validation(const CivilizationSpecCatalog& catalog,
                                                                 const CivilizationSpecValidationResult& validation) {
    std::size_t valid_count = 0U;
    for (const CivilizationSpec& spec : catalog.civilizations) {
        if (validate_civilization_spec(spec).valid) {
            ++valid_count;
        }
    }
    std::ostringstream out;
    out << "CivilizationSpec validation:\n";
    out << "- loaded: " << catalog.civilizations.size() << "\n";
    out << "- valid: " << valid_count << "\n";
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

[[nodiscard]] std::string format_civilization_catalog_list(const CivilizationSpecCatalog& catalog,
                                                           const CivilizationSpecValidationResult& validation) {
    std::ostringstream out;
    out << "CivilizationSpec catalog:\n";
    out << "- loaded: " << catalog.civilizations.size() << "\n";
    out << "- errors: " << validation.errors.size() << "\n";
    for (const CivilizationSpec& spec : catalog.civilizations) {
        out << "- " << spec.id << ": " << spec.display_name << ", years " << spec.earliest_year << '-' << spec.latest_year << "\n";
    }
    for (const std::string& error : validation.errors) {
        out << "  error: " << error << "\n";
    }
    return out.str();
}


[[nodiscard]] std::vector<std::string> collect_civilization_catalog_tags(const CivilizationSpecCatalog& catalog) {
    std::vector<std::string> tags;
    const std::map<std::string, std::size_t> counts = count_catalog_tags(catalog);
    tags.reserve(counts.size());
    for (const auto& [tag, count] : counts) {
        (void)count;
        tags.push_back(tag);
    }
    return tags;
}

[[nodiscard]] std::vector<const CivilizationSpec*> find_civilization_specs_by_tag(const CivilizationSpecCatalog& catalog,
                                                                                  std::string_view tag) {
    std::vector<const CivilizationSpec*> specs;
    for (const CivilizationSpec& spec : catalog.civilizations) {
        if (std::find(spec.tags.begin(), spec.tags.end(), tag) != spec.tags.end()) {
            specs.push_back(&spec);
        }
    }
    return specs;
}

[[nodiscard]] std::string format_civilization_catalog_tags(const CivilizationSpecCatalog& catalog) {
    const std::map<std::string, std::size_t> counts = count_catalog_tags(catalog);
    std::ostringstream out;
    out << "CivilizationSpec tags:\n";
    if (counts.empty()) {
        out << "- none\n";
        return out.str();
    }
    for (const auto& [tag, count] : counts) {
        out << "- " << tag << ": " << count << " specs\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_civilization_specs_by_tag(const CivilizationSpecCatalog& catalog,
                                                           std::string_view tag) {
    const std::vector<const CivilizationSpec*> specs = find_civilization_specs_by_tag(catalog, tag);
    std::ostringstream out;
    out << "Civilizations tagged " << tag << ":\n";
    if (specs.empty()) {
        out << "- none\n";
        return out.str();
    }
    for (const CivilizationSpec* spec : specs) {
        out << "- " << spec->id << ": " << spec->display_name << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_civilization_catalog_tag_validation(const CivilizationSpecCatalog& catalog,
                                                                     const CivilizationSpecValidationResult& validation) {
    std::ostringstream out;
    out << "CivilizationSpec tag validation:\n";
    out << "- loaded specs: " << catalog.civilizations.size() << "\n";
    out << "- unique tags: " << collect_civilization_catalog_tags(catalog).size() << "\n";
    out << "- warnings: " << validation.warnings.size() << "\n";
    out << "- errors: " << validation.errors.size() << "\n";
    for (const std::string& warning : validation.warnings) {
        out << "  warning: " << warning << "\n";
    }
    for (const std::string& error : validation.errors) {
        out << "  error: " << error << "\n";
    }
    return out.str();
}

} // namespace archive
