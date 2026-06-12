#include "evidence_potential_api.h"

#include <cctype>
#include <map>
#include <set>
#include <sstream>

namespace archive {

namespace {

[[nodiscard]] std::string lower_copy(std::string_view text) {
    std::string lowered;
    lowered.reserve(text.size());
    for (char ch : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return lowered;
}

[[nodiscard]] bool contains_any(std::string_view text, std::initializer_list<std::string_view> needles) {
    const std::string lowered = lower_copy(text);
    return std::any_of(needles.begin(), needles.end(), [&](std::string_view needle) {
        return lowered.find(std::string(needle)) != std::string::npos;
    });
}

[[nodiscard]] std::string event_search_text(const Event& event) {
    return event.event_type + " " + event.title + " " + event.canonical_description;
}

[[nodiscard]] AccessLevel potential_access_for_source(AccessLevel source_access, bool public_safe) {
    if (public_safe) {
        return AccessLevel::Public;
    }
    return can_view(AccessLevel::Public, source_access) ? AccessLevel::Public : AccessLevel::Curator;
}

void add_unique_evidence_modifier(std::vector<EvidenceModifier>& values, EvidenceModifier modifier) {
    if (std::find(values.begin(), values.end(), modifier) == values.end()) {
        values.push_back(modifier);
    }
}

struct PotentialDraft {
    std::string sort_key;
    EvidencePotential potential;
};

[[nodiscard]] std::vector<std::string> event_site_ids(const ArchiveEngineState& state, const Event& event) {
    std::vector<std::string> ids;
    for (const std::string& participant_id : event.participant_entity_ids) {
        const Entity* entity = state.hidden_truth.find_entity(participant_id);
        if (entity != nullptr && (entity->type == EntityType::Site || entity->type == EntityType::Settlement)) {
            add_unique_string(ids, participant_id);
        }
    }
    return ids;
}

[[nodiscard]] std::vector<std::string> event_creator_ids(const ArchiveEngineState& state, const Event& event) {
    std::vector<std::string> ids;
    for (const std::string& participant_id : event.participant_entity_ids) {
        const Entity* entity = state.hidden_truth.find_entity(participant_id);
        if (entity != nullptr &&
            (entity->type == EntityType::Person || entity->type == EntityType::Office || entity->type == EntityType::Faction)) {
            add_unique_string(ids, participant_id);
        }
    }
    return ids;
}

[[nodiscard]] EvidencePotential make_event_potential(const ArchiveEngineState& state,
                                                      const Event& event,
                                                      EvidencePotentialTraceType trace_type,
                                                      ArtifactType artifact_type,
                                                      std::string rationale,
                                                      EvidencePotentialStrength strength) {
    EvidencePotential potential;
    potential.source_type = EvidencePotentialSourceType::HiddenEvent;
    potential.source_id = event.id;
    potential.trace_type = trace_type;
    potential.likely_artifact_type = artifact_type;
    potential.subject = event.title;
    potential.rationale = std::move(rationale);
    potential.earliest_possible_year = event.start_year;
    potential.latest_possible_year = std::max(event.end_year, event.start_year);
    potential.likely_site_ids = event_site_ids(state, event);
    potential.likely_creator_ids = event_creator_ids(state, event);
    potential.public_safe = can_view(AccessLevel::Public, event.min_access);
    potential.min_access = potential_access_for_source(event.min_access, potential.public_safe);
    potential.strength = strength;
    return potential;
}

[[nodiscard]] bool source_is_hidden_only(const ArchiveEngineState& state, const EvidencePotential& potential) {
    if (potential.source_type == EvidencePotentialSourceType::HiddenEvent) {
        const Event* event = state.hidden_truth.find_event(potential.source_id);
        return event != nullptr && !can_view(AccessLevel::Public, event->min_access);
    }
    if (potential.source_type == EvidencePotentialSourceType::HiddenEntity ||
        potential.source_type == EvidencePotentialSourceType::Site ||
        potential.source_type == EvidencePotentialSourceType::Office) {
        const Entity* entity = state.hidden_truth.find_entity(potential.source_id);
        return entity != nullptr && !can_view(AccessLevel::Public, entity->min_access);
    }
    if (potential.source_type == EvidencePotentialSourceType::Mystery) {
        const auto it = std::find_if(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
            return mystery.id == potential.source_id;
        });
        return it != state.mysteries.end() && !can_view(AccessLevel::Public, it->min_access);
    }
    return false;
}

[[nodiscard]] bool source_exists(const ArchiveEngineState& state, const EvidencePotential& potential) {
    switch (potential.source_type) {
        case EvidencePotentialSourceType::HiddenEvent:
            return state.hidden_truth.find_event(potential.source_id) != nullptr;
        case EvidencePotentialSourceType::HiddenEntity:
        case EvidencePotentialSourceType::Site:
        case EvidencePotentialSourceType::Office:
            return state.hidden_truth.find_entity(potential.source_id) != nullptr;
        case EvidencePotentialSourceType::Mystery:
            return std::any_of(state.mysteries.begin(), state.mysteries.end(), [&](const Mystery& mystery) {
                return mystery.id == potential.source_id;
            });
    }
    return false;
}

[[nodiscard]] bool potential_visible_to(const EvidencePotential& potential, AccessLevel access) {
    return can_view(access, potential.min_access) && (potential.public_safe || can_view(access, AccessLevel::Curator));
}

void format_public_potential_line(std::ostringstream& out, const EvidencePotential& potential) {
    out << "  - " << potential.id << ": " << to_string(potential.trace_type)
        << " -> " << to_string(potential.likely_artifact_type)
        << "; years " << year_text(potential.earliest_possible_year)
        << "-" << year_text(potential.latest_possible_year)
        << "; strength=" << to_string(potential.strength)
        << "; discoverable=" << (potential.discoverable ? "true" : "false") << "\n";
}

void format_restricted_potential_line(std::ostringstream& out, const EvidencePotential& potential) {
    format_public_potential_line(out, potential);
    out << "    source_type: " << to_string(potential.source_type) << "\n";
    out << "    source_id: " << potential.source_id << "\n";
    out << "    subject: " << potential.subject << "\n";
    out << "    rationale: " << potential.rationale << "\n";
    if (!potential.likely_site_ids.empty()) {
        out << "    likely_site_ids:";
        for (const std::string& id : potential.likely_site_ids) {
            out << " " << id;
        }
        out << "\n";
    }
    if (!potential.likely_creator_ids.empty()) {
        out << "    likely_creator_ids:";
        for (const std::string& id : potential.likely_creator_ids) {
            out << " " << id;
        }
        out << "\n";
    }
    if (!potential.likely_distortions.empty()) {
        out << "    likely_distortions:";
        for (EvidenceModifier modifier : potential.likely_distortions) {
            out << " " << to_string(modifier);
        }
        out << "\n";
    }
}

} // namespace

[[nodiscard]] std::string to_string(EvidencePotentialSourceType type) {
    switch (type) {
        case EvidencePotentialSourceType::HiddenEvent: return "hidden_event";
        case EvidencePotentialSourceType::HiddenEntity: return "hidden_entity";
        case EvidencePotentialSourceType::Site: return "site";
        case EvidencePotentialSourceType::Office: return "office";
        case EvidencePotentialSourceType::Mystery: return "mystery";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(EvidencePotentialTraceType type) {
    switch (type) {
        case EvidencePotentialTraceType::InscriptionTrace: return "inscription_trace";
        case EvidencePotentialTraceType::LedgerTrace: return "ledger_trace";
        case EvidencePotentialTraceType::OralTraditionTrace: return "oral_tradition_trace";
        case EvidencePotentialTraceType::RitualTrace: return "ritual_trace";
        case EvidencePotentialTraceType::LegalRecordTrace: return "legal_record_trace";
        case EvidencePotentialTraceType::MaterialDepositTrace: return "material_deposit_trace";
        case EvidencePotentialTraceType::CopyTraditionTrace: return "copy_tradition_trace";
        case EvidencePotentialTraceType::AbsenceTrace: return "absence_trace";
    }
    return "unknown";
}

[[nodiscard]] std::string to_string(EvidencePotentialStrength strength) {
    switch (strength) {
        case EvidencePotentialStrength::Weak: return "weak";
        case EvidencePotentialStrength::Moderate: return "moderate";
        case EvidencePotentialStrength::Strong: return "strong";
    }
    return "unknown";
}

[[nodiscard]] std::vector<EvidencePotential> derive_evidence_potentials(const ArchiveEngineState& state) {
    std::vector<PotentialDraft> drafts;

    for (const auto& [event_id, event] : state.hidden_truth.events()) {
        const std::string text = event_search_text(event);
        if (contains_any(text, {"admin", "legal", "tax", "ledger", "record", "office", "court", "toll", "grant", "quota", "authority", "institution"})) {
            EvidencePotential potential = make_event_potential(
                state,
                event,
                contains_any(text, {"law", "legal", "court", "decree", "grant"}) ? EvidencePotentialTraceType::LegalRecordTrace : EvidencePotentialTraceType::LedgerTrace,
                contains_any(text, {"law", "legal", "court", "decree", "grant"}) ? ArtifactType::DamagedManuscript : ArtifactType::TradeLedger,
                "Administrative, legal, or authority events plausibly leave ledger, docket, or legal-copy traces without creating artifacts in v28.2.",
                EvidencePotentialStrength::Strong
            );
            add_unique_evidence_modifier(potential.likely_distortions, EvidenceModifier::NarrowScope);
            if (contains_any(text, {"copy", "decree", "grant"})) {
                add_unique_evidence_modifier(potential.likely_distortions, EvidenceModifier::LaterCopy);
            }
            drafts.push_back(PotentialDraft{"event:" + event_id + ":admin", std::move(potential)});
        }

        if (contains_any(text, {"ritual", "shrine", "oracle", "cult", "priest", "temple", "oath", "ancestor", "myth", "codification"})) {
            EvidencePotential potential = make_event_potential(
                state,
                event,
                EvidencePotentialTraceType::RitualTrace,
                ArtifactType::OralHistory,
                "Ritual or shrine-context events plausibly leave formulaic ritual, oral, or inscriptional traces without scheduling discovery.",
                EvidencePotentialStrength::Moderate
            );
            add_unique_evidence_modifier(potential.likely_distortions, EvidenceModifier::MythicCompression);
            add_unique_evidence_modifier(potential.likely_distortions, EvidenceModifier::RitualAnachronism);
            drafts.push_back(PotentialDraft{"event:" + event_id + ":ritual", std::move(potential)});
        }

        if (contains_any(text, {"disaster", "loss", "destroy", "destruction", "collapse", "fire", "flood", "drought", "abandon", "silt", "storm", "failure", "damage", "lost"})) {
            EvidencePotential potential = make_event_potential(
                state,
                event,
                contains_any(text, {"lost", "loss", "destroy", "destruction"}) ? EvidencePotentialTraceType::AbsenceTrace : EvidencePotentialTraceType::MaterialDepositTrace,
                ArtifactType::DamagedManuscript,
                "Disaster, loss, or destruction events plausibly leave deposits, silences, damage horizons, or absence traces without materializing evidence.",
                EvidencePotentialStrength::Moderate
            );
            add_unique_evidence_modifier(potential.likely_distortions, EvidenceModifier::Damage);
            add_unique_evidence_modifier(potential.likely_distortions, EvidenceModifier::MisdatedStratum);
            drafts.push_back(PotentialDraft{"event:" + event_id + ":loss", std::move(potential)});
        }
    }

    for (const auto& [entity_id, entity] : state.hidden_truth.entities()) {
        if (entity.type == EntityType::Site || entity.type == EntityType::Settlement) {
            EvidencePotential potential;
            potential.source_type = EvidencePotentialSourceType::Site;
            potential.source_id = entity_id;
            potential.trace_type = EvidencePotentialTraceType::MaterialDepositTrace;
            potential.likely_artifact_type = ArtifactType::Inscription;
            potential.subject = entity.canonical_name;
            potential.rationale = "Site-like hidden entities plausibly preserve inscriptions, built deposits, or displaced material contexts without generating artifacts.";
            potential.earliest_possible_year = entity.existence_interval.start_year;
            potential.latest_possible_year = entity.existence_interval.end_year;
            potential.likely_site_ids = {entity_id};
            potential.likely_distortions = {EvidenceModifier::Damage, EvidenceModifier::MisdatedStratum};
            potential.strength = EvidencePotentialStrength::Moderate;
            potential.public_safe = can_view(AccessLevel::Public, entity.min_access);
            potential.min_access = potential_access_for_source(entity.min_access, potential.public_safe);
            drafts.push_back(PotentialDraft{"entity:" + entity_id + ":site", std::move(potential)});
        } else if (entity.type == EntityType::Office) {
            EvidencePotential potential;
            potential.source_type = EvidencePotentialSourceType::Office;
            potential.source_id = entity_id;
            potential.trace_type = EvidencePotentialTraceType::LegalRecordTrace;
            potential.likely_artifact_type = ArtifactType::TradeLedger;
            potential.subject = entity.canonical_name;
            potential.rationale = "Office entities plausibly leave appointment lists, rosters, seals, or legal copies without creating any public claim in v28.2.";
            potential.earliest_possible_year = entity.existence_interval.start_year;
            potential.latest_possible_year = entity.existence_interval.end_year;
            potential.likely_creator_ids = {entity_id};
            potential.likely_distortions = {EvidenceModifier::NarrowScope, EvidenceModifier::LaterCopy};
            potential.strength = EvidencePotentialStrength::Strong;
            potential.public_safe = can_view(AccessLevel::Public, entity.min_access);
            potential.min_access = potential_access_for_source(entity.min_access, potential.public_safe);
            drafts.push_back(PotentialDraft{"entity:" + entity_id + ":office", std::move(potential)});
        }
    }

    for (const Mystery& mystery : state.mysteries) {
        if (!mystery.clue_artifact_ids.empty() || !mystery.misleading_artifact_ids.empty() || !mystery.evidence_links.empty()) {
            EvidencePotential potential;
            potential.source_type = EvidencePotentialSourceType::Mystery;
            potential.source_id = mystery.id;
            potential.trace_type = mystery.misleading_artifact_ids.empty() ? EvidencePotentialTraceType::CopyTraditionTrace : EvidencePotentialTraceType::AbsenceTrace;
            potential.likely_artifact_type = ArtifactType::DamagedManuscript;
            potential.subject = mystery.title;
            potential.rationale = "Mysteries with clue or misleading artifacts imply possible copy-tradition gaps, absences, or later interpretive traces without resolving the mystery.";
            potential.earliest_possible_year = 0;
            potential.latest_possible_year = kOpenEndedYear;
            potential.likely_distortions = {EvidenceModifier::LaterCopy, EvidenceModifier::Mistranslation};
            potential.strength = EvidencePotentialStrength::Weak;
            potential.public_safe = can_view(AccessLevel::Public, mystery.min_access);
            potential.min_access = potential_access_for_source(mystery.min_access, potential.public_safe);
            drafts.push_back(PotentialDraft{"mystery:" + mystery.id + ":copy", std::move(potential)});
        }
    }

    std::sort(drafts.begin(), drafts.end(), [](const PotentialDraft& lhs, const PotentialDraft& rhs) {
        return lhs.sort_key < rhs.sort_key;
    });

    std::vector<EvidencePotential> potentials;
    potentials.reserve(drafts.size());
    for (std::size_t index = 0; index < drafts.size(); ++index) {
        std::ostringstream id;
        id << "evidence_potential." << std::setfill('0') << std::setw(4) << index;
        drafts[index].potential.id = id.str();
        potentials.push_back(std::move(drafts[index].potential));
    }
    return potentials;
}

void derive_evidence_potentials_into_state(ArchiveEngineState& state) {
    state.evidence_potentials = derive_evidence_potentials(state);
}

[[nodiscard]] std::vector<std::string> validate_evidence_potentials(const ArchiveEngineState& state) {
    std::vector<std::string> errors;
    std::set<std::string> seen_ids;

    for (const EvidencePotential& potential : state.evidence_potentials) {
        const std::string label = potential.id.empty() ? std::string{"<empty evidence potential id>"} : potential.id;
        if (potential.id.empty()) {
            errors.push_back("evidence potential has empty id");
        } else if (!seen_ids.insert(potential.id).second) {
            errors.push_back("duplicate evidence potential id: " + potential.id);
        }
        if (potential.source_id.empty()) {
            errors.push_back("evidence potential " + label + " has empty source_id");
        } else if (has_prefix(potential.source_id, "fragment.") || has_prefix(potential.source_id, "civilization_fragment.")) {
            errors.push_back("evidence potential " + label + " is fragment-derived; fragments remain inert in v28.2");
        } else if (!source_exists(state, potential)) {
            errors.push_back("evidence potential " + label + " references missing source " + potential.source_id);
        }
        if (potential.latest_possible_year < potential.earliest_possible_year) {
            errors.push_back("evidence potential " + label + " has invalid year range");
        }
        if (potential.rationale.empty()) {
            errors.push_back("evidence potential " + label + " has empty rationale");
        }
        for (const std::string& site_id : potential.likely_site_ids) {
            const Entity* site = state.hidden_truth.find_entity(site_id);
            if (site == nullptr) {
                errors.push_back("evidence potential " + label + " references unknown likely site " + site_id);
            } else if (site->type != EntityType::Site && site->type != EntityType::Settlement) {
                errors.push_back("evidence potential " + label + " likely site " + site_id + " is not site-like");
            }
        }
        for (const std::string& creator_id : potential.likely_creator_ids) {
            if (state.hidden_truth.find_entity(creator_id) == nullptr) {
                errors.push_back("evidence potential " + label + " references unknown likely creator " + creator_id);
            }
        }
        if (potential.public_safe && source_is_hidden_only(state, potential)) {
            errors.push_back("evidence potential " + label + " is public_safe but source is hidden-only");
        }
    }

    return errors;
}

[[nodiscard]] std::string format_evidence_potential_summary(const ArchiveEngineState& state, AccessLevel access) {
    std::map<std::string, std::size_t> by_trace;
    std::size_t visible = 0;
    std::size_t public_safe = 0;
    std::size_t discoverable = 0;
    for (const EvidencePotential& potential : state.evidence_potentials) {
        ++by_trace[to_string(potential.trace_type)];
        if (potential_visible_to(potential, access)) {
            ++visible;
        }
        if (potential.public_safe) {
            ++public_safe;
        }
        if (potential.discoverable) {
            ++discoverable;
        }
    }

    std::ostringstream out;
    out << "EvidencePotential summary:\n";
    out << "- total: " << state.evidence_potentials.size() << "\n";
    out << "- visible_to_" << to_string(access) << ": " << visible << "\n";
    out << "- public_safe: " << public_safe << "\n";
    out << "- discoverable: " << discoverable << "\n";
    out << "- behavior: inert model only; no artifacts, discoveries, candidates, or mutations are generated from EvidencePotential in v28.2.\n";
    out << "Trace counts:\n";
    for (const auto& [trace, count] : by_trace) {
        out << "- " << trace << ": " << count << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_evidence_potential_list(const ArchiveEngineState& state, AccessLevel access) {
    std::ostringstream out;
    out << "EvidencePotentials visible to " << to_string(access) << ":\n";
    std::size_t visible = 0;
    for (const EvidencePotential& potential : state.evidence_potentials) {
        if (!potential_visible_to(potential, access)) {
            continue;
        }
        ++visible;
        if (can_view(access, AccessLevel::Curator)) {
            format_restricted_potential_line(out, potential);
        } else {
            format_public_potential_line(out, potential);
        }
    }
    if (visible == 0U) {
        out << "- none visible at this access level; use curator/debug access to inspect hidden-source potentials.\n";
    }
    out << "- total_modeled: " << state.evidence_potentials.size() << "\n";
    return out.str();
}

[[nodiscard]] std::string format_evidence_potential_detail(const ArchiveEngineState& state,
                                                            AccessLevel access,
                                                            const std::string& evidence_potential_id) {
    const auto it = std::find_if(state.evidence_potentials.begin(), state.evidence_potentials.end(), [&](const EvidencePotential& potential) {
        return potential.id == evidence_potential_id;
    });
    std::ostringstream out;
    out << "EvidencePotential detail:\n";
    if (it == state.evidence_potentials.end()) {
        out << "- found: false\n";
        out << "- evidence_potential_id: " << evidence_potential_id << "\n";
        return out.str();
    }
    out << "- found: true\n";
    if (!potential_visible_to(*it, access)) {
        out << "- visible: false\n";
        out << "- explanation: this potential is restricted because its source remains hidden-only.\n";
        return out.str();
    }
    out << "- visible: true\n";
    if (can_view(access, AccessLevel::Curator)) {
        format_restricted_potential_line(out, *it);
    } else {
        format_public_potential_line(out, *it);
    }
    return out.str();
}

[[nodiscard]] std::string format_evidence_potential_validation(const ArchiveEngineState& state, AccessLevel access) {
    const std::vector<std::string> errors = validate_evidence_potentials(state);
    std::ostringstream out;
    out << "EvidencePotential validation:\n";
    out << "- result: " << (errors.empty() ? "passed" : "failed") << "\n";
    out << "- checked: " << state.evidence_potentials.size() << "\n";
    out << "- errors: " << errors.size() << "\n";
    if (!errors.empty()) {
        if (can_view(access, AccessLevel::Curator)) {
            for (const std::string& error : errors) {
                out << "  - " << error << "\n";
            }
        } else {
            out << "- details: restricted\n";
        }
    }
    return out.str();
}

} // namespace archive
