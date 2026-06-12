/*
 * Structural validation, anachronism detection, and contradiction integrity checks.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

[[nodiscard]] bool has_prefix(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] bool type_is_allowed(EntityType actual, std::initializer_list<EntityType> allowed) {
    return std::find(allowed.begin(), allowed.end(), actual) != allowed.end();
}

[[nodiscard]] std::vector<AnachronismReport> detect_anachronisms(const ArchiveEngineState& state) {
    std::vector<AnachronismReport> reports;

    auto add_temporal_metadata_report_if_needed = [&](const std::string& artifact_id,
                                                       const Artifact& artifact,
                                                       std::string_view field_name,
                                                       const std::string& entity_id,
                                                       std::string_view year_kind,
                                                       int year) {
        (void)artifact;
        const Entity* entity = state.hidden_truth.find_entity(entity_id);
        if (entity == nullptr) {
            // Cross-reference validation reports missing metadata IDs. Avoid
            // duplicating that error as a temporal anachronism.
            return;
        }
        if (!entity->existence_interval.contains(year)) {
            reports.push_back(AnachronismReport{
                artifact_id,
                std::string(field_name) + ": " + entity->canonical_name,
                std::string(year_kind),
                year,
                entity->existence_interval.start_year,
                entity->existence_interval.end_year,
                "high",
                "none",
                AnachronismStatus::InvalidGenerationBug,
            });
        }
    };

    auto add_claimed_surface_metadata_report_if_needed = [&](const std::string& artifact_id,
                                                             const Artifact& artifact,
                                                             std::string_view field_name,
                                                             const std::string& entity_id) {
        const Entity* entity = state.hidden_truth.find_entity(entity_id);
        if (entity == nullptr) {
            return;
        }
        if (entity->existence_interval.contains(artifact.claimed_creation_year)) {
            return;
        }

        AnachronismStatus status = AnachronismStatus::InvalidGenerationBug;
        std::string explanation = "none";
        std::string severity = "high";
        if (has_evidence_modifier(artifact, EvidenceModifier::Forgery)) {
            status = AnachronismStatus::ValidBecauseForged;
            explanation = "Forgery";
        } else if (has_any_evidence_modifier(artifact, {EvidenceModifier::LaterCopy, EvidenceModifier::Interpolation, EvidenceModifier::MisdatedStratum})) {
            status = AnachronismStatus::ValidBecauseLaterCopy;
            explanation = "LaterCopy/Interpolation/MisdatedStratum";
            severity = "medium";
        } else if (has_evidence_modifier(artifact, EvidenceModifier::RitualAnachronism)) {
            status = AnachronismStatus::ValidBecauseRitual;
            explanation = "RitualAnachronism";
            severity = "medium";
        }

        reports.push_back(AnachronismReport{
            artifact_id,
            std::string(field_name) + " claimed-surface metadata: " + entity->canonical_name,
            "claimed_creation_year",
            artifact.claimed_creation_year,
            entity->existence_interval.start_year,
            entity->existence_interval.end_year,
            severity,
            explanation,
            status,
        });
    };

    for (const auto& [artifact_id, artifact] : state.public_archive.artifacts()) {
        add_temporal_metadata_report_if_needed(artifact_id, artifact, "location_created", artifact.location_created, "true_creation_year", artifact.true_creation_year);
        add_temporal_metadata_report_if_needed(artifact_id, artifact, "location_found", artifact.location_found, "discovery_year", artifact.discovery_year);
        add_temporal_metadata_report_if_needed(artifact_id, artifact, "language_id", artifact.language_id, "true_creation_year", artifact.true_creation_year);
        add_temporal_metadata_report_if_needed(artifact_id, artifact, "dialect_id", artifact.dialect_id, "true_creation_year", artifact.true_creation_year);

        // In this MVP, oral-history records use script_id for the first modern
        // catalog transcription, not an original writing system. Until the
        // schema is split into original_script_id/transcription_script_id, check
        // oral-history scripts against discovery year.
        const bool script_is_transcription_layer = artifact.type == ArtifactType::OralHistory;
        add_temporal_metadata_report_if_needed(
            artifact_id,
            artifact,
            "script_id",
            artifact.script_id,
            script_is_transcription_layer ? "discovery_year" : "true_creation_year",
            script_is_transcription_layer ? artifact.discovery_year : artifact.true_creation_year
        );

        // Claimed-surface language metadata is checked against the claimed date.
        // This catches forgeries and later copies that use a valid true-date
        // dialect/script while claiming an earlier origin.
        add_claimed_surface_metadata_report_if_needed(artifact_id, artifact, "language_id", artifact.language_id);
        add_claimed_surface_metadata_report_if_needed(artifact_id, artifact, "dialect_id", artifact.dialect_id);
        if (!script_is_transcription_layer) {
            add_claimed_surface_metadata_report_if_needed(artifact_id, artifact, "script_id", artifact.script_id);
        }

        for (const std::string& entity_id : artifact.referenced_entity_ids) {
            const Entity* entity = state.hidden_truth.find_entity(entity_id);
            if (entity == nullptr) {
                reports.push_back(AnachronismReport{
                    artifact_id,
                    entity_id,
                    "claimed_creation_year",
                    artifact.claimed_creation_year,
                    0,
                    0,
                    "high",
                    "missing referenced entity",
                    AnachronismStatus::InvalidGenerationBug,
                });
                continue;
            }

            if (!entity->existence_interval.contains(artifact.claimed_creation_year)) {
                if (has_evidence_modifier(artifact, EvidenceModifier::Forgery)) {
                    reports.push_back(AnachronismReport{
                        artifact_id,
                        entity->canonical_name,
                        "claimed_creation_year",
                        artifact.claimed_creation_year,
                        entity->existence_interval.start_year,
                        entity->existence_interval.end_year,
                        "high",
                        "Forgery",
                        AnachronismStatus::ValidBecauseForged,
                    });
                } else if (has_evidence_modifier(artifact, EvidenceModifier::LaterCopy)) {
                    reports.push_back(AnachronismReport{
                        artifact_id,
                        entity->canonical_name,
                        "claimed_creation_year",
                        artifact.claimed_creation_year,
                        entity->existence_interval.start_year,
                        entity->existence_interval.end_year,
                        "medium",
                        "LaterCopy",
                        AnachronismStatus::ValidBecauseLaterCopy,
                    });
                } else {
                    reports.push_back(AnachronismReport{
                        artifact_id,
                        entity->canonical_name,
                        "claimed_creation_year",
                        artifact.claimed_creation_year,
                        entity->existence_interval.start_year,
                        entity->existence_interval.end_year,
                        "high",
                        "none",
                        AnachronismStatus::InvalidGenerationBug,
                    });
                }
            }
        }

        const Entity* attributed = state.hidden_truth.find_entity(artifact.attributed_creator_id);
        if (attributed != nullptr && !attributed->existence_interval.contains(artifact.true_creation_year)) {
            if (has_evidence_modifier(artifact, EvidenceModifier::Forgery)) {
                reports.push_back(AnachronismReport{
                    artifact_id,
                    attributed->canonical_name + " as attributed creator",
                    "true_creation_year",
                    artifact.true_creation_year,
                    attributed->existence_interval.start_year,
                    attributed->existence_interval.end_year,
                    "high",
                    "Forgery",
                    AnachronismStatus::ValidBecauseForged,
                });
            } else if (has_evidence_modifier(artifact, EvidenceModifier::RitualAnachronism)) {
                reports.push_back(AnachronismReport{
                    artifact_id,
                    attributed->canonical_name + " as attributed creator",
                    "true_creation_year",
                    artifact.true_creation_year,
                    attributed->existence_interval.start_year,
                    attributed->existence_interval.end_year,
                    "medium",
                    "RitualAnachronism",
                    AnachronismStatus::ValidBecauseRitual,
                });
            } else {
                reports.push_back(AnachronismReport{
                    artifact_id,
                    attributed->canonical_name + " as attributed creator",
                    "true_creation_year",
                    artifact.true_creation_year,
                    attributed->existence_interval.start_year,
                    attributed->existence_interval.end_year,
                    "high",
                    "none",
                    AnachronismStatus::InvalidGenerationBug,
                });
            }
        }
    }

    return reports;
}

[[nodiscard]] std::vector<std::string> validate_cross_references(const ArchiveEngineState& state) {
    std::vector<std::string> errors;

    auto validate_entity_field = [&](const std::string& artifact_id,
                                     std::string_view field_name,
                                     const std::string& entity_id,
                                     std::initializer_list<EntityType> allowed_types) {
        const Entity* entity = state.hidden_truth.find_entity(entity_id);
        if (entity == nullptr) {
            errors.push_back("artifact " + artifact_id + " field " + std::string(field_name) + " references missing entity " + entity_id);
            return;
        }
        if (!type_is_allowed(entity->type, allowed_types)) {
            errors.push_back("artifact " + artifact_id + " field " + std::string(field_name) + " expected compatible entity type but got " + to_string(entity->type) + " for " + entity_id);
        }
    };

    auto validate_person_id_if_structured = [&](const std::string& artifact_id,
                                                std::string_view field_name,
                                                const std::string& value) {
        if (!has_prefix(value, "person.")) {
            return;
        }
        const Entity* entity = state.hidden_truth.find_entity(value);
        if (entity == nullptr) {
            errors.push_back("artifact " + artifact_id + " field " + std::string(field_name) + " references missing person entity " + value);
            return;
        }
        if (entity->type != EntityType::Person) {
            errors.push_back("artifact " + artifact_id + " field " + std::string(field_name) + " expected person entity but got " + to_string(entity->type) + " for " + value);
        }
    };

    auto claim_entity_is_mediated_at_year = [](const Artifact& source) {
        return has_any_evidence_modifier(source, {
            EvidenceModifier::Forgery,
            EvidenceModifier::LaterCopy,
            EvidenceModifier::Interpolation,
            EvidenceModifier::RitualAnachronism,
            EvidenceModifier::MythicCompression,
            EvidenceModifier::MisdatedStratum,
        });
    };

    auto validate_claim_semantic_entity = [&](const std::string& claim_id,
                                              std::string_view field_name,
                                              const std::optional<std::string>& maybe_entity_id,
                                              const Artifact& source,
                                              const std::optional<int>& maybe_claimed_year) {
        if (!maybe_entity_id.has_value()) {
            return;
        }
        const Entity* entity = state.hidden_truth.find_entity(*maybe_entity_id);
        if (entity == nullptr) {
            errors.push_back("claim " + claim_id + " semantics field " + std::string(field_name) + " references missing entity " + *maybe_entity_id);
            return;
        }
        if (maybe_claimed_year.has_value() && !entity->existence_interval.contains(*maybe_claimed_year) && !claim_entity_is_mediated_at_year(source)) {
            errors.push_back("claim " + claim_id + " semantics field " + std::string(field_name) + " references entity " + *maybe_entity_id + " outside valid range at claimed_year=" + std::to_string(*maybe_claimed_year));
        }
    };

    for (const auto& [artifact_id, artifact] : state.public_archive.artifacts()) {
        for (const std::string& event_id : artifact.hidden_event_links) {
            if (state.hidden_truth.find_event(event_id) == nullptr) {
                errors.push_back("artifact " + artifact_id + " references missing hidden event " + event_id);
            }
        }

        validate_entity_field(artifact_id, "location_created", artifact.location_created, {EntityType::Site, EntityType::Settlement});
        validate_entity_field(artifact_id, "location_found", artifact.location_found, {EntityType::Site, EntityType::Settlement});
        validate_entity_field(artifact_id, "language_id", artifact.language_id, {EntityType::Language});
        validate_entity_field(artifact_id, "dialect_id", artifact.dialect_id, {EntityType::Dialect});
        validate_entity_field(artifact_id, "script_id", artifact.script_id, {EntityType::Script});
        validate_person_id_if_structured(artifact_id, "creator_id", artifact.creator_id);
        validate_person_id_if_structured(artifact_id, "attributed_creator_id", artifact.attributed_creator_id);

        for (const std::string& entity_id : artifact.referenced_entity_ids) {
            if (state.hidden_truth.find_entity(entity_id) == nullptr) {
                errors.push_back("artifact " + artifact_id + " references missing entity " + entity_id);
            }
        }

        for (const std::string& contradiction_id : artifact.contradiction_ids) {
            if (state.public_archive.find_contradiction(contradiction_id) == nullptr) {
                errors.push_back("artifact " + artifact_id + " references missing contradiction " + contradiction_id);
            }
        }
    }

    for (const auto& [claim_id, claim] : state.public_archive.claims()) {
        const Artifact* source = state.public_archive.find_artifact(claim.source_artifact_id);
        if (source == nullptr) {
            errors.push_back("claim " + claim_id + " references missing source artifact " + claim.source_artifact_id);
            continue;
        }
        if (std::find(source->claim_ids.begin(), source->claim_ids.end(), claim_id) == source->claim_ids.end()) {
            errors.push_back("claim " + claim_id + " is not listed by source artifact " + claim.source_artifact_id);
        }

        if (claim.semantics.has_value()) {
            const ClaimSemantics& semantics = *claim.semantics;
            if (semantics.claimed_year.has_value() && *semantics.claimed_year > source->true_creation_year &&
                !has_evidence_modifier(*source, EvidenceModifier::RitualAnachronism)) {
                errors.push_back("claim " + claim_id + " semantics claimed_year is after source artifact true_creation_year");
            }

            validate_claim_semantic_entity(claim_id, "subject_entity_id", semantics.subject_entity_id, *source, semantics.claimed_year);
            validate_claim_semantic_entity(claim_id, "object_entity_id", semantics.object_entity_id, *source, semantics.claimed_year);

            if (semantics.object_entity_id.has_value()) {
                const Entity* object = state.hidden_truth.find_entity(*semantics.object_entity_id);
                if (object != nullptr) {
                    if (semantics.predicate_type == PredicateType::CreatedOffice && object->type != EntityType::Office) {
                        errors.push_back("claim " + claim_id + " semantics CreatedOffice expects object_entity_id to be an office");
                    }
                    if (semantics.predicate_type == PredicateType::UsesScript && object->type != EntityType::Script) {
                        errors.push_back("claim " + claim_id + " semantics UsesScript expects object_entity_id to be a script");
                    }
                    if (semantics.predicate_type == PredicateType::LocatedAt &&
                        object->type != EntityType::Site && object->type != EntityType::Settlement) {
                        errors.push_back("claim " + claim_id + " semantics LocatedAt expects object_entity_id to be a site or settlement");
                    }
                }
            }
        }
    }

    for (const auto& [contradiction_id, contradiction] : state.public_archive.contradictions()) {
        for (const std::string& artifact_id : contradiction.involved_artifact_ids) {
            const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
            if (artifact == nullptr) {
                errors.push_back("contradiction " + contradiction_id + " references missing artifact " + artifact_id);
                continue;
            }
            if (std::find(artifact->contradiction_ids.begin(), artifact->contradiction_ids.end(), contradiction_id) == artifact->contradiction_ids.end()) {
                errors.push_back("contradiction " + contradiction_id + " is not listed by involved artifact " + artifact_id);
            }
        }
    }

    return errors;
}

[[nodiscard]] std::vector<std::string> validate_mysteries(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> ids;

    auto mystery_exists = [&](const std::string& mystery_id) {
        return std::any_of(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
            return mystery.id == mystery_id;
        });
    };

    for (const Mystery& mystery : state.mysteries) {
        if (mystery.id.empty()) {
            errors.push_back("mystery has empty id");
            continue;
        }
        if (!ids.insert(mystery.id).second) {
            errors.push_back("duplicate mystery id: " + mystery.id);
        }
        if (mystery.title.empty()) {
            errors.push_back("mystery " + mystery.id + " missing title");
        }
        if (mystery.protected_question.empty()) {
            errors.push_back("mystery " + mystery.id + " missing protected question");
        }
        if (mystery.max_public_confidence < 0.0 || mystery.max_public_confidence > 1.0 ||
            mystery.max_scholar_confidence < 0.0 || mystery.max_scholar_confidence > 1.0) {
            errors.push_back("mystery " + mystery.id + " has invalid confidence cap");
        }
        if (mystery.max_scholar_confidence < mystery.max_public_confidence) {
            errors.push_back("mystery " + mystery.id + " has scholar cap below public cap");
        }
        if (mystery.clue_artifact_ids.empty() && mystery.evidence_links.empty()) {
            errors.push_back("mystery " + mystery.id + " has no clue artifacts or evidence links");
        }
        if (mystery.evidence_links.empty()) {
            errors.push_back("mystery " + mystery.id + " has no explicit evidence links");
        }

        for (const std::string& artifact_id : mystery.clue_artifact_ids) {
            const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
            if (artifact == nullptr) {
                errors.push_back("mystery " + mystery.id + " references missing clue artifact " + artifact_id);
                continue;
            }
            if (std::find(artifact->mystery_links.begin(), artifact->mystery_links.end(), mystery.id) == artifact->mystery_links.end()) {
                errors.push_back("mystery " + mystery.id + " is not listed by clue artifact " + artifact_id);
            }
        }
        for (const std::string& artifact_id : mystery.misleading_artifact_ids) {
            const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
            if (artifact == nullptr) {
                errors.push_back("mystery " + mystery.id + " references missing misleading artifact " + artifact_id);
                continue;
            }
            if (std::find(artifact->mystery_links.begin(), artifact->mystery_links.end(), mystery.id) == artifact->mystery_links.end()) {
                errors.push_back("mystery " + mystery.id + " is not listed by misleading artifact " + artifact_id);
            }
        }

        for (const MysteryEvidenceLink& link : mystery.evidence_links) {
            const Artifact* artifact = state.public_archive.find_artifact(link.artifact_id);
            if (artifact == nullptr) {
                errors.push_back("mystery " + mystery.id + " evidence link references missing artifact " + link.artifact_id);
                continue;
            }
            if (std::find(artifact->mystery_links.begin(), artifact->mystery_links.end(), mystery.id) == artifact->mystery_links.end()) {
                errors.push_back("mystery " + mystery.id + " evidence link artifact " + link.artifact_id + " does not list the mystery");
            }
            if (link.claim_id.has_value()) {
                const Claim* claim = state.public_archive.find_claim(*link.claim_id);
                if (claim == nullptr) {
                    errors.push_back("mystery " + mystery.id + " evidence link references missing claim " + *link.claim_id);
                } else if (claim->source_artifact_id != link.artifact_id) {
                    errors.push_back("mystery " + mystery.id + " evidence link claim " + *link.claim_id + " does not belong to artifact " + link.artifact_id);
                }
            }
            if (link.weight_multiplier < 0.0) {
                errors.push_back("mystery " + mystery.id + " evidence link has negative weight multiplier for artifact " + link.artifact_id);
            }
        }
    }

    for (const auto& [artifact_id, artifact] : state.public_archive.artifacts()) {
        for (const std::string& mystery_id : artifact.mystery_links) {
            if (!mystery_exists(mystery_id)) {
                errors.push_back("artifact " + artifact_id + " references missing mystery " + mystery_id);
            }
        }
    }

    return errors;
}

[[nodiscard]] std::vector<std::string> validate_hidden_truth_mutations(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> record_ids;

    for (const HiddenTruthMutationRecord& record : state.hidden_truth_mutations) {
        if (record.id.empty()) {
            errors.push_back("hidden truth mutation record has empty id");
        } else if (!record_ids.insert(record.id).second) {
            errors.push_back("duplicate hidden truth mutation record id: " + record.id);
        }
        if (record.source_cluster_id.empty()) {
            errors.push_back("hidden truth mutation record " + record.id + " has empty source_cluster_id");
        }
        if (record.source_decision == HiddenClusterDecision::Reject) {
            errors.push_back("hidden truth mutation record " + record.id + " has rejected source decision");
        }
        if (record.inserted_entity_ids.empty() && record.inserted_event_ids.empty()) {
            errors.push_back("hidden truth mutation record " + record.id + " lists no inserted entities or events");
        }
        if (record.target_topic.empty()) {
            errors.push_back("hidden truth mutation record " + record.id + " has empty target_topic");
        }
        if (record.cluster_scope.empty()) {
            errors.push_back("hidden truth mutation record " + record.id + " has empty cluster_scope");
        }
        if (record.start_year <= 0 || record.end_year <= 0 || record.start_year > record.end_year) {
            errors.push_back("hidden truth mutation record " + record.id + " has invalid year window");
        }
        if (record.authorized_access_level != "curator" &&
            record.authorized_access_level != "canon" &&
            record.authorized_access_level != "debug") {
            errors.push_back("hidden truth mutation record " + record.id + " has non-authorized access level " + record.authorized_access_level);
        }
        if (record.algorithm_version.empty()) {
            errors.push_back("hidden truth mutation record " + record.id + " has empty algorithm_version");
        }
        if (record.validation_summary.empty()) {
            errors.push_back("hidden truth mutation record " + record.id + " has empty validation_summary");
        }
        for (const std::string& entity_id : record.inserted_entity_ids) {
            if (state.hidden_truth.find_entity(entity_id) == nullptr) {
                errors.push_back("hidden truth mutation record " + record.id + " references missing inserted entity " + entity_id);
            }
        }
        for (const std::string& event_id : record.inserted_event_ids) {
            if (state.hidden_truth.find_event(event_id) == nullptr) {
                errors.push_back("hidden truth mutation record " + record.id + " references missing inserted event " + event_id);
            }
        }
    }

    return errors;
}

[[nodiscard]] std::vector<std::string> validate_hidden_mutation_artifact_source(const ArchiveEngineState& state,
                                                                                const HiddenTruthMutationRecord& record) {
    std::vector<std::string> errors;
    const std::string record_label = record.id.empty() ? std::string("<empty mutation record id>") : record.id;

    if (record.id.empty()) {
        errors.push_back("hidden mutation artifact source has empty mutation record id");
    }
    if (record.source_cluster_id.empty()) {
        errors.push_back("hidden mutation artifact source " + record_label + " has missing source cluster id");
    }
    if (record.source_decision == HiddenClusterDecision::Reject) {
        errors.push_back("hidden mutation artifact source " + record_label + " has rejected source decision");
    }
    if (record.inserted_entity_ids.empty()) {
        errors.push_back("hidden mutation artifact source " + record_label + " has missing inserted entities");
    }
    if (record.inserted_event_ids.empty()) {
        errors.push_back("hidden mutation artifact source " + record_label + " has missing inserted events");
    }
    // v25.1 keeps the existing string summary field. A later structured audit
    // status can replace this substring check without changing candidate output.
    if (record.validation_summary.find("passed") == std::string::npos) {
        errors.push_back("hidden mutation artifact source " + record_label + " validation summary is not passed");
    }

    for (const std::string& entity_id : record.inserted_entity_ids) {
        if (state.hidden_truth.find_entity(entity_id) == nullptr) {
            errors.push_back("hidden mutation artifact source " + record_label + " references missing hidden entity " + entity_id);
        }
    }

    for (const std::string& event_id : record.inserted_event_ids) {
        const Event* event = state.hidden_truth.find_event(event_id);
        if (event == nullptr) {
            errors.push_back("hidden mutation artifact source " + record_label + " references missing hidden event " + event_id);
            continue;
        }
        if (event->end_year < event->start_year) {
            errors.push_back("hidden mutation artifact source " + record_label + " references event " + event_id + " with invalid event chronology");
        }
        for (const std::string& cause_id : event->cause_event_ids) {
            const Event* cause = state.hidden_truth.find_event(cause_id);
            if (cause == nullptr) {
                errors.push_back("hidden mutation artifact source " + record_label + " event " + event_id + " references missing cause event " + cause_id);
                continue;
            }
            if (cause->end_year > event->start_year) {
                errors.push_back("hidden mutation artifact source " + record_label + " cause event " + cause_id + " ends after effect event " + event_id + " starts");
            }
        }
    }

    return errors;
}


[[nodiscard]] std::vector<std::string> validate_hidden_mutation_artifact_source(const ArchiveEngineState& state,
                                                                                const HiddenMutationArtifactSource& source) {
    std::vector<std::string> errors;
    const std::string source_label = source.mutation_record_id.empty() ? std::string("<empty mutation record id>") : source.mutation_record_id;

    if (source.mutation_record_id.empty()) {
        errors.push_back("hidden mutation artifact source has empty mutation_record_id");
    }
    if (source.source_cluster_id.empty()) {
        errors.push_back("hidden mutation artifact source " + source_label + " has missing source_cluster_id");
    }
    if (source.source_entity_ids.empty()) {
        errors.push_back("hidden mutation artifact source " + source_label + " has missing source_entity_ids");
    }
    if (source.source_event_ids.empty()) {
        errors.push_back("hidden mutation artifact source " + source_label + " has missing source_event_ids");
    }

    const auto record_it = std::find_if(state.hidden_truth_mutations.begin(), state.hidden_truth_mutations.end(), [&](const HiddenTruthMutationRecord& record) {
        return record.id == source.mutation_record_id;
    });
    if (record_it == state.hidden_truth_mutations.end()) {
        errors.push_back("hidden mutation artifact source " + source_label + " references missing mutation record");
    } else {
        const std::vector<std::string> record_errors = validate_hidden_mutation_artifact_source(state, *record_it);
        errors.insert(errors.end(), record_errors.begin(), record_errors.end());
        if (source.source_cluster_id != record_it->source_cluster_id) {
            errors.push_back("hidden mutation artifact source " + source_label + " source_cluster_id does not match mutation record");
        }
        for (const std::string& entity_id : source.source_entity_ids) {
            if (std::find(record_it->inserted_entity_ids.begin(), record_it->inserted_entity_ids.end(), entity_id) == record_it->inserted_entity_ids.end()) {
                errors.push_back("hidden mutation artifact source " + source_label + " entity " + entity_id + " is not listed by mutation record");
            }
        }
        for (const std::string& event_id : source.source_event_ids) {
            if (std::find(record_it->inserted_event_ids.begin(), record_it->inserted_event_ids.end(), event_id) == record_it->inserted_event_ids.end()) {
                errors.push_back("hidden mutation artifact source " + source_label + " event " + event_id + " is not listed by mutation record");
            }
        }
    }

    for (const std::string& entity_id : source.source_entity_ids) {
        if (state.hidden_truth.find_entity(entity_id) == nullptr) {
            errors.push_back("hidden mutation artifact source " + source_label + " references missing hidden entity " + entity_id);
        }
    }

    for (const std::string& event_id : source.source_event_ids) {
        const Event* event = state.hidden_truth.find_event(event_id);
        if (event == nullptr) {
            errors.push_back("hidden mutation artifact source " + source_label + " references missing hidden event " + event_id);
            continue;
        }
        if (event->end_year < event->start_year) {
            errors.push_back("hidden mutation artifact source " + source_label + " references event " + event_id + " with invalid chronology");
        }
        for (const std::string& cause_id : event->cause_event_ids) {
            const Event* cause = state.hidden_truth.find_event(cause_id);
            if (cause == nullptr) {
                errors.push_back("hidden mutation artifact source " + source_label + " event " + event_id + " references missing cause event " + cause_id);
                continue;
            }
            if (cause->end_year > event->start_year) {
                errors.push_back("hidden mutation artifact source " + source_label + " cause event " + cause_id + " ends after effect event " + event_id + " starts");
            }
        }
    }

    return errors;
}

[[nodiscard]] std::vector<std::string> validate_materialized_hidden_mutation_artifact_provenance(const ArchiveEngineState& state,
                                                                                                  const Artifact& artifact) {
    std::vector<std::string> errors;
    if (!artifact.hidden_mutation_artifact_provenance.has_value()) {
        return errors;
    }

    const MaterializedHiddenMutationArtifactProvenance& provenance = *artifact.hidden_mutation_artifact_provenance;
    const std::string artifact_label = artifact.id.empty() ? std::string("<empty artifact id>") : artifact.id;

    if (provenance.candidate_id.empty()) {
        errors.push_back("materialized hidden-mutation artifact " + artifact_label + " has empty candidate_id");
    }
    if (provenance.mutation_record_id.empty()) {
        errors.push_back("materialized hidden-mutation artifact " + artifact_label + " has empty mutation_record_id");
    }
    if (provenance.source_cluster_id.empty()) {
        errors.push_back("materialized hidden-mutation artifact " + artifact_label + " has empty source_cluster_id");
    }
    if (provenance.source_entity_ids.empty()) {
        errors.push_back("materialized hidden-mutation artifact " + artifact_label + " has empty source_entity_ids");
    }
    if (provenance.source_event_ids.empty()) {
        errors.push_back("materialized hidden-mutation artifact " + artifact_label + " has empty source_event_ids");
    }
    if (artifact.claim_ids.empty()) {
        errors.push_back("materialized hidden-mutation artifact " + artifact_label + " has no public-safe claims");
    }

    const auto record_it = std::find_if(state.hidden_truth_mutations.begin(), state.hidden_truth_mutations.end(), [&](const HiddenTruthMutationRecord& record) {
        return record.id == provenance.mutation_record_id;
    });
    if (record_it == state.hidden_truth_mutations.end()) {
        errors.push_back("materialized hidden-mutation artifact " + artifact_label + " references missing mutation record " + provenance.mutation_record_id);
    } else {
        if (provenance.source_cluster_id != record_it->source_cluster_id) {
            errors.push_back("materialized hidden-mutation artifact " + artifact_label + " source_cluster_id does not match mutation record");
        }
        for (const std::string& entity_id : provenance.source_entity_ids) {
            if (std::find(record_it->inserted_entity_ids.begin(), record_it->inserted_entity_ids.end(), entity_id) == record_it->inserted_entity_ids.end()) {
                errors.push_back("materialized hidden-mutation artifact " + artifact_label + " source entity " + entity_id + " is not listed by mutation record");
            }
        }
        for (const std::string& event_id : provenance.source_event_ids) {
            if (std::find(record_it->inserted_event_ids.begin(), record_it->inserted_event_ids.end(), event_id) == record_it->inserted_event_ids.end()) {
                errors.push_back("materialized hidden-mutation artifact " + artifact_label + " source event " + event_id + " is not listed by mutation record");
            }
        }
    }

    for (const std::string& entity_id : provenance.source_entity_ids) {
        if (state.hidden_truth.find_entity(entity_id) == nullptr) {
            errors.push_back("materialized hidden-mutation artifact " + artifact_label + " references missing hidden entity " + entity_id);
        }
    }
    for (const std::string& event_id : provenance.source_event_ids) {
        if (state.hidden_truth.find_event(event_id) == nullptr) {
            errors.push_back("materialized hidden-mutation artifact " + artifact_label + " references missing hidden event " + event_id);
        }
    }

    return errors;
}

[[nodiscard]] std::vector<std::string> validate_discovery_log(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> discovery_ids;
    std::map<std::string, int> artifact_discovery_counts;

    for (const Discovery& discovery : state.discovery_log) {
        if (discovery.id.empty()) {
            errors.push_back("discovery record has empty id for artifact " + discovery.artifact_id);
        } else if (!discovery_ids.insert(discovery.id).second) {
            errors.push_back("duplicate discovery id: " + discovery.id);
        }

        if (discovery.artifact_id.empty()) {
            errors.push_back("discovery " + discovery.id + " has empty artifact_id");
            continue;
        }
        ++artifact_discovery_counts[discovery.artifact_id];

        const Artifact* artifact = state.public_archive.find_artifact(discovery.artifact_id);
        if (artifact == nullptr) {
            errors.push_back("discovery " + discovery.id + " references missing artifact " + discovery.artifact_id);
            continue;
        }
        if (discovery.discovery_year != artifact->discovery_year) {
            errors.push_back("discovery " + discovery.id + " year " + std::to_string(discovery.discovery_year) +
                             " does not match artifact " + discovery.artifact_id + " discovery_year " +
                             std::to_string(artifact->discovery_year));
        }
        if (discovery.site_id != artifact->location_found) {
            errors.push_back("discovery " + discovery.id + " site " + discovery.site_id +
                             " does not match artifact " + discovery.artifact_id + " location_found " + artifact->location_found);
        }
        if (!can_view(discovery.min_access, artifact->min_access)) {
            errors.push_back("discovery " + discovery.id + " access " + to_string(discovery.min_access) +
                             " is less restrictive than artifact " + discovery.artifact_id + " access " + to_string(artifact->min_access));
        }
    }

    for (const auto& [artifact_id, artifact] : state.public_archive.artifacts()) {
        (void)artifact;
        const int count = artifact_discovery_counts[artifact_id];
        if (count == 0) {
            errors.push_back("artifact " + artifact_id + " has no matching discovery record");
        } else if (count > 1) {
            errors.push_back("artifact " + artifact_id + " has multiple discovery records");
        }
    }

    return errors;
}

[[nodiscard]] std::vector<std::string> validate_full_state(const ArchiveEngineState& state) {
    std::vector<std::string> errors = state.hidden_truth.validate();
    if (state.hidden_truth.entities().empty() && state.hidden_truth.events().empty() &&
        state.public_archive.artifacts().empty() && state.public_archive.claims().empty()) {
        errors.push_back("archive engine state is empty and has not been initialized");
    }
    std::vector<std::string> archive_errors = state.public_archive.validate_metadata();
    errors.insert(errors.end(), archive_errors.begin(), archive_errors.end());

    std::vector<std::string> cross_reference_errors = validate_cross_references(state);
    errors.insert(errors.end(), cross_reference_errors.begin(), cross_reference_errors.end());

    std::vector<std::string> mystery_errors = validate_mysteries(state);
    errors.insert(errors.end(), mystery_errors.begin(), mystery_errors.end());

    std::vector<std::string> discovery_errors = validate_discovery_log(state);
    errors.insert(errors.end(), discovery_errors.begin(), discovery_errors.end());

    std::vector<std::string> mutation_errors = validate_hidden_truth_mutations(state);
    errors.insert(errors.end(), mutation_errors.begin(), mutation_errors.end());

    std::vector<std::string> evidence_potential_errors = validate_evidence_potentials(state);
    errors.insert(errors.end(), evidence_potential_errors.begin(), evidence_potential_errors.end());

    std::vector<std::string> candidate_plan_errors = validate_candidate_artifact_plans(state);
    errors.insert(errors.end(), candidate_plan_errors.begin(), candidate_plan_errors.end());

    std::vector<std::string> candidate_evaluation_errors = validate_candidate_artifact_plan_evaluations(state);
    errors.insert(errors.end(), candidate_evaluation_errors.begin(), candidate_evaluation_errors.end());

    std::vector<std::string> candidate_proposal_errors = validate_candidate_artifact_proposals(state);
    errors.insert(errors.end(), candidate_proposal_errors.begin(), candidate_proposal_errors.end());

    std::vector<std::string> candidate_proposal_audit_errors = validate_candidate_artifact_proposal_audits(state);
    errors.insert(errors.end(), candidate_proposal_audit_errors.begin(), candidate_proposal_audit_errors.end());

    std::vector<std::string> candidate_draft_errors = validate_candidate_artifact_drafts(state);
    errors.insert(errors.end(), candidate_draft_errors.begin(), candidate_draft_errors.end());
    std::vector<std::string> control_layer_audit_errors = validate_control_layer_audit_report(build_control_layer_audit_report());
    errors.insert(errors.end(), control_layer_audit_errors.begin(), control_layer_audit_errors.end());

    for (const auto& [artifact_id, artifact] : state.public_archive.artifacts()) {
        (void)artifact_id;
        const std::vector<std::string> provenance_errors = validate_materialized_hidden_mutation_artifact_provenance(state, artifact);
        errors.insert(errors.end(), provenance_errors.begin(), provenance_errors.end());
    }

    const std::vector<AnachronismReport> reports = detect_anachronisms(state);
    for (const AnachronismReport& report : reports) {
        if (report.status == AnachronismStatus::InvalidGenerationBug) {
            errors.push_back(
                "invalid anachronism in " + report.artifact_id +
                ": " + report.referenced_item +
                " unavailable at " + report.checked_year_kind + "=" + std::to_string(report.checked_year)
            );
        }
    }

    return errors;
}

} // namespace archive
