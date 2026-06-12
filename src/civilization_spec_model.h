#pragma once
#include "archive_common.h"
#include "civilization_fragment_model.h"

namespace archive {

struct CivilizationSpecProfile {
    std::string target_depth = "prototype";
    std::vector<std::string> future_modules;
};

struct CivilizationSpec {
    std::string id;
    std::string display_name;
    std::string description;

    std::vector<std::string> tags;
    std::optional<CivilizationSpecProfile> profile;

    int earliest_year = 0;
    int latest_year = 0;

    std::vector<std::string> geographic_features;
    std::vector<std::string> environmental_pressures;
    std::vector<std::string> major_sites;

    std::vector<std::string> economic_pressures;
    std::vector<std::string> trade_goods;

    std::vector<std::string> institution_archetypes;
    std::vector<std::string> social_actor_archetypes;
    std::vector<std::string> authority_conflicts;

    std::vector<std::string> religious_or_mythic_archetypes;
    std::vector<std::string> ritual_pressures;

    std::vector<std::string> writing_system_archetypes;
    std::vector<std::string> recordkeeping_styles;

    std::vector<std::string> artifact_media;
    std::vector<std::string> evidence_distortion_modes;
    std::vector<std::string> mystery_archetypes;

    int target_hidden_entity_count = 18;
    int target_hidden_event_count = 24;
    int target_public_artifact_count = 16;
    int target_mystery_count = 5;

    std::uint64_t seed = 0;
};

struct CivilizationSpecCatalog {
    std::string schema_version;
    std::string catalog_id;
    std::vector<CivilizationSpec> civilizations;
    std::vector<CivilizationSpecFragment> fragments;
};

struct CivilizationSpecLoadResult {
    CivilizationSpecCatalog catalog;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    [[nodiscard]] bool ok() const {
        return errors.empty();
    }
};

struct CivilizationSpecValidationResult {
    bool valid = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

} // namespace archive
