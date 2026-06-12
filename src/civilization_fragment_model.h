#pragma once
#include "archive_common.h"

namespace archive {

enum class CivilizationFragmentCategory {
    Geography,
    Ecology,
    Polity,
    Economy,
    Religion,
    Culture,
    Technology,
    ArtifactBias,
    HistoricalDensity,
    TruthPolicy,
    MysteryPolicy,
    StrangenessOverlay,
    PreservationModel,
    CollapseModel,
    Constraint,
};

enum class PatchStrategy {
    Replace,
    SetIfAbsent,
    AppendUnique,
    Add,
    Scale,
    Clamp,
    Min,
    Max,
};

struct SpecPatchValue {
    enum class Kind {
        Bool,
        Int,
        Double,
        String,
        StringList,
    };

    Kind kind = Kind::String;
    bool bool_value = false;
    int int_value = 0;
    double double_value = 0.0;
    std::string string_value;
    std::vector<std::string> string_list_value;
};

struct SpecPatch {
    std::string path;
    PatchStrategy strategy = PatchStrategy::Replace;
    SpecPatchValue value;
};

struct CivilizationSpecFragment {
    std::string id;
    std::string schema_version = "1.2";
    std::string title;
    CivilizationFragmentCategory category = CivilizationFragmentCategory::Culture;

    std::vector<std::string> tags;
    std::vector<std::string> requires_tags;
    std::vector<std::string> excludes_tags;

    int priority = 100;

    std::vector<SpecPatch> patches;
};

} // namespace archive
