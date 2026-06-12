/*
 * Hidden entity/event proposal gate.
 *
 * v21 intentionally stops at proposals: it can generate and validate possible
 * additions to HiddenTruthGraph, but it never inserts them. This keeps the
 * long-standing hidden-truth isolation invariant intact while allowing the
 * procedural expansion pipeline to be tested before any future mutation path.
 *
 * v21.1 hardening: proposal IDs use a wider deterministic digest, public output
 * redacts proposal counts, and proposal evaluation simulates insertion on a copy
 * of HiddenTruthGraph before accepting. The live archive state is never mutated.
 */
#include "impossible_archive.h"

namespace archive {

namespace {

[[nodiscard]] std::uint64_t hidden_proposal_hash(std::string_view text) {
    std::uint64_t value = 1469598103934665603ULL;
    for (const char ch : text) {
        value ^= static_cast<unsigned char>(ch);
        value *= 1099511628211ULL;
    }
    return value;
}

[[nodiscard]] std::string hex64(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
}

[[nodiscard]] std::string proposal_digest(const CandidateGenerationRequest& request,
                                          std::size_t index,
                                          std::string_view proposal_kind) {
    std::ostringstream material;
    material << request.target_topic << "|" << request.target_year << "|" << request.seed
             << "|" << static_cast<int>(request.strategy) << "|" << index << "|" << proposal_kind;
    constexpr std::uint64_t golden_ratio = static_cast<std::uint64_t>(0x9e3779b97f4a7c15ULL);
    constexpr std::uint64_t role_multiplier = static_cast<std::uint64_t>(0xbf58476d1ce4e5b9ULL);
    std::uint64_t value = hidden_proposal_hash(material.str());
    value ^= static_cast<std::uint64_t>(request.target_year) + golden_ratio + (value << 6U) + (value >> 2U);
    value ^= (static_cast<std::uint64_t>(index) + static_cast<std::uint64_t>(1U)) * role_multiplier;
    return hex64(value);
}

[[nodiscard]] std::string proposal_suffix(const CandidateGenerationRequest& request,
                                          std::size_t index,
                                          std::string_view proposal_kind) {
    return claim_suffix_for_id(request.target_topic) + "_" + std::to_string(request.target_year) + "_" +
           proposal_digest(request, index, proposal_kind);
}

void add_error(std::vector<std::string>& errors, std::string text) {
    errors.push_back(std::move(text));
}

[[nodiscard]] bool entity_available_for_event(const Entity& entity, const Event& event) {
    return entity.existence_interval.contains(event.start_year) && entity.existence_interval.contains(event.end_year);
}

void add_proposed_entity_provenance_errors(const HiddenTruthGraph& graph,
                                           const Entity& entity,
                                           std::vector<std::string>& errors) {
    if (!entity.created_by_event_id.empty()) {
        const Event* created_by = graph.find_event(entity.created_by_event_id);
        if (created_by == nullptr) {
            add_error(errors, "proposed entity references missing created_by_event_id " + entity.created_by_event_id);
        } else if (created_by->end_year > entity.existence_interval.start_year) {
            add_error(errors,
                      "proposed entity created_by_event_id " + entity.created_by_event_id +
                      " occurs after entity start year " + std::to_string(entity.existence_interval.start_year));
        }
    }

    if (!entity.ended_by_event_id.empty()) {
        const Event* ended_by = graph.find_event(entity.ended_by_event_id);
        if (ended_by == nullptr) {
            add_error(errors, "proposed entity references missing ended_by_event_id " + entity.ended_by_event_id);
        } else if (entity.existence_interval.end_year != kOpenEndedYear &&
                   ended_by->start_year < entity.existence_interval.end_year) {
            add_error(errors,
                      "proposed entity ended_by_event_id " + entity.ended_by_event_id +
                      " occurs before entity end year " + std::to_string(entity.existence_interval.end_year));
        }
    }
}

void add_simulated_graph_errors(const ArchiveEngineState& state,
                                const HiddenProposal& proposal,
                                std::vector<std::string>& errors) {
    const std::vector<std::string> baseline_errors = state.hidden_truth.validate();

    HiddenTruthGraph simulated = state.hidden_truth;
    try {
        if (proposal.type == HiddenProposalType::Entity && proposal.proposed_entity.has_value()) {
            simulated.add_entity(*proposal.proposed_entity);
        } else if (proposal.type == HiddenProposalType::Event && proposal.proposed_event.has_value()) {
            simulated.add_event(*proposal.proposed_event);
        } else {
            return;
        }
    } catch (const std::exception& ex) {
        add_error(errors, std::string("simulated hidden graph insertion failed: ") + ex.what());
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

[[nodiscard]] std::string proposed_year_text_for(const HiddenProposal& proposal) {
    if (proposal.proposed_event.has_value()) {
        return std::to_string(proposal.proposed_event->start_year);
    }
    if (proposal.proposed_entity.has_value()) {
        return std::to_string(proposal.proposed_entity->existence_interval.start_year);
    }
    return "unavailable";
}

} // namespace

[[nodiscard]] std::vector<HiddenProposal> generate_hidden_proposals(const ArchiveEngineState& state,
                                                                    const CandidateGenerationRequest& request) {
    const std::optional<GenerationTarget> target = resolve_generation_target(state, request.target_topic);
    if (!target.has_value()) {
        return {};
    }

    std::vector<HiddenProposal> proposals;

    if (target->topic == "lock_authority") {
        const std::string event_suffix = proposal_suffix(request, 0U, "event.lock_authority_hearing");
        proposals.push_back(HiddenProposal{
            "hidden_proposal.event.lock_authority_hearing." + event_suffix,
            HiddenProposalType::Event,
            target->topic,
            request.target_year,
            "Propose a narrow post-schism hearing that explains why later archive fragments disagree about who received lower lock authority.",
            std::nullopt,
            Event{
                "event.proposed.lock_authority_hearing." + event_suffix,
                "archive_hearing",
                "Lower Lock Authority Hearing",
                619,
                619,
                {"person.ivara"},
                {"event.salt_moon_schism"},
                {"site.reservoir_gate", "office.drowned_chancellor"},
                "A proposed hidden hearing after the Salt-Moon Schism would explain why later fragments mix office, keeper, and ritual authority language.",
                TruthLayer::CanonicalTruth,
                AccessLevel::Canon,
            },
            {"would justify future Drowned Chancellor corroborating fragments", "would increase lock-authority dossier density"},
        });

        const std::string entity_suffix = proposal_suffix(request, 1U, "entity.lock_witness_office");
        proposals.push_back(HiddenProposal{
            "hidden_proposal.entity.lock_witness_office." + entity_suffix,
            HiddenProposalType::Entity,
            target->topic,
            request.target_year,
            "Propose a minor witness office created after the schism, useful for later archive fragments without changing the main Drowned Chancellor chronology.",
            Entity{
                "office.proposed_lock_witness." + entity_suffix,
                EntityType::Office,
                "Lock Witness",
                {"Keeper-Witness of the Lower Locks"},
                Interval{619, 760},
                "event.salt_moon_schism",
                "",
                AccessLevel::Canon,
            },
            std::nullopt,
            {"would provide a lower-stakes office for future corroborating records", "would not resolve the Third Lock Authority mystery by itself"},
        });
    } else if (target->topic == "silt_levy") {
        const std::string suffix = proposal_suffix(request, 0U, "event.silt_levy_appeal");
        proposals.push_back(HiddenProposal{
            "hidden_proposal.event.silt_levy_appeal." + suffix,
            HiddenProposalType::Event,
            target->topic,
            request.target_year,
            "Propose a narrow lower-lock appeal after the silt levy, useful for generating additional ledger or petition fragments.",
            std::nullopt,
            Event{
                "event.proposed.silt_levy_appeal." + suffix,
                "petition",
                "Lower Lock Silt Levy Appeal",
                608,
                608,
                {"person.ivara"},
                {"event.canal_tax_reform"},
                {"site.reservoir_gate"},
                "A proposed appeal by lower-lock officials would create a hidden basis for later levy fragments without touching the lock-authority mystery.",
                TruthLayer::CanonicalTruth,
                AccessLevel::Canon,
            },
            {"would support future silt levy petitions", "should not create Third Lock Authority mystery evidence"},
        });
    } else if (target->topic == "drowned_chancellor") {
        const std::string suffix = proposal_suffix(request, 0U, "event.drowned_chancellor_oath");
        proposals.push_back(HiddenProposal{
            "hidden_proposal.event.drowned_chancellor_oath." + suffix,
            HiddenProposalType::Event,
            target->topic,
            request.target_year,
            "Propose a formal oath ceremony for the Drowned Chancellor after the office already exists.",
            std::nullopt,
            Event{
                "event.proposed.drowned_chancellor_oath." + suffix,
                "oath",
                "Drowned Chancellor Oath Ceremony",
                620,
                620,
                {"person.ivara"},
                {"event.salt_moon_schism"},
                {"office.drowned_chancellor", "script.green_seal", "site.reservoir_gate"},
                "A proposed oath ceremony would explain later formulaic Green-Seal office language without moving the office creation date.",
                TruthLayer::CanonicalTruth,
                AccessLevel::Canon,
            },
            {"would support future Green-Seal office formulae", "would not legitimize pre-schism Drowned Chancellor references"},
        });
    } else if (target->topic == "three_keepers") {
        const std::string suffix = proposal_suffix(request, 0U, "event.three_keepers_recitation");
        proposals.push_back(HiddenProposal{
            "hidden_proposal.event.three_keepers_recitation." + suffix,
            HiddenProposalType::Event,
            target->topic,
            request.target_year,
            "Propose an early ritual recitation that later oral history could compress into one moon judge.",
            std::nullopt,
            Event{
                "event.proposed.three_keepers_recitation." + suffix,
                "ritual_recitation",
                "First Three Keepers Recitation",
                621,
                621,
                {"person.ivara"},
                {"event.salt_moon_schism"},
                {"site.reservoir_gate", "office.drowned_chancellor"},
                "A proposed ritual recitation could provide a hidden basis for later mythic compression without making the oral song literal.",
                TruthLayer::MythicTruth,
                AccessLevel::Canon,
            },
            {"would deepen ritual-variant generation", "would preserve rather than resolve the Third Lock Authority mystery"},
        });
    }

    return proposals;
}

[[nodiscard]] HiddenProposalEvaluation evaluate_hidden_proposal(const ArchiveEngineState& state,
                                                               const HiddenProposal& proposal) {
    HiddenProposalEvaluation evaluation;
    evaluation.evaluated_proposal_id = proposal.id;
    evaluation.predicted_archive_implications = proposal.predicted_archive_implications;

    if (proposal.id.empty()) {
        add_error(evaluation.validation_errors, "proposal id is empty");
    }

    if (proposal.type == HiddenProposalType::Entity) {
        if (!proposal.proposed_entity.has_value()) {
            add_error(evaluation.validation_errors, "entity proposal has no proposed entity payload");
        } else {
            const Entity& entity = *proposal.proposed_entity;
            if (entity.id.empty()) {
                add_error(evaluation.validation_errors, "proposed entity id is empty");
            }
            if (state.hidden_truth.find_entity(entity.id) != nullptr) {
                add_error(evaluation.validation_errors, "proposed entity id already exists: " + entity.id);
            }
            if (entity.canonical_name.empty()) {
                add_error(evaluation.validation_errors, "proposed entity canonical_name is empty");
            }
            if (entity.existence_interval.end_year < entity.existence_interval.start_year) {
                add_error(evaluation.validation_errors, "proposed entity existence interval is invalid");
            }
            add_proposed_entity_provenance_errors(state.hidden_truth, entity, evaluation.validation_errors);
        }
    } else if (proposal.type == HiddenProposalType::Event) {
        if (!proposal.proposed_event.has_value()) {
            add_error(evaluation.validation_errors, "event proposal has no proposed event payload");
        } else {
            const Event& event = *proposal.proposed_event;
            if (event.id.empty()) {
                add_error(evaluation.validation_errors, "proposed event id is empty");
            }
            if (state.hidden_truth.find_event(event.id) != nullptr) {
                add_error(evaluation.validation_errors, "proposed event id already exists: " + event.id);
            }
            if (event.end_year < event.start_year) {
                add_error(evaluation.validation_errors, "proposed event ends before it starts");
            }
            for (const std::string& cause_id : event.cause_event_ids) {
                const Event* cause = state.hidden_truth.find_event(cause_id);
                if (cause == nullptr) {
                    if (cause_id != event.id) {
                        add_error(evaluation.validation_errors, "proposed event references missing cause " + cause_id);
                    }
                } else if (cause->end_year > event.start_year) {
                    add_error(evaluation.validation_errors, "proposed event occurs before cause " + cause_id + " ends");
                }
            }
            for (const std::string& participant_id : event.participant_entity_ids) {
                const Entity* participant = state.hidden_truth.find_entity(participant_id);
                if (participant == nullptr) {
                    add_error(evaluation.validation_errors, "proposed event references missing participant " + participant_id);
                } else if (!entity_available_for_event(*participant, event)) {
                    add_error(evaluation.validation_errors, "participant " + participant_id + " unavailable during proposed event");
                }
            }
            for (const std::string& required_id : event.required_entity_ids) {
                const Entity* required = state.hidden_truth.find_entity(required_id);
                if (required == nullptr) {
                    add_error(evaluation.validation_errors, "proposed event references missing required entity " + required_id);
                } else if (!entity_available_for_event(*required, event)) {
                    add_error(evaluation.validation_errors, "required entity " + required_id + " unavailable during proposed event");
                }
            }
        }
    } else {
        add_error(evaluation.validation_errors, "unknown hidden proposal type");
    }

    if (evaluation.validation_errors.empty()) {
        add_simulated_graph_errors(state, proposal, evaluation.validation_errors);
    }

    if (!evaluation.validation_errors.empty()) {
        evaluation.decision = HiddenProposalDecision::Reject;
        evaluation.explanation = "Hidden proposal failed chronology, provenance, or simulated graph validation; hidden truth was not mutated.";
    } else {
        evaluation.decision = HiddenProposalDecision::AcceptableProposal;
        evaluation.explanation = "Hidden proposal is structurally compatible with the current hidden chronology after simulated insertion, but remains advisory and was not inserted.";
    }
    return evaluation;
}

[[nodiscard]] std::string format_hidden_proposals(const ArchiveEngineState& state,
                                                  AccessLevel access,
                                                  const CandidateGenerationRequest& request) {
    std::ostringstream out;
    out << "Hidden proposal gate visible to " << to_string(access) << ":\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- requested_target_year: " << request.target_year << "\n";
    const std::string before = serialize_for_replay_test(state);
    const std::vector<HiddenProposal> proposals = generate_hidden_proposals(state, request);
    const std::string after = serialize_for_replay_test(state);
    if (can_view(access, AccessLevel::Curator)) {
        out << "- generated_count: " << proposals.size() << "\n";
    } else {
        out << "- generated_count: restricted\n";
    }
    out << "- archive_mutated: " << (before == after ? "false" : "true") << "\n";

    if (!can_view(access, AccessLevel::Curator)) {
        out << "- hidden proposal internals are restricted to curator/canon/debug access\n";
        return out.str();
    }

    if (proposals.empty()) {
        out << "- no hidden proposals generated for unresolved or unsupported target topic\n";
        return out.str();
    }

    for (std::size_t i = 0; i < proposals.size(); ++i) {
        const HiddenProposalEvaluation evaluation = evaluate_hidden_proposal(state, proposals[i]);
        out << "Proposal " << i << ":\n";
        out << "- id: " << proposals[i].id << "\n";
        out << "- type: " << to_string(proposals[i].type) << "\n";
        out << "- requested_target_year: " << proposals[i].requested_target_year << "\n";
        out << "- proposed_year: " << proposed_year_text_for(proposals[i]) << "\n";
        out << "- decision: " << to_string(evaluation.decision) << "\n";
        out << "- description: " << proposals[i].description << "\n";
        out << "- explanation: " << evaluation.explanation << "\n";
        if (!evaluation.predicted_archive_implications.empty()) {
            out << "Predicted archive implications:\n";
            for (const std::string& implication : evaluation.predicted_archive_implications) {
                out << "- " << implication << "\n";
            }
        }
    }
    return out.str();
}


[[nodiscard]] std::string hidden_proposal_proposed_year_text(const HiddenProposal& proposal) {
    return proposed_year_text_for(proposal);
}

[[nodiscard]] HiddenProposalMigrationPlan plan_hidden_proposal_migration(const ArchiveEngineState& state,
                                                                        const HiddenProposal& proposal) {
    HiddenProposalMigrationPlan plan;
    const HiddenProposalEvaluation evaluation = evaluate_hidden_proposal(state, proposal);
    plan.proposal_id = proposal.id;
    plan.proposal_decision = evaluation.decision;
    plan.predicted_public_archive_effects = evaluation.predicted_archive_implications;
    plan.validation_errors = evaluation.validation_errors;
    plan.would_mutate_hidden_truth = false;

    if (proposal.proposed_entity.has_value()) {
        plan.projected_entities.push_back(*proposal.proposed_entity);
    }
    if (proposal.proposed_event.has_value()) {
        plan.projected_events.push_back(*proposal.proposed_event);
    }

    const std::optional<GenerationTarget> target = resolve_generation_target(state, proposal.target_topic);
    if (target.has_value()) {
        if (target->mystery_id.has_value()) {
            plan.affected_mystery_ids.push_back(*target->mystery_id);
        }
        plan.affected_claim_ids = target->claim_ids;
    }

    if (evaluation.decision == HiddenProposalDecision::AcceptableProposal) {
        plan.predicted_public_archive_effects.push_back("would allow a future curator/canon/debug acceptance gate to insert projected hidden entities or events after validation");
    } else {
        plan.predicted_public_archive_effects.push_back("would not be eligible for hidden-truth insertion without resolving validation errors");
    }
    return plan;
}

[[nodiscard]] std::string format_hidden_proposal_migration_plan(const ArchiveEngineState& state,
                                                               AccessLevel access,
                                                               const CandidateGenerationRequest& request,
                                                               std::size_t proposal_index) {
    std::ostringstream out;
    out << "Hidden proposal migration plan visible to " << to_string(access) << ":\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- requested_target_year: " << request.target_year << "\n";
    out << "- proposal_index: " << proposal_index << "\n";

    const std::string before = serialize_for_replay_test(state);
    const std::vector<HiddenProposal> proposals = generate_hidden_proposals(state, request);
    const std::string after = serialize_for_replay_test(state);
    out << "- archive_mutated: " << (before == after ? "false" : "true") << "\n";
    out << "- would_mutate_hidden_truth: false\n";

    if (!can_view(access, AccessLevel::Curator)) {
        out << "- generated_count: restricted\n";
        out << "- hidden proposal migration internals are restricted to curator/canon/debug access\n";
        return out.str();
    }

    out << "- generated_count: " << proposals.size() << "\n";
    if (proposal_index >= proposals.size()) {
        out << "- decision: Reject\n";
        out << "- explanation: hidden proposal index is out of range; no migration plan was applied.\n";
        return out.str();
    }

    const HiddenProposal& proposal = proposals[proposal_index];
    const HiddenProposalMigrationPlan plan = plan_hidden_proposal_migration(state, proposal);
    out << "- proposal_id: " << plan.proposal_id << "\n";
    out << "- proposal_type: " << to_string(proposal.type) << "\n";
    out << "- requested_target_year: " << proposal.requested_target_year << "\n";
    out << "- proposed_year: " << hidden_proposal_proposed_year_text(proposal) << "\n";
    out << "- proposal_decision: " << to_string(plan.proposal_decision) << "\n";

    if (!plan.projected_entities.empty()) {
        out << "Projected entities:\n";
        for (const Entity& entity : plan.projected_entities) {
            out << "- " << entity.id << " [" << to_string(entity.type) << ", "
                << interval_text(entity.existence_interval) << "]: " << entity.canonical_name << "\n";
        }
    }
    if (!plan.projected_events.empty()) {
        out << "Projected events:\n";
        for (const Event& event : plan.projected_events) {
            out << "- " << event.id << " (" << event.start_year;
            if (event.end_year != event.start_year) {
                out << "-" << event.end_year;
            }
            out << "): " << event.title << "\n";
        }
    }
    if (!plan.affected_mystery_ids.empty()) {
        out << "Affected mysteries:\n";
        for (const std::string& mystery_id : plan.affected_mystery_ids) {
            out << "- " << mystery_id << "\n";
        }
    }
    if (!plan.affected_claim_ids.empty()) {
        out << "Affected claims:\n";
        for (const std::string& claim_id : plan.affected_claim_ids) {
            out << "- " << claim_id << "\n";
        }
    }
    if (!plan.predicted_public_archive_effects.empty()) {
        out << "Predicted public archive effects:\n";
        for (const std::string& effect : plan.predicted_public_archive_effects) {
            out << "- " << effect << "\n";
        }
    }
    if (!plan.validation_errors.empty()) {
        out << "Validation risks:\n";
        for (const std::string& error : plan.validation_errors) {
            out << "- " << error << "\n";
        }
    }
    return out.str();
}

[[nodiscard]] std::string format_hidden_proposal_evaluation(const ArchiveEngineState& state,
                                                           AccessLevel access,
                                                           const CandidateGenerationRequest& request,
                                                           std::size_t proposal_index) {
    std::ostringstream out;
    out << "Hidden proposal evaluation visible to " << to_string(access) << ":\n";
    out << "- target_topic: " << request.target_topic << "\n";
    out << "- requested_target_year: " << request.target_year << "\n";
    out << "- proposal_index: " << proposal_index << "\n";

    const std::string before = serialize_for_replay_test(state);
    const std::vector<HiddenProposal> proposals = generate_hidden_proposals(state, request);
    const std::string after = serialize_for_replay_test(state);
    out << "- archive_mutated: " << (before == after ? "false" : "true") << "\n";

    if (!can_view(access, AccessLevel::Curator)) {
        out << "- generated_count: restricted\n";
        out << "- hidden proposal internals are restricted to curator/canon/debug access\n";
        return out.str();
    }

    out << "- generated_count: " << proposals.size() << "\n";
    if (proposal_index >= proposals.size()) {
        out << "- decision: Reject\n";
        out << "- explanation: hidden proposal index is out of range; hidden truth was not mutated.\n";
        return out.str();
    }

    const HiddenProposal& proposal = proposals[proposal_index];
    const HiddenProposalEvaluation evaluation = evaluate_hidden_proposal(state, proposal);
    out << "- proposal_id: " << proposal.id << "\n";
    out << "- proposal_type: " << to_string(proposal.type) << "\n";
    out << "- requested_target_year: " << proposal.requested_target_year << "\n";
    out << "- proposed_year: " << proposed_year_text_for(proposal) << "\n";
    out << "- decision: " << to_string(evaluation.decision) << "\n";
    out << "- explanation: " << evaluation.explanation << "\n";
    if (!evaluation.validation_errors.empty()) {
        out << "Validation errors:\n";
        for (const std::string& error : evaluation.validation_errors) {
            out << "- " << error << "\n";
        }
    }
    if (!evaluation.predicted_archive_implications.empty()) {
        out << "Predicted archive implications:\n";
        for (const std::string& implication : evaluation.predicted_archive_implications) {
            out << "- " << implication << "\n";
        }
    }
    return out.str();
}

} // namespace archive
