#pragma once
#include "archive_common.h"

namespace archive {

enum class EntityType {
    Person,
    Office,
    Settlement,
    Site,
    Technology,
    Language,
    Dialect,
    Script,
    Faction,
};

enum class TruthLayer {
    CanonicalTruth,
    PublicInference,
    MythicTruth,
    ForgedTruth,
    ProtectedMystery,
};


struct Entity {
    std::string id;
    EntityType type = EntityType::Person;
    std::string canonical_name;
    std::vector<std::string> alternate_names;
    Interval existence_interval;
    std::string created_by_event_id;
    std::string ended_by_event_id;
    AccessLevel min_access = AccessLevel::Canon;
};

struct Event {
    std::string id;
    std::string event_type;
    std::string title;
    int start_year = 0;
    int end_year = 0;
    std::vector<std::string> participant_entity_ids;
    std::vector<std::string> cause_event_ids;
    std::vector<std::string> required_entity_ids;
    std::string canonical_description;
    TruthLayer truth_layer = TruthLayer::CanonicalTruth;
    AccessLevel min_access = AccessLevel::Canon;
};


class HiddenTruthGraph {
public:
    void add_entity(Entity entity) {
        const std::string id = entity.id;
        auto [it, inserted] = entities_.emplace(id, std::move(entity));
        (void)it;
        if (!inserted) {
            throw std::runtime_error("duplicate entity id: " + id);
        }
    }

    void add_event(Event event) {
        const std::string id = event.id;
        auto [it, inserted] = events_.emplace(id, std::move(event));
        (void)it;
        if (!inserted) {
            throw std::runtime_error("duplicate event id: " + id);
        }
    }

