/*
 * Procedural hidden timeline cluster generation.
 *
 * v23 is the first hidden-world generation slice, but it remains advisory:
 * clusters are generated, simulated on a copied HiddenTruthGraph, evaluated, and
 * formatted for curator/canon/debug review. The live ArchiveEngineState is never
 * mutated here.
 */
#include "impossible_archive.h"

namespace archive {

namespace {

[[nodiscard]] std::uint64_t cluster_hash(std::string_view text) {
    std::uint64_t value = 1469598103934665603ULL;
    for (const char ch : text) {
        value ^= static_cast<unsigned char>(ch);
        value *= 1099511628211ULL;
    }
    return value;
}

[[nodiscard]] std::string cluster_hex64(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
}

[[nodiscard]] std::string cluster_digest(const HiddenTimelineClusterRequest& request,
                                         std::size_t index,
                                         std::string_view kind) {
    std::ostringstream material;
    material << request.target_topic << "|" << request.start_year << "|" << request.end_year
             << "|" << request.seed << "|" << to_string(request.scope) << "|" << index << "|" << kind;
    std::uint64_t value = cluster_hash(material.str());
    constexpr std::uint64_t mix_a = static_cast<std::uint64_t>(0x9e3779b97f4a7c15ULL);
    constexpr std::uint64_t mix_b = static_cast<std::uint64_t>(0xbf58476d1ce4e5b9ULL);
    value ^= static_cast<std::uint64_t>(request.start_year) + mix_a + (value << 6U) + (value >> 2U);
    value ^= static_cast<std::uint64_t>(request.end_year) * mix_b;
    value ^= (static_cast<std::uint64_t>(index) + static_cast<std::uint64_t>(1U)) * mix_a;
    return cluster_hex64(value);
}

[[nodiscard]] int clamp_year_to_window(int preferred,
                                       const HiddenTimelineClusterRequest& request,
                                       int fallback_offset) {
    if (request.start_year <= 0 || request.end_year <= 0 || request.start_year > request.end_year) {
        return preferred;
    }
    const int fallback = request.start_year + fallback_offset;
    return std::max(request.start_year, std::min(request.end_year, preferred == 0 ? fallback : preferred));
}

void add_error(std::vector<std::string>& errors, std::string error) {
    errors.push_back(std::move(error));
}

[[nodiscard]] ProposedCausalLink make_proposed_causal_link(std::string cause_event_id,
                                                          std::string effect_event_id,
                                                          std::string explanation = {}) {
    ProposedCausalLink link;
    link.cause_event_id = std::move(cause_event_id);
    link.effect_event_id = std::move(effect_event_id);
    link.explanation = std::move(explanation);
    return link;
}

[[nodiscard]] std::string format_proposed_causal_link(const ProposedCausalLink& link) {
    std::ostringstream out;
    out << link.cause_event_id << " -> " << link.effect_event_id;
    if (!link.explanation.empty()) {
        out << " (" << link.explanation << ")";
    }
    return out.str();
}

[[nodiscard]] const Entity* find_proposed_entity(const GeneratedHiddenTimelineCluster& cluster,
                                                 const std::string& id) {
    const auto it = std::find_if(cluster.proposed_entities.begin(), cluster.proposed_entities.end(), [&](const Entity& entity) {
        return entity.id == id;
    });
    return it == cluster.proposed_entities.end() ? nullptr : &*it;
}

[[nodiscard]] const Event* find_proposed_event(const GeneratedHiddenTimelineCluster& cluster,
                                               const std::string& id) {
    const auto it = std::find_if(cluster.proposed_events.begin(), cluster.proposed_events.end(), [&](const Event& event) {
        return event.id == id;
    });
    return it == cluster.proposed_events.end() ? nullptr : &*it;
}

[[nodiscard]] const Entity* find_entity_in_state_or_cluster(const ArchiveEngineState& state,
                                                            const GeneratedHiddenTimelineCluster& cluster,
                                                            const std::string& id) {
    if (const Entity* entity = find_proposed_entity(cluster, id)) {
        return entity;
    }
    return state.hidden_truth.find_entity(id);
}

[[nodiscard]] const Event* find_event_in_state_or_cluster(const ArchiveEngineState& state,
                                                          const GeneratedHiddenTimelineCluster& cluster,
                                                          const std::string& id) {
    if (const Event* event = find_proposed_event(cluster, id)) {
        return event;
    }
    return state.hidden_truth.find_event(id);
}

void add_cluster_local_errors(const ArchiveEngineState& state,
                              const GeneratedHiddenTimelineCluster& cluster,
                              std::vector<std::string>& errors) {
    if (cluster.request.start_year <= 0 || cluster.request.end_year <= 0) {
        add_error(errors, "cluster request has unset year window");
    }
    if (cluster.request.start_year > cluster.request.end_year) {
        add_error(errors, "cluster request start_year is after end_year");
    }
    if (!cluster.resolved_target.has_value()) {
        add_error(errors, "cluster target topic did not resolve");
    }
    if (cluster.proposed_entities.empty() && cluster.proposed_events.empty()) {
        add_error(errors, "cluster contains no proposed entities or events");
    }

    std::set<std::string> seen_entity_ids;
    for (const Entity& entity : cluster.proposed_entities) {
        if (entity.id.empty()) {
            add_error(errors, "proposed entity has empty id");
            continue;
        }
        if (!seen_entity_ids.insert(entity.id).second) {
            add_error(errors, "duplicate proposed entity id " + entity.id);
        }
        if (state.hidden_truth.find_entity(entity.id) != nullptr) {
            add_error(errors, "proposed entity id collides with existing hidden entity " + entity.id);
        }
        if (entity.existence_interval.start_year <= 0 || entity.existence_interval.end_year < entity.existence_interval.start_year) {
            add_error(errors, "proposed entity " + entity.id + " has invalid existence interval");
        }
    }

    std::set<std::string> seen_event_ids;
    for (const Event& event : cluster.proposed_events) {
        if (event.id.empty()) {
            add_error(errors, "proposed event has empty id");
            continue;
        }
        if (!seen_event_ids.insert(event.id).second) {
            add_error(errors, "duplicate proposed event id " + event.id);
        }
        if (state.hidden_truth.find_event(event.id) != nullptr) {
            add_error(errors, "proposed event id collides with existing hidden event " + event.id);
        }
        if (event.end_year < event.start_year) {
            add_error(errors, "proposed event " + event.id + " ends before it starts");
        }
        if (cluster.request.start_year <= cluster.request.end_year &&
            (event.start_year < cluster.request.start_year || event.end_year > cluster.request.end_year)) {
            add_error(errors, "proposed event " + event.id + " falls outside cluster year window");
        }

        for (const std::string& cause_id : event.cause_event_ids) {
            const Event* cause = find_event_in_state_or_cluster(state, cluster, cause_id);
            if (cause == nullptr) {
                add_error(errors, "proposed event " + event.id + " references missing cause " + cause_id);
            } else if (cause->end_year > event.start_year) {
                add_error(errors, "proposed event " + event.id + " occurs before cause " + cause_id + " ends");
            }
        }

        for (const std::string& participant_id : event.participant_entity_ids) {
            const Entity* participant = find_entity_in_state_or_cluster(state, cluster, participant_id);
            if (participant == nullptr) {
                add_error(errors, "proposed event " + event.id + " references missing participant " + participant_id);
            } else if (!participant->existence_interval.contains(event.start_year) ||
                       !participant->existence_interval.contains(event.end_year)) {
                add_error(errors, "participant " + participant_id + " does not exist during proposed event " + event.id);
            }
        }

        for (const std::string& required_id : event.required_entity_ids) {
            const Entity* required = find_entity_in_state_or_cluster(state, cluster, required_id);
            if (required == nullptr) {
                add_error(errors, "proposed event " + event.id + " references missing required entity " + required_id);
            } else if (!required->existence_interval.contains(event.start_year) ||
                       !required->existence_interval.contains(event.end_year)) {
                add_error(errors, "required entity " + required_id + " unavailable during proposed event " + event.id);
            }
        }
    }


    for (const ProposedCausalLink& link : cluster.causal_links) {
        if (link.cause_event_id.empty()) {
            add_error(errors, "causal link has empty cause_event_id");
            continue;
        }
        if (link.effect_event_id.empty()) {
            add_error(errors, "causal link has empty effect_event_id");
            continue;
        }

        const Event* cause = find_event_in_state_or_cluster(state, cluster, link.cause_event_id);
        const Event* effect = find_event_in_state_or_cluster(state, cluster, link.effect_event_id);
        if (cause == nullptr) {
            add_error(errors, "causal link references missing cause " + link.cause_event_id);
            continue;
        }
        if (effect == nullptr) {
            add_error(errors, "causal link references missing effect " + link.effect_event_id);
            continue;
        }
        if (cause->end_year > effect->start_year) {
            add_error(errors, "causal link cause " + link.cause_event_id +
                              " ends after effect " + link.effect_event_id + " starts");
        }
        if (std::find(effect->cause_event_ids.begin(), effect->cause_event_ids.end(), link.cause_event_id) ==
            effect->cause_event_ids.end()) {
            add_error(errors, "causal link " + link.cause_event_id + " -> " + link.effect_event_id +
                              " is not mirrored by effect cause_event_ids");
        }
    }

    for (const Entity& entity : cluster.proposed_entities) {
        if (!entity.created_by_event_id.empty()) {
            const Event* created_by = find_event_in_state_or_cluster(state, cluster, entity.created_by_event_id);
            if (created_by == nullptr) {
                add_error(errors, "proposed entity " + entity.id + " references missing created_by_event_id " + entity.created_by_event_id);
            } else if (created_by->end_year > entity.existence_interval.start_year) {
                add_error(errors, "proposed entity " + entity.id + " is created after its existence interval begins");
            }
        }
        if (!entity.ended_by_event_id.empty()) {
            const Event* ended_by = find_event_in_state_or_cluster(state, cluster, entity.ended_by_event_id);
            if (ended_by == nullptr) {
                add_error(errors, "proposed entity " + entity.id + " references missing ended_by_event_id " + entity.ended_by_event_id);
            } else if (entity.existence_interval.end_year != kOpenEndedYear &&
                       ended_by->start_year < entity.existence_interval.end_year) {
                add_error(errors, "proposed entity " + entity.id + " is ended before its existence interval ends");
            }
        }
    }
}

void add_simulated_cluster_errors(const ArchiveEngineState& state,
                                  const GeneratedHiddenTimelineCluster& cluster,
                                  std::vector<std::string>& errors) {
    const std::vector<std::string> baseline_errors = state.hidden_truth.validate();
    HiddenTruthGraph simulated = state.hidden_truth;
    try {
        for (const Entity& entity : cluster.proposed_entities) {
            simulated.add_entity(entity);
        }
        for (const Event& event : cluster.proposed_events) {
            simulated.add_event(event);
        }
    } catch (const std::exception& ex) {
        add_error(errors, std::string("simulated hidden cluster insertion failed: ") + ex.what());
        return;
    }

    const std::vector<std::string> simulated_errors = simulated.validate();
    for (const std::string& error : simulated_errors) {
        if (std::find(baseline_errors.begin(), baseline_errors.end(), error) != baseline_errors.end()) {
            continue;
        }
        add_error(errors, "simulated hidden graph validation: " + error);
    }
}

[[nodiscard]] bool cluster_touches_protected_mystery(const GeneratedHiddenTimelineCluster& cluster) {
    return cluster.resolved_target.has_value() && cluster.resolved_target->mystery_id.has_value() && cluster.proposed_events.size() >= 2U;
}

[[nodiscard]] std::string target_resolution_text(const GeneratedHiddenTimelineCluster& cluster, AccessLevel access) {
    if (!cluster.resolved_target.has_value()) {
        return "unresolved";
    }
    std::ostringstream out;
    out << cluster.resolved_target->topic;
    if (cluster.resolved_target->mystery_id.has_value() && can_view(access, AccessLevel::Curator)) {
        out << " -> " << *cluster.resolved_target->mystery_id;
    }
    return out.str();
}

[[nodiscard]] std::string nonempty_cluster_topic(const GeneratedHiddenTimelineCluster& cluster) {
    if (cluster.resolved_target.has_value() && !cluster.resolved_target->topic.empty()) {
        return cluster.resolved_target->topic;
    }
    if (!cluster.request.target_topic.empty()) {
        return cluster.request.target_topic;
    }
    return "unresolved";
}

[[nodiscard]] std::string hidden_cluster_source_id(const GeneratedHiddenTimelineCluster& cluster) {
    const HiddenTimelineClusterRequest& request = cluster.request;
    const std::size_t cluster_size = cluster.proposed_entities.size() + cluster.proposed_events.size();
    std::ostringstream out;
    out << "hidden_cluster." << to_string(request.scope) << "." << claim_suffix_for_id(nonempty_cluster_topic(cluster))
        << "." << request.start_year << "_" << request.end_year << "." << request.seed << "."
        << cluster_digest(request, cluster_size, "source");
    return out.str();
}

[[nodiscard]] std::string hidden_mutation_record_id(const GeneratedHiddenTimelineCluster& cluster) {
    const HiddenTimelineClusterRequest& request = cluster.request;
    const std::size_t cluster_size = cluster.proposed_entities.size() + cluster.proposed_events.size();
    std::ostringstream out;
    out << "mutation.hidden_truth." << to_string(request.scope) << "." << claim_suffix_for_id(nonempty_cluster_topic(cluster))
        << "." << request.start_year << "_" << request.end_year << "." << request.seed << "."
        << cluster_digest(request, cluster_size + 1U, "mutation");
    return out.str();
}

[[nodiscard]] std::string cluster_public_label_from_key(std::string_view key) {
    std::string text;
    bool capitalize_next = true;
    for (char ch : key) {
        if (ch == '_' || ch == '-') {
            text.push_back(' ');
            capitalize_next = true;
            continue;
        }
        if (capitalize_next) {
            text.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            capitalize_next = false;
        } else {
            text.push_back(ch);
        }
    }
    return text;
}

[[nodiscard]] std::string cluster_key_from_id(std::string_view id) {
    const std::size_t last_dot = id.rfind('.');
    if (last_dot == std::string_view::npos || last_dot + 1U >= id.size()) {
        return std::string(id);
    }
    return std::string(id.substr(last_dot + 1U));
}

[[nodiscard]] std::string cluster_label_from_entity_id(std::string_view id) {
    return cluster_public_label_from_key(cluster_key_from_id(id));
}

[[nodiscard]] int earliest_after_source_events(const ArchiveEngineState& state,
                                               const SpecGenerationTargetSource& source,
                                               int requested_start_year) {
    int year = requested_start_year;
    for (const std::string& event_id : source.source_event_ids) {
        if (const Event* event = state.hidden_truth.find_event(event_id)) {
            year = std::max(year, event->end_year + 1);
        }
    }
    return year;
}

[[nodiscard]] bool add_spec_hidden_cluster(const ArchiveEngineState& state,
                                           const HiddenTimelineClusterRequest& request,
                                           GeneratedHiddenTimelineCluster& cluster,
                                           const GenerationTarget& target,
                                           const std::string& base_suffix) {
    if (!target.spec_source.has_value() || !state.civilization_source.has_value()) {
        return false;
    }
    if (request.scope != HiddenClusterScope::InstitutionOrigin) {
        cluster.rationale = "spec-bootstrapped hidden cluster compatibility currently supports institution_origin scope only";
        return true;
    }

    const SpecGenerationTargetSource& source = *target.spec_source;
    const int first_allowed_year = earliest_after_source_events(state, source, request.start_year);
    if (first_allowed_year > request.end_year) {
        cluster.rationale = "spec target source events occur after the requested hidden-cluster window; no cluster generated";
        return true;
    }
    const int pressure_year = std::max(first_allowed_year, clamp_year_to_window(first_allowed_year, request, 2));
    const int response_year = std::max(pressure_year, clamp_year_to_window(first_allowed_year + 4, request, 8));
    const int record_year = std::max(response_year, clamp_year_to_window(first_allowed_year + 8, request, 13));

    const std::string topic_suffix = claim_suffix_for_id(target.topic);
    const std::string actor_id = "entity.generated." + source.civilization_id + "." + topic_suffix + ".witness_lineage." + base_suffix;
    const std::string office_id = "entity.generated." + source.civilization_id + "." + topic_suffix + ".review_office." + base_suffix;
    const std::string pressure_event_id = "event.generated." + source.civilization_id + "." + topic_suffix + ".pressure." + base_suffix;
    const std::string response_event_id = "event.generated." + source.civilization_id + "." + topic_suffix + ".response." + base_suffix;
    const std::string record_event_id = "event.generated." + source.civilization_id + "." + topic_suffix + ".record." + base_suffix;

    const std::string label = source.source_entity_ids.empty()
        ? cluster_public_label_from_key(target.topic)
        : cluster_label_from_entity_id(source.source_entity_ids.front());

    cluster.proposed_entities.push_back(Entity{
        actor_id,
        EntityType::Faction,
        "Generated " + label + " Witness Lineage",
        {"Spec Bootstrap Witnesses"},
        Interval{pressure_year, state.civilization_source->latest_year},
        pressure_event_id,
        "",
        AccessLevel::Canon,
    });
    cluster.proposed_entities.push_back(Entity{
        office_id,
        EntityType::Office,
        "Generated " + label + " Review Office",
        {"Spec Bootstrap Review Office"},
        Interval{response_year, state.civilization_source->latest_year},
        response_event_id,
        "",
        AccessLevel::Canon,
    });

    std::vector<std::string> pressure_participants = source.source_entity_ids;
    add_unique_string(pressure_participants, actor_id);
    std::vector<std::string> pressure_causes = source.source_event_ids;
    if (pressure_causes.empty()) {
        pressure_causes.push_back("event." + source.civilization_id + ".foundation");
    }

    cluster.proposed_events.push_back(Event{
        pressure_event_id,
        "spec_generated_pressure",
        "Generated spec pressure around " + cluster_public_label_from_key(target.topic),
        pressure_year,
        pressure_year,
        pressure_participants,
        pressure_causes,
        source.source_entity_ids,
        "A spec-scoped generated pressure episode extends the selected civilization without touching fixed-fixture IDs.",
        TruthLayer::CanonicalTruth,
        AccessLevel::Canon,
    });
    cluster.proposed_events.push_back(Event{
        response_event_id,
        "spec_generated_institutional_response",
        "Generated spec response around " + cluster_public_label_from_key(target.topic),
        response_year,
        response_year,
        {actor_id, office_id},
        {pressure_event_id},
        source.source_entity_ids,
        "A spec-scoped response event records how the target pressure becomes reviewable institutional context.",
        TruthLayer::CanonicalTruth,
        AccessLevel::Canon,
    });
    cluster.proposed_events.push_back(Event{
        record_event_id,
        "spec_generated_recordkeeping_trace",
        "Generated spec recordkeeping trace around " + cluster_public_label_from_key(target.topic),
        record_year,
        record_year,
        {office_id},
        {response_event_id},
        source.source_entity_ids,
        "A spec-scoped trace event prepares future public evidence without generating artifacts yet.",
        TruthLayer::CanonicalTruth,
        AccessLevel::Canon,
    });

    cluster.causal_links = {
        make_proposed_causal_link(pressure_causes.front(), pressure_event_id, "spec source pressure enables generated extension"),
        make_proposed_causal_link(pressure_event_id, response_event_id, "pressure receives institutional response"),
        make_proposed_causal_link(response_event_id, record_event_id, "response leaves recordkeeping trace"),
    };
    cluster.rationale = "Generated a spec-scoped hidden cluster from the selected CivilizationSpec target without reusing fixed-fixture IDs.";
    return true;
}

[[nodiscard]] HiddenTruthMutationRecord build_hidden_truth_mutation_record(
    const GeneratedHiddenTimelineCluster& cluster,
    const HiddenTimelineClusterEvaluation& evaluation,
    const HiddenClusterMaterializationResult& result,
    AccessLevel access
) {
    HiddenTruthMutationRecord record;
    record.id = hidden_mutation_record_id(cluster);
    record.source_cluster_id = hidden_cluster_source_id(cluster);
    record.source_decision = evaluation.decision;
    record.inserted_entity_ids = result.inserted_entity_ids;
    record.inserted_event_ids = result.inserted_event_ids;
    record.target_topic = nonempty_cluster_topic(cluster);
    record.cluster_scope = to_string(cluster.request.scope);
    record.start_year = cluster.request.start_year;
    record.end_year = cluster.request.end_year;
    record.seed = cluster.request.seed;
    record.authorized_access_level = to_string(access);
    record.algorithm_version = "v24.1.hidden_mutation_audit";
    record.validation_summary = "hidden graph validation passed; full archive validation passed after insertion and audit record creation";
    return record;
}

} // namespace

[[nodiscard]] GeneratedHiddenTimelineCluster generate_hidden_timeline_cluster(
    const ArchiveEngineState& state,
    const HiddenTimelineClusterRequest& request
) {
    GeneratedHiddenTimelineCluster cluster;
    cluster.request = request;
    cluster.resolved_target = resolve_generation_target(state, request.target_topic);

    if (!cluster.resolved_target.has_value()) {
        cluster.rationale = "target topic did not resolve; no hidden cluster generated";
        return cluster;
    }
    if (request.start_year <= 0 || request.end_year <= 0 || request.start_year > request.end_year) {
        cluster.rationale = "invalid year window; no hidden cluster generated";
        return cluster;
    }

    const std::string topic_suffix = claim_suffix_for_id(cluster.resolved_target->topic);
    const std::string base_suffix = topic_suffix + "_" + std::to_string(request.start_year) + "_" +
                                    std::to_string(request.end_year) + "_" + cluster_digest(request, 0U, "cluster");

    if (cluster.resolved_target->spec_source.has_value()) {
        if (add_spec_hidden_cluster(state, request, cluster, *cluster.resolved_target, base_suffix)) {
            return cluster;
        }
    }

    const int pressure_year = clamp_year_to_window(604, request, 3);
    const int compact_year = clamp_year_to_window(613, request, 14);
    const int codification_year = clamp_year_to_window(619, request, 20);

    if (request.scope == HiddenClusterScope::InstitutionOrigin && cluster.resolved_target->topic == "lock_authority") {
        const std::string faction_id = "faction.generated_lower_lock_keepers." + base_suffix;
        const std::string office_id = "office.generated_mouth_counters." + base_suffix;
        const std::string pressure_event_id = "event.generated.reservoir_pressure." + base_suffix;
        const std::string compact_event_id = "event.generated.emergency_lock_compact." + base_suffix;
        const std::string ritual_event_id = "event.generated.ritual_lock_codification." + base_suffix;

        cluster.proposed_entities.push_back(Entity{
            faction_id,
            EntityType::Faction,
            "Generated Lower Lock Keepers",
            {"Lower Lock Keeper Lineage"},
            Interval{pressure_year, 760},
            pressure_event_id,
            "",
            AccessLevel::Canon,
        });
        cluster.proposed_entities.push_back(Entity{
            office_id,
            EntityType::Office,
            "Generated Mouth Counters",
            {"Counters of the Lower Lock Mouths"},
            Interval{compact_year, 760},
            compact_event_id,
            "",
            AccessLevel::Canon,
        });

        cluster.proposed_events.push_back(Event{
            pressure_event_id,
            "ecological_pressure",
            "Generated lower-lock reservoir pressure",
            pressure_year,
            pressure_year,
            {"person.ivara", faction_id},
            {"event.reservoir_mismanagement"},
            {"site.reservoir_gate"},
            "A generated pressure episode makes lower-lock keepers politically relevant before the schism.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        });
        cluster.proposed_events.push_back(Event{
            compact_event_id,
            "institutional_compact",
            "Generated emergency lock compact",
            compact_year,
            compact_year,
            {"person.ivara", faction_id},
            {"event.temple_revolt", pressure_event_id},
            {"site.reservoir_gate", faction_id},
            "A generated emergency compact gives lock-keepers a narrow administrative role before later ritual codification.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        });
        cluster.proposed_events.push_back(Event{
            ritual_event_id,
            "ritual_codification",
            "Generated ritual lock authority codification",
            codification_year,
            codification_year,
            {"person.ivara", faction_id},
            {compact_event_id, "event.salt_moon_schism"},
            {"site.reservoir_gate", "office.drowned_chancellor", office_id},
            "A generated codification event explains why later fragments blur office, keeper, and ritual authority.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        });

        cluster.causal_links = {
            make_proposed_causal_link(pressure_event_id, compact_event_id, "pressure enables emergency compact"),
            make_proposed_causal_link(compact_event_id, ritual_event_id, "compact is later codified ritually"),
            make_proposed_causal_link("event.salt_moon_schism", ritual_event_id, "schism supplies ritual pressure"),
        };
        cluster.rationale = "Generated an institution-origin cluster connecting ecological pressure, emergency compact, and ritualized lock authority.";
        return cluster;
    }

    if (cluster.resolved_target->topic == "silt_levy") {
        const std::string faction_id = "faction.generated_levy_petitioners." + base_suffix;
        const std::string pressure_event_id = "event.generated.levy_pressure." + base_suffix;
        const std::string petition_event_id = "event.generated.silt_levy_petition." + base_suffix;
        cluster.proposed_entities.push_back(Entity{
            faction_id,
            EntityType::Faction,
            "Generated Levy Petitioners",
            {"Lower Lock Levy Witnesses"},
            Interval{std::max(request.start_year, 605), 760},
            pressure_event_id,
            "",
            AccessLevel::Canon,
        });
        cluster.proposed_events.push_back(Event{
            pressure_event_id,
            "ecological_pressure",
            "Generated levy pressure episode",
            std::max(request.start_year, 605),
            std::max(request.start_year, 605),
            {"person.ivara"},
            {"event.canal_tax_reform"},
            {"site.reservoir_gate"},
            "A generated pressure episode creates additional hidden context for levy records.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        });
        cluster.proposed_events.push_back(Event{
            petition_event_id,
            "petition",
            "Generated silt levy petition",
            std::max(request.start_year, 608),
            std::max(request.start_year, 608),
            {"person.ivara", faction_id},
            {pressure_event_id},
            {"site.reservoir_gate", faction_id},
            "A generated petition supports future silt levy evidence without linking to lock-authority mysteries.",
            TruthLayer::CanonicalTruth,
            AccessLevel::Canon,
        });
        cluster.causal_links = {make_proposed_causal_link(pressure_event_id, petition_event_id, "pressure motivates petition")};
        cluster.rationale = "Generated a levy-specific hidden cluster; no Third Lock Authority mystery binding was added.";
        return cluster;
    }

    cluster.rationale = "resolved target has no v23 cluster generator for the requested scope; no hidden cluster generated";
    return cluster;
}

[[nodiscard]] HiddenTimelineClusterEvaluation evaluate_hidden_timeline_cluster(
    const ArchiveEngineState& state,
    const GeneratedHiddenTimelineCluster& cluster
) {
    HiddenTimelineClusterEvaluation evaluation;
    evaluation.cluster = cluster;
    evaluation.simulated_on_copy = true;
    evaluation.would_mutate_hidden_truth = false;

    add_cluster_local_errors(state, cluster, evaluation.validation_errors);
    add_simulated_cluster_errors(state, cluster, evaluation.validation_errors);

    if (cluster.resolved_target.has_value()) {
        if (cluster.resolved_target->mystery_id.has_value()) {
            evaluation.affected_mystery_ids.push_back(*cluster.resolved_target->mystery_id);
        }
        evaluation.affected_claim_ids = cluster.resolved_target->claim_ids;
    }

    if (!cluster.proposed_events.empty()) {
        evaluation.predicted_public_archive_effects.push_back("could provide hidden context for future generated artifacts after a later curator/canon/debug materialization gate");
    }
    if (cluster_touches_protected_mystery(cluster)) {
        evaluation.predicted_public_archive_effects.push_back("could affect protected-mystery pressure and should remain advisory until migration planning is reviewed");
    }

    if (!evaluation.validation_errors.empty()) {
        evaluation.decision = HiddenClusterDecision::Reject;
        evaluation.assessment = "Reject: cluster simulation introduced validation errors; hidden truth was not mutated.";
    } else if (cluster_touches_protected_mystery(cluster)) {
        evaluation.decision = HiddenClusterDecision::NeedsCuratorReview;
        evaluation.assessment = "Needs curator review: structurally valid cluster touches a protected mystery and may alter future interpretive pressure.";
    } else {
        evaluation.decision = HiddenClusterDecision::AcceptableCluster;
        evaluation.assessment = "Acceptable cluster: copied-graph simulation passed and no protected mystery pressure was detected.";
    }
    return evaluation;
}

[[nodiscard]] std::string format_hidden_timeline_cluster(
    const ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTimelineClusterRequest& request
) {
    std::ostringstream out;
    out << "Hidden timeline cluster visible to " << to_string(access) << ":\n";
    out << "- cluster_scope: " << to_string(request.scope) << "\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- year_window: " << request.start_year << "-" << request.end_year << "\n";

    const std::string before = serialize_for_replay_test(state);
    const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
    const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
    const std::string after = serialize_for_replay_test(state);
    out << "- archive_mutated: " << (before == after ? "false" : "true") << "\n";
    out << "- would_mutate_hidden_truth: " << (evaluation.would_mutate_hidden_truth ? "true" : "false") << "\n";

    if (!can_view(access, AccessLevel::Curator)) {
        out << "- hidden cluster internals are restricted to curator/canon/debug access\n";
        return out.str();
    }

    out << "- target_resolution: " << target_resolution_text(cluster, access) << "\n";
    if (cluster.resolved_target.has_value() && cluster.resolved_target->spec_source.has_value() &&
        can_view(access, AccessLevel::Curator)) {
        const SpecGenerationTargetSource& source = *cluster.resolved_target->spec_source;
        out << "Spec target source:\n";
        out << "- civilization_id: " << source.civilization_id << "\n";
        out << "- target_kind: " << source.target_kind << "\n";
        out << "- target_key: " << source.target_key << "\n";
        out << "- source_entities:";
        for (const std::string& id : source.source_entity_ids) {
            out << " " << id;
        }
        out << "\n";
        out << "- source_events:";
        for (const std::string& id : source.source_event_ids) {
            out << " " << id;
        }
        out << "\n";
    }
    out << "- decision: " << to_string(evaluation.decision) << "\n";
    out << "- simulated_on_copy: " << (evaluation.simulated_on_copy ? "true" : "false") << "\n";
    out << "- rationale: " << cluster.rationale << "\n";
    out << "- assessment: " << evaluation.assessment << "\n";

    if (!cluster.proposed_entities.empty()) {
        out << "Proposed entities:\n";
        for (const Entity& entity : cluster.proposed_entities) {
            out << "- " << entity.id << " [" << to_string(entity.type) << ", "
                << interval_text(entity.existence_interval) << "]: " << entity.canonical_name << "\n";
        }
    }
    if (!cluster.proposed_events.empty()) {
        out << "Proposed events:\n";
        for (const Event& event : cluster.proposed_events) {
            out << "- " << event.id << " (" << event.start_year;
            if (event.end_year != event.start_year) {
                out << "-" << event.end_year;
            }
            out << "): " << event.title << "\n";
        }
    }
    if (!cluster.causal_links.empty()) {
        out << "Causal links:\n";
        for (const ProposedCausalLink& link : cluster.causal_links) {
            out << "- " << format_proposed_causal_link(link) << "\n";
        }
    }
    if (!evaluation.affected_mystery_ids.empty()) {
        out << "Affected mysteries:\n";
        for (const std::string& id : evaluation.affected_mystery_ids) {
            out << "- " << id << "\n";
        }
    }
    if (!evaluation.affected_claim_ids.empty()) {
        out << "Affected claims:\n";
        for (const std::string& id : evaluation.affected_claim_ids) {
            out << "- " << id << "\n";
        }
    }
    if (!evaluation.predicted_public_archive_effects.empty()) {
        out << "Predicted public archive effects:\n";
        for (const std::string& effect : evaluation.predicted_public_archive_effects) {
            out << "- " << effect << "\n";
        }
    }
    if (!evaluation.validation_errors.empty()) {
        out << "Validation errors:\n";
        for (const std::string& error : evaluation.validation_errors) {
            out << "- " << error << "\n";
        }
    }
    if (access == AccessLevel::Debug) {
        out << "Debug trace:\n";
        out << "- seed: " << request.seed << "\n";
        out << "- proposed_entity_count: " << cluster.proposed_entities.size() << "\n";
        out << "- proposed_event_count: " << cluster.proposed_events.size() << "\n";
    }
    return out.str();
}


[[nodiscard]] HiddenClusterMaterializationResult materialize_hidden_timeline_cluster(
    ArchiveEngineState& state,
    const GeneratedHiddenTimelineCluster& cluster,
    AccessLevel access
) {
    HiddenClusterMaterializationResult result;

    if (!can_view(access, AccessLevel::Curator)) {
        result.explanation = "hidden cluster materialization requires curator/canon/debug access; hidden truth was not mutated.";
        return result;
    }

    const HiddenTimelineClusterEvaluation evaluation = evaluate_hidden_timeline_cluster(state, cluster);
    result.source_decision = evaluation.decision;
    if (evaluation.decision == HiddenClusterDecision::Reject) {
        result.validation_errors = evaluation.validation_errors;
        result.explanation = "hidden cluster evaluation rejected the cluster; hidden truth was not mutated.";
        return result;
    }

    ArchiveEngineState snapshot = state;
    try {
        for (const Entity& entity : cluster.proposed_entities) {
            state.hidden_truth.add_entity(entity);
            result.inserted_entity_ids.push_back(entity.id);
        }
        for (const Event& event : cluster.proposed_events) {
            state.hidden_truth.add_event(event);
            result.inserted_event_ids.push_back(event.id);
        }

        std::vector<std::string> errors = state.hidden_truth.validate();
        const std::vector<std::string> full_errors = validate_full_state(state);
        errors.insert(errors.end(), full_errors.begin(), full_errors.end());
        if (!errors.empty()) {
            result.validation_errors = errors;
            state = snapshot;
            result.inserted_entity_ids.clear();
            result.inserted_event_ids.clear();
            result.explanation = "post-mutation validation failed; hidden cluster insertion rolled back.";
            result.mutated = false;
            return result;
        }
    } catch (const std::exception& ex) {
        result.validation_errors.push_back(std::string("hidden cluster insertion failed: ") + ex.what());
        state = snapshot;
        result.inserted_entity_ids.clear();
        result.inserted_event_ids.clear();
        result.explanation = "hidden cluster insertion threw an exception; hidden truth was rolled back.";
        result.mutated = false;
        return result;
    }

    HiddenTruthMutationRecord record = build_hidden_truth_mutation_record(cluster, evaluation, result, access);
    result.mutation_record_id = record.id;
    state.hidden_truth_mutations.push_back(std::move(record));

    const std::vector<std::string> final_errors = validate_full_state(state);
    if (!final_errors.empty()) {
        result.validation_errors = final_errors;
        state = snapshot;
        result.inserted_entity_ids.clear();
        result.inserted_event_ids.clear();
        result.mutation_record_id.clear();
        result.explanation = "post-audit validation failed; hidden cluster insertion and mutation record creation rolled back.";
        result.mutated = false;
        return result;
    }

    result.mutated = true;
    if (evaluation.decision == HiddenClusterDecision::NeedsCuratorReview) {
        result.explanation = "curator-or-higher-approved hidden cluster materialized after protected-mystery review; hidden graph, full archive validation, and mutation-audit recording passed.";
    } else {
        result.explanation = "hidden cluster materialized through curator-or-higher-approved insertion; hidden graph, full archive validation, and mutation-audit recording passed.";
    }
    return result;
}

[[nodiscard]] std::string format_hidden_cluster_materialization_query(
    ArchiveEngineState& state,
    AccessLevel access,
    const HiddenTimelineClusterRequest& request
) {
    std::ostringstream out;
    out << "Hidden cluster materialization visible to " << to_string(access) << ":\n";
    out << "- cluster_scope: " << to_string(request.scope) << "\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- year_window: " << request.start_year << "-" << request.end_year << "\n";

    const GeneratedHiddenTimelineCluster cluster = generate_hidden_timeline_cluster(state, request);
    const HiddenClusterMaterializationResult result = materialize_hidden_timeline_cluster(state, cluster, access);

    out << "- source_decision: " << to_string(result.source_decision) << "\n";
    out << "- mutated: " << (result.mutated ? "true" : "false") << "\n";
    out << "- explanation: " << result.explanation << "\n";

    if (!can_view(access, AccessLevel::Curator)) {
        out << "- hidden cluster materialization internals are restricted to curator/canon/debug access\n";
        return out.str();
    }

    out << "- target_resolution: " << target_resolution_text(cluster, access) << "\n";
    if (cluster.resolved_target.has_value() && cluster.resolved_target->spec_source.has_value()) {
        const SpecGenerationTargetSource& source = *cluster.resolved_target->spec_source;
        out << "Spec target source:\n";
        out << "- civilization_id: " << source.civilization_id << "\n";
        out << "- target_kind: " << source.target_kind << "\n";
        out << "- target_key: " << source.target_key << "\n";
        out << "- source_entities:";
        for (const std::string& id : source.source_entity_ids) {
            out << " " << id;
        }
        out << "\n";
        out << "- source_events:";
        for (const std::string& id : source.source_event_ids) {
            out << " " << id;
        }
        out << "\n";
    }
    if (!result.mutation_record_id.empty()) {
        out << "- mutation_record_id: " << result.mutation_record_id << "\n";
    }
    if (!result.inserted_entity_ids.empty()) {
        out << "Inserted entities:\n";
        for (const std::string& id : result.inserted_entity_ids) {
            out << "- " << id << "\n";
        }
    }
    if (!result.inserted_event_ids.empty()) {
        out << "Inserted events:\n";
        for (const std::string& id : result.inserted_event_ids) {
            out << "- " << id << "\n";
        }
    }
    if (!result.validation_errors.empty()) {
        out << "Validation errors:\n";
        for (const std::string& error : result.validation_errors) {
            out << "- " << error << "\n";
        }
    }
    if (access == AccessLevel::Debug) {
        out << "Debug trace:\n";
        out << "- proposed_entity_count: " << cluster.proposed_entities.size() << "\n";
        out << "- proposed_event_count: " << cluster.proposed_events.size() << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_hidden_truth_mutations(const ArchiveEngineState& state, AccessLevel access) {
    std::ostringstream out;
    out << "Hidden truth mutation records visible to " << to_string(access) << ":\n";
    if (!can_view(access, AccessLevel::Curator)) {
        out << "- record_count: redacted\n";
        out << "- hidden mutation audit internals are restricted to curator/canon/debug access\n";
        return out.str();
    }

    out << "- record_count: " << state.hidden_truth_mutations.size() << "\n";
    if (state.hidden_truth_mutations.empty()) {
        out << "- none\n";
        return out.str();
    }

    for (const HiddenTruthMutationRecord& record : state.hidden_truth_mutations) {
        out << "Record " << record.id << ":\n";
        out << "- source_cluster_id: " << record.source_cluster_id << "\n";
        out << "- source_decision: " << to_string(record.source_decision) << "\n";
        out << "- target_topic: " << record.target_topic << "\n";
        out << "- cluster_scope: " << record.cluster_scope << "\n";
        out << "- year_window: " << record.start_year << "-" << record.end_year << "\n";
        out << "- seed: " << record.seed << "\n";
        out << "- authorized_access_level: " << record.authorized_access_level << "\n";
        out << "- algorithm_version: " << record.algorithm_version << "\n";
        out << "- validation_summary: " << record.validation_summary << "\n";
        if (!record.inserted_entity_ids.empty()) {
            out << "Inserted entities:\n";
            for (const std::string& id : record.inserted_entity_ids) {
                out << "- " << id << "\n";
            }
        }
        if (!record.inserted_event_ids.empty()) {
            out << "Inserted events:\n";
            for (const std::string& id : record.inserted_event_ids) {
                out << "- " << id << "\n";
            }
        }
    }
    return out.str();
}

} // namespace archive
