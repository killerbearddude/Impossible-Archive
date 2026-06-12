#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <map>
#include <set>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace archive {

constexpr int kOpenEndedYear = 999999;

struct Interval {
    int start_year = 0;
    int end_year = kOpenEndedYear;

    [[nodiscard]] bool contains(int year) const {
        return start_year <= year && year <= end_year;
    }
};

// Access levels are ordered from least to most privileged.
// All user-facing formatting should select visible objects first and only then
// render text; do not render full state and redact with string replacement.
enum class AccessLevel {
    Public = 0,
    Scholar = 1,
    Curator = 2,
    Canon = 3,
    Debug = 4,
};


enum class HiddenMutationArtifactCandidateShape {
    AdministrativeDocket,
    RitualNotice,
    ScholarFragment,
};



// Trace for GenerationTarget values resolved from a spec-bootstrapped runtime.
// It is optional so fixed-fixture generation remains unchanged.
struct SpecGenerationTargetSource {
    std::string civilization_id;
    std::string target_kind;
    std::string target_key;
    std::vector<std::string> source_entity_ids;
    std::vector<std::string> source_event_ids;
};

// Archive-aware binding for generation. The resolver prevents target_topic from
// being mere wording: candidates should link only to mysteries/entities/claims
// explicitly resolved here.
struct GenerationTarget {
    std::string topic;
    std::optional<std::string> mystery_id;
    std::vector<std::string> entity_ids;
    std::vector<std::string> claim_ids;
    int earliest_valid_year = 0;
    int latest_valid_year = kOpenEndedYear;
    std::optional<SpecGenerationTargetSource> spec_source;
};


[[nodiscard]] std::string year_text(int year);
[[nodiscard]] std::string interval_text(const Interval& interval);
[[nodiscard]] int rank(AccessLevel access);
[[nodiscard]] bool can_view(AccessLevel current, AccessLevel required);
[[nodiscard]] std::string to_string(AccessLevel access);
[[nodiscard]] AccessLevel parse_access(std::string_view text);
[[nodiscard]] double clamp01(double value);
[[nodiscard]] double random_between(std::mt19937_64& rng, double low, double high);
void add_unique_string(std::vector<std::string>& values, const std::string& value);
[[nodiscard]] bool has_prefix(std::string_view text, std::string_view prefix);
[[nodiscard]] bool contains_substr(const std::string& haystack, const std::string& needle);
[[nodiscard]] std::uint64_t parse_u64(const std::string& text);
[[nodiscard]] int parse_int(const std::string& text);

} // namespace archive
