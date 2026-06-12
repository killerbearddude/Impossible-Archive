/*
 * Replay and validation serialization. Keep deterministic ordering stable for mutation/rollback tests.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

[[nodiscard]] std::string format_validation(const ArchiveEngineState& state, AccessLevel access) {
    std::ostringstream out;
    const std::vector<std::string> errors = validate_full_state(state);
    out << "Validation summary:\n";
    if (errors.empty()) {
        out << "- PASS: hidden causality, required artifact metadata, typed evidence modifiers, structured metadata IDs, metadata temporal availability, typed claim semantics, detected contradictions, cross-references, contradiction causes, explained anachronisms, and hidden mutation provenance, and inert evidence potentials are valid.\n";
    } else {
        out << "- FAIL:\n";
        for (const std::string& error : errors) {
            out << "  - " << error << "\n";
        }
    }

    if (can_view(access, AccessLevel::Debug)) {
        out << "- Debug validation checks executed: "
            << "cause order, participant lifespan, artifact metadata, typed evidence modifiers, structured metadata ID validation, metadata temporal availability, typed claim semantics, automatic contradiction detection, cross-reference integrity, contradiction cause enforcement, anachronism mediation, hidden mutation provenance, and EvidencePotential source/range/public-safety validation.\n";
    }

    return out.str();
}

[[nodiscard]] std::string serialize_for_replay_test(const ArchiveEngineState& state) {
    std::ostringstream out;
    out << "seed=" << state.seed << "\n";
    out << "seeded_calendar_dispute=" << (state.include_seeded_calendar_dispute ? "true" : "false") << "\n";
    if (state.civilization_source.has_value()) {
        const CivilizationRuntimeSource& source = *state.civilization_source;
        out << "S|" << source.civilization_id << "|" << source.display_name << "|" << source.seed
            << "|" << source.earliest_year << "|" << source.latest_year << "|"
            << source.catalog_id << "|" << source.schema_version << "\n";
    }
    for (const auto& [id, entity] : state.hidden_truth.entities()) {
        out << "E|" << id << "|" << to_string(entity.type) << "|" << entity.canonical_name << "|" << interval_text(entity.existence_interval) << "\n";
    }
    for (const auto& [id, event] : state.hidden_truth.events()) {
        out << "V|" << id << "|" << event.start_year << "|" << event.end_year << "|" << event.title << "\n";
    }
    for (const auto& [id, artifact] : state.public_archive.artifacts()) {
        out << "A|" << id << "|" << to_string(artifact.type) << "|" << to_string(artifact.voice_register) << "|" << artifact.true_creation_year << "|"
            << artifact.claimed_creation_year << "|" << artifact.discovery_year
            << "|" << evidence_modifier_list_text(artifact)
            << "|" << std::fixed << std::setprecision(6) << artifact.reliability_score << "|" << artifact.public_text << "\n";
    }
    for (const Discovery& discovery : state.discovery_log) {
        out << "D|" << discovery.id << "|" << discovery.artifact_id << "|" << discovery.discovery_year
            << "|" << discovery.site_id << "|" << to_string(discovery.min_access) << "\n";
    }
    for (const auto& [id, contradiction] : state.public_archive.contradictions()) {
        out << "C|" << id << "|" << contradiction.detector_rule << "|" << to_string(contradiction.assigned_cause) << "|" << contradiction.public_resolution_status << "\n";
    }
    for (const HiddenTruthMutationRecord& record : state.hidden_truth_mutations) {
        out << "M|" << record.id << "|" << record.source_cluster_id << "|" << to_string(record.source_decision)
            << "|" << record.target_topic << "|" << record.cluster_scope << "|" << record.start_year << "|"
            << record.end_year << "|" << record.seed << "|" << record.authorized_access_level << "|"
            << record.algorithm_version << "|" << record.validation_summary;
        for (const std::string& id : record.inserted_entity_ids) {
            out << "|E:" << id;
        }
        for (const std::string& id : record.inserted_event_ids) {
            out << "|V:" << id;
        }
        out << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string serialize_world_content_for_seed_test(const ArchiveEngineState& state) {
    std::ostringstream out;
    out << "seeded_calendar_dispute=" << (state.include_seeded_calendar_dispute ? "true" : "false") << "\n";
    if (state.civilization_source.has_value()) {
        const CivilizationRuntimeSource& source = *state.civilization_source;
        out << "S|" << source.civilization_id << "|" << source.display_name << "|" << source.seed
            << "|" << source.earliest_year << "|" << source.latest_year << "|"
            << source.catalog_id << "|" << source.schema_version << "\n";
    }
    for (const auto& [id, entity] : state.hidden_truth.entities()) {
        out << "E|" << id << "|" << to_string(entity.type) << "|" << entity.canonical_name << "|" << interval_text(entity.existence_interval) << "\n";
    }
    for (const auto& [id, event] : state.hidden_truth.events()) {
        out << "V|" << id << "|" << event.start_year << "|" << event.end_year << "|" << event.title << "\n";
    }
    for (const auto& [id, artifact] : state.public_archive.artifacts()) {
        out << "A|" << id << "|" << to_string(artifact.type) << "|" << to_string(artifact.voice_register) << "|" << artifact.true_creation_year << "|"
            << artifact.claimed_creation_year << "|" << artifact.discovery_year
            << "|" << evidence_modifier_list_text(artifact)
            << "|" << std::fixed << std::setprecision(6) << artifact.reliability_score << "|" << artifact.public_text << "\n";
    }
    for (const Discovery& discovery : state.discovery_log) {
        out << "D|" << discovery.id << "|" << discovery.artifact_id << "|" << discovery.discovery_year
            << "|" << discovery.site_id << "|" << to_string(discovery.min_access) << "\n";
    }
    for (const auto& [id, contradiction] : state.public_archive.contradictions()) {
        out << "C|" << id << "|" << contradiction.detector_rule << "|" << to_string(contradiction.assigned_cause) << "|" << contradiction.public_resolution_status << "\n";
    }
    for (const HiddenTruthMutationRecord& record : state.hidden_truth_mutations) {
        out << "M|" << record.id << "|" << record.source_cluster_id << "|" << to_string(record.source_decision)
            << "|" << record.target_topic << "|" << record.cluster_scope << "|" << record.start_year << "|"
            << record.end_year << "|" << record.seed << "|" << record.authorized_access_level << "|"
            << record.algorithm_version << "|" << record.validation_summary;
        for (const std::string& id : record.inserted_entity_ids) {
            out << "|E:" << id;
        }
        for (const std::string& id : record.inserted_event_ids) {
            out << "|V:" << id;
        }
        out << "\n";
    }
    return out.str();
}

[[nodiscard]] bool contains_substr(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

[[nodiscard]] std::uint64_t parse_u64(const std::string& text);

} // namespace archive