    [[nodiscard]] const Entity* find_entity(const std::string& id) const {
        const auto it = entities_.find(id);
        if (it == entities_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] const Event* find_event(const std::string& id) const {
        const auto it = events_.find(id);
        if (it == events_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    [[nodiscard]] const std::map<std::string, Entity>& entities() const {
        return entities_;
    }

    [[nodiscard]] const std::map<std::string, Event>& events() const {
        return events_;
    }

    [[nodiscard]] std::vector<std::string> validate() const {
        std::vector<std::string> errors;

        for (const auto& [entity_id, entity] : entities_) {
            if (!entity.created_by_event_id.empty() && find_event(entity.created_by_event_id) == nullptr) {
                errors.push_back("entity " + entity_id + " references missing created_by_event_id " + entity.created_by_event_id);
            }
            if (!entity.ended_by_event_id.empty() && find_event(entity.ended_by_event_id) == nullptr) {
                errors.push_back("entity " + entity_id + " references missing ended_by_event_id " + entity.ended_by_event_id);
            }
        }

        for (const auto& [event_id, event] : events_) {
            if (event.end_year < event.start_year) {
                errors.push_back("event " + event_id + " ends before it starts");
            }

            for (const std::string& cause_id : event.cause_event_ids) {
                const Event* cause = find_event(cause_id);
                if (cause == nullptr) {
                    errors.push_back("event " + event_id + " references missing cause " + cause_id);
                    continue;
                }
                if (cause->end_year > event.start_year) {
                    errors.push_back("event " + event_id + " occurs before cause " + cause_id + " ends");
                }
            }

            for (const std::string& participant_id : event.participant_entity_ids) {
                const Entity* participant = find_entity(participant_id);
                if (participant == nullptr) {
                    errors.push_back("event " + event_id + " references missing participant " + participant_id);
                    continue;
                }
                if (!participant->existence_interval.contains(event.start_year) ||
                    !participant->existence_interval.contains(event.end_year)) {
                    errors.push_back("participant " + participant_id + " does not exist during event " + event_id);
                }
            }

            for (const std::string& required_id : event.required_entity_ids) {
                const Entity* required = find_entity(required_id);
                if (required == nullptr) {
                    errors.push_back("event " + event_id + " references missing required entity " + required_id);
                    continue;
                }
                if (!required->existence_interval.contains(event.start_year) ||
                    !required->existence_interval.contains(event.end_year)) {
                    errors.push_back("required entity " + required_id + " unavailable during event " + event_id);
                }
            }
        }

        // Detect causal cycles independently of date ordering. Same-year or
        // otherwise boundary-valid cycles are still invalid hidden causality.
        std::map<std::string, int> visit_state;
        auto dfs = [&](auto&& self, const std::string& event_id, std::vector<std::string>& stack) -> void {
            const int state = visit_state[event_id];
            if (state == 1) {
                errors.push_back("causal cycle detected involving event " + event_id);
                return;
            }
            if (state == 2) {
                return;
            }

            visit_state[event_id] = 1;
            stack.push_back(event_id);
            const Event* event = find_event(event_id);
            if (event != nullptr) {
                for (const std::string& cause_id : event->cause_event_ids) {
                    if (find_event(cause_id) != nullptr) {
                        self(self, cause_id, stack);
                    }
                }
            }
            stack.pop_back();
            visit_state[event_id] = 2;
        };

        for (const auto& [event_id, event] : events_) {
            (void)event;
            if (visit_state[event_id] == 0) {
                std::vector<std::string> stack;
                dfs(dfs, event_id, stack);
            }
        }

        return errors;
    }

private:
    std::map<std::string, Entity> entities_;
    std::map<std::string, Event> events_;
};


enum class HiddenProposalType {
    Entity,
    Event,
};

enum class HiddenProposalDecision {
    AcceptableProposal,
    Reject,
    NeedsCuratorReview,
};

struct HiddenProposal {
    std::string id;
    HiddenProposalType type = HiddenProposalType::Event;
    std::string target_topic;
    int requested_target_year = 0;
    std::string description;
    std::optional<Entity> proposed_entity;
    std::optional<Event> proposed_event;
    std::vector<std::string> predicted_archive_implications;
};

struct HiddenProposalEvaluation {
    std::string evaluated_proposal_id;
    HiddenProposalDecision decision = HiddenProposalDecision::Reject;
    std::vector<std::string> validation_errors;
    std::vector<std::string> predicted_archive_implications;
    std::string explanation;
};

// v22 migration planning is intentionally advisory. It describes what a later
// hidden-truth acceptance step would change, but it never mutates live state.
struct HiddenProposalMigrationPlan {
    std::string proposal_id;
    HiddenProposalDecision proposal_decision = HiddenProposalDecision::Reject;
    std::vector<Entity> projected_entities;
    std::vector<Event> projected_events;
    std::vector<std::string> affected_mystery_ids;
    std::vector<std::string> affected_claim_ids;
    std::vector<std::string> predicted_public_archive_effects;
    std::vector<std::string> validation_errors;
    bool would_mutate_hidden_truth = false;
};

// v23 hidden timeline cluster generation. Clusters are generated and evaluated
// as connected hidden-world slices on a copied HiddenTruthGraph. They remain
// advisory only; v23 never mutates live hidden truth. v24.2 keeps the
// review/display causal metadata typed instead of storing rendered strings.
enum class HiddenClusterScope {
    InstitutionOrigin,
    SchismPrecursor,
    EcologicalPressure,
    PoliticalRealignment,
    RitualCodification,
};

struct HiddenTimelineClusterRequest {
    HiddenClusterScope scope = HiddenClusterScope::InstitutionOrigin;
    std::string target_topic;
    int start_year = 0;
    int end_year = 0;
    std::uint64_t seed = 0;
};

struct ProposedCausalLink {
    std::string cause_event_id;
    std::string effect_event_id;
    std::string explanation;
};

struct GeneratedHiddenTimelineCluster {
    HiddenTimelineClusterRequest request;
    std::optional<GenerationTarget> resolved_target;
    std::vector<Entity> proposed_entities;
    std::vector<Event> proposed_events;
    std::vector<ProposedCausalLink> causal_links;
    std::string rationale;
};

enum class HiddenClusterDecision {
    AcceptableCluster,
    Reject,
    NeedsCuratorReview,
};

struct HiddenTimelineClusterEvaluation {
    HiddenClusterDecision decision = HiddenClusterDecision::Reject;
    GeneratedHiddenTimelineCluster cluster;
    std::vector<std::string> validation_errors;
    std::vector<std::string> affected_mystery_ids;
    std::vector<std::string> affected_claim_ids;
    std::vector<std::string> predicted_public_archive_effects;
    bool simulated_on_copy = false;
    bool would_mutate_hidden_truth = false;
    std::string assessment;
};

// v24 controlled hidden-cluster materialization. This is the first hidden-truth
// mutation path, but it is still explicit, curator/canon/debug-gated, freshly
// evaluated, snapshot/rollback protected, and fully validated after insertion.
// v24.1 adds first-class provenance records for successful hidden-truth
// mutations. Failed or rejected materializations must not create records.
struct HiddenTruthMutationRecord {
    std::string id;
    std::string source_cluster_id;
    HiddenClusterDecision source_decision = HiddenClusterDecision::Reject;
    std::vector<std::string> inserted_entity_ids;
    std::vector<std::string> inserted_event_ids;
    std::string target_topic;
    std::string cluster_scope;
    int start_year = 0;
    int end_year = 0;
    std::uint64_t seed = 0;
    std::string authorized_access_level;
    std::string algorithm_version;
    std::string validation_summary;
};

struct HiddenClusterMaterializationResult {
    bool mutated = false;
    HiddenClusterDecision source_decision = HiddenClusterDecision::Reject;
    std::vector<std::string> inserted_entity_ids;
    std::vector<std::string> inserted_event_ids;
    std::string mutation_record_id;
    std::vector<std::string> validation_errors;
    std::string explanation;
};


[[nodiscard]] std::string to_string(EntityType type);
[[nodiscard]] std::string to_string(TruthLayer layer);
[[nodiscard]] bool type_is_allowed(EntityType actual, std::initializer_list<EntityType> allowed);
[[nodiscard]] std::string to_string(HiddenProposalType type);
[[nodiscard]] std::string to_string(HiddenProposalDecision decision);
[[nodiscard]] std::string to_string(HiddenClusterScope scope);
[[nodiscard]] HiddenClusterScope parse_hidden_cluster_scope(std::string_view text);
[[nodiscard]] std::string to_string(HiddenClusterDecision decision);

} // namespace archive
