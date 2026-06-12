/*
 * Discovery views and interpreter/theory scoring. Evidence must be filtered by access and archive year before theories are built.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

[[nodiscard]] std::vector<Interpreter> default_interpreters() {
    return {
        Interpreter{"interpreter.bureaucratic_minimalist", "Tala of the Counting Reeds", EpistemicStyle::BureaucraticMinimalist, AccessLevel::Scholar},
        Interpreter{"interpreter.ritual_formalist", "Saren of the Moon-Lock School", EpistemicStyle::RitualFormalist, AccessLevel::Scholar},
        Interpreter{"interpreter.anti_dynastic_revisionist", "Veyr the Anti-Dynastic Reader", EpistemicStyle::AntiDynasticRevisionist, AccessLevel::Scholar},
    };
}

[[nodiscard]] bool claim_is_in_contradiction(const Claim& claim, const std::vector<const Contradiction*>& contradictions) {
    return std::any_of(contradictions.begin(), contradictions.end(), [&](const Contradiction* contradiction) {
        return contradiction != nullptr &&
               std::find(contradiction->involved_claim_ids.begin(), contradiction->involved_claim_ids.end(), claim.id) != contradiction->involved_claim_ids.end();
    });
}

[[nodiscard]] bool claim_has_visible_forgery_caveat(const Claim& claim,
                                                            const std::vector<const Contradiction*>& contradictions,
                                                            AccessLevel access) {
    return std::any_of(contradictions.begin(), contradictions.end(), [&](const Contradiction* contradiction) {
        return contradiction != nullptr &&
               contradiction->assigned_cause == ContradictionCause::Forgery &&
               can_view(access, contradiction->cause_min_access) &&
               std::find(contradiction->involved_claim_ids.begin(), contradiction->involved_claim_ids.end(), claim.id) != contradiction->involved_claim_ids.end();
    });
}

[[nodiscard]] bool artifact_type_visible_as(const Artifact& artifact, ArtifactType expected, AccessLevel access) {
    if (artifact.type != expected) {
        return false;
    }
    if (expected == ArtifactType::ForgedDecree) {
        return can_view(access, AccessLevel::Curator);
    }
    return true;
}

[[nodiscard]] bool artifact_is_visible_disputed_decree(const Artifact& artifact, AccessLevel access) {
    return artifact.type == ArtifactType::ForgedDecree && !can_view(access, AccessLevel::Curator);
}

[[nodiscard]] double epistemic_style_multiplier(EpistemicStyle style,
                                                const Claim& claim,
                                                const Artifact& artifact,
                                                const std::vector<const Contradiction*>& contradictions,
                                                AccessLevel access) {
    double multiplier = 1.0;

    switch (style) {
        case EpistemicStyle::BureaucraticMinimalist:
            if (artifact_type_visible_as(artifact, ArtifactType::TradeLedger, access)) multiplier *= 1.35;
            if (claim.type == ClaimType::FactualClaim) multiplier *= 1.15;
            if (has_visible_evidence_modifier(artifact, EvidenceModifier::NarrowScope, access)) multiplier *= 1.15;
            if (artifact_type_visible_as(artifact, ArtifactType::OralHistory, access)) multiplier *= 0.50;
            if (claim.type == ClaimType::MythicCompression) multiplier *= 0.50;
            if (claim.type == ClaimType::PropagandaClaim) multiplier *= 0.75;
            break;

        case EpistemicStyle::RitualFormalist:
            if (artifact_type_visible_as(artifact, ArtifactType::OralHistory, access)) multiplier *= 1.45;
            if (claim.type == ClaimType::LegalFiction) multiplier *= 1.15;
            if (claim.type == ClaimType::MythicCompression) multiplier *= 1.60;
            if (has_any_visible_evidence_modifier(artifact, {EvidenceModifier::RitualAnachronism, EvidenceModifier::MythicCompression}, access)) multiplier *= 1.25;
            if (artifact_type_visible_as(artifact, ArtifactType::TradeLedger, access) &&
                has_visible_evidence_modifier(artifact, EvidenceModifier::NarrowScope, access)) {
                multiplier *= 0.45;
            }
            break;

        case EpistemicStyle::AntiDynasticRevisionist:
            if (artifact_type_visible_as(artifact, ArtifactType::Inscription, access)) multiplier *= 1.20;
            if (artifact_type_visible_as(artifact, ArtifactType::ForgedDecree, access)) multiplier *= 4.00;
            if (artifact_is_visible_disputed_decree(artifact, access)) multiplier *= 1.20;
            if (claim.type == ClaimType::PropagandaClaim) multiplier *= 1.25;
            if (claim.type == ClaimType::LegalFiction) multiplier *= 1.45;
            if (has_visible_evidence_modifier(artifact, EvidenceModifier::Propaganda, access)) multiplier *= 1.30;
            if (has_visible_evidence_modifier(artifact, EvidenceModifier::Forgery, access)) multiplier *= 2.00;
            if (claim_has_visible_forgery_caveat(claim, contradictions, access)) multiplier *= 1.45;
            if (artifact_type_visible_as(artifact, ArtifactType::TradeLedger, access)) multiplier *= 0.75;
            break;
    }

    if (claim_is_in_contradiction(claim, contradictions)) {
        if (style == EpistemicStyle::BureaucraticMinimalist) {
            multiplier *= 0.90;
        } else if (style == EpistemicStyle::RitualFormalist) {
            multiplier *= 1.05;
        } else if (style == EpistemicStyle::AntiDynasticRevisionist) {
            multiplier *= 1.15;
        }
    }

    return multiplier;
}

[[nodiscard]] std::vector<ScoredClaim> score_claims_for_interpreter(const ArchiveEngineState& state,
                                                                    const Interpreter& interpreter,
                                                                    const std::vector<const Claim*>& claims,
                                                                    const std::vector<const Contradiction*>& contradictions) {
    std::vector<ScoredClaim> scored;
    for (const Claim* claim : claims) {
        if (claim == nullptr) {
            continue;
        }
        const Artifact* artifact = state.public_archive.find_artifact(claim->source_artifact_id);
        if (artifact == nullptr || !can_view(interpreter.access, artifact->min_access)) {
            continue;
        }
        const double base = citation_weight(state, *claim);
        const double multiplier = epistemic_style_multiplier(interpreter.style, *claim, *artifact, contradictions, interpreter.access);
        scored.push_back(ScoredClaim{claim, clamp01(base * multiplier)});
    }

    std::sort(scored.begin(), scored.end(), [](const ScoredClaim& lhs, const ScoredClaim& rhs) {
        if (lhs.score != rhs.score) {
            return lhs.score > rhs.score;
        }
        const std::string left_id = lhs.claim == nullptr ? std::string{} : lhs.claim->id;
        const std::string right_id = rhs.claim == nullptr ? std::string{} : rhs.claim->id;
        return left_id < right_id;
    });
    return scored;
}

[[nodiscard]] std::string top_evidence_phrase(const ArchiveEngineState& state,
                                                const std::vector<ScoredClaim>& selected) {
    if (selected.empty() || selected.front().claim == nullptr) {
        return "No visible evidence is strong enough for this interpreter to cite.";
    }

    const Claim* top = selected.front().claim;
    const Artifact* artifact = state.public_archive.find_artifact(top->source_artifact_id);
    std::ostringstream out;
    out << "The strongest selected evidence is " << top->id;
    if (artifact != nullptr) {
        out << " from " << artifact->title;
    }
    out << ", scored at " << std::fixed << std::setprecision(2) << selected.front().score << ".";

    if (selected.size() > 1 && selected[1].claim != nullptr) {
        out << " It is paired with " << selected[1].claim->id << " as secondary evidence.";
    }
    return out.str();
}

[[nodiscard]] bool selected_contains_claim(const std::vector<ScoredClaim>& selected, std::string_view claim_id) {
    return std::any_of(selected.begin(), selected.end(), [&](const ScoredClaim& item) {
        return item.claim != nullptr && item.claim->id == claim_id;
    });
}

[[nodiscard]] bool selected_has_visible_forgery_caveat(const std::vector<ScoredClaim>& selected,
                                                       const std::vector<const Contradiction*>& contradictions,
                                                       AccessLevel access) {
    return std::any_of(selected.begin(), selected.end(), [&](const ScoredClaim& item) {
        return item.claim != nullptr && claim_has_visible_forgery_caveat(*item.claim, contradictions, access);
    });
}

[[nodiscard]] std::string theory_summary_for_style(EpistemicStyle style,
                                                   const std::vector<ScoredClaim>& selected,
                                                   const std::vector<const Contradiction*>& contradictions,
                                                   const ArchiveEngineState& state,
                                                   AccessLevel access) {
    std::ostringstream summary;
    switch (style) {
        case EpistemicStyle::BureaucraticMinimalist:
            summary << "This interpretation privileges administrative records and narrow accounting claims over royal spectacle or ritual memory.";
            if (selected_contains_claim(selected, "claim.levy_exists_607")) {
                summary << " It centers the silt ledger because that visible claim currently carries the strongest administrative weight.";
            }
            break;
        case EpistemicStyle::RitualFormalist:
            summary << "This interpretation treats ritual and legal formulae as meaningful even when they are not literal personnel records.";
            if (selected_contains_claim(selected, "claim.three_as_one")) {
                summary << " It gives the Three Keepers song interpretive force as mythic or ritual evidence.";
            }
            if (selected_contains_claim(selected, "claim.aru_created_office")) {
                summary << " It also treats office-language in the Aru material as evidence of ritualized authority, not as a clean factual record.";
            }
            break;
        case EpistemicStyle::AntiDynasticRevisionist:
            summary << "This interpretation reads dynastic and restoration claims as interested political evidence rather than neutral chronicle.";
            if (selected_has_visible_forgery_caveat(selected, contradictions, access)) {
                summary << " Because this interpreter can see the restricted forgery cause, the Aru decree receives additional suspicious weight.";
            } else if (selected_contains_claim(selected, "claim.aru_created_office")) {
                summary << " The Aru decree is treated as disputed public evidence without assuming curator-only forgery facts.";
            }
            break;
    }

    summary << " " << top_evidence_phrase(state, selected);
    return summary.str();
}

[[nodiscard]] Theory build_theory_for_interpreter(const ArchiveEngineState& state, const Interpreter& interpreter, int archive_year) {
    const std::vector<const Claim*> claims = visible_claims(state, interpreter.access, archive_year);
    const std::vector<const Contradiction*> contradictions = visible_contradictions(state, interpreter.access, archive_year);
    const std::vector<ScoredClaim> scored = score_claims_for_interpreter(state, interpreter, claims, contradictions);

    Theory theory;
    theory.id = "theory." + interpreter.id;
    theory.interpreter_id = interpreter.id;
    theory.interpreter_name = interpreter.name;
    theory.style = interpreter.style;
    theory.min_access = interpreter.access;

    constexpr std::size_t kMaxEvidence = 3;
    std::vector<ScoredClaim> selected_scored;
    double total = 0.0;
    for (const ScoredClaim& item : scored) {
        if (item.claim == nullptr || theory.supporting_evidence.size() >= kMaxEvidence) {
            break;
        }
        theory.supporting_evidence.push_back(EvidenceCitation{item.claim->source_artifact_id, item.claim->id, item.score});
        selected_scored.push_back(item);
        total += item.score;
    }

    for (const Contradiction* contradiction : contradictions) {
        if (contradiction == nullptr) {
            continue;
        }
        if (contradiction_relevant_to_citations(*contradiction, theory.supporting_evidence)) {
            theory.contradiction_ids.push_back(contradiction->id);
        }
    }

    theory.confidence = theory.supporting_evidence.empty() ? 0.0 : clamp01(total / static_cast<double>(theory.supporting_evidence.size()));
    theory.summary = theory_summary_for_style(interpreter.style, selected_scored, contradictions, state, interpreter.access);
    return theory;
}

[[nodiscard]] std::vector<Theory> build_theories(const ArchiveEngineState& state, int archive_year) {
    std::vector<Theory> theories;
    for (const Interpreter& interpreter : default_interpreters()) {
        theories.push_back(build_theory_for_interpreter(state, interpreter, archive_year));
    }
    return theories;
}

[[nodiscard]] std::string format_theory(const Theory& theory, const ArchiveEngineState& state, AccessLevel viewer_access, int archive_year) {
    std::ostringstream out;
    out << theory.interpreter_name << " [" << to_string(theory.style) << "]\n";
    out << "Theory ID: " << theory.id << "\n";
    out << "Interpretive confidence: " << std::fixed << std::setprecision(2) << theory.confidence
        << " (" << interpretive_confidence_label(theory.confidence) << ")\n";
    out << theory.summary << "\n";

    if (!theory.supporting_evidence.empty()) {
        out << "Evidence:\n";
        for (const EvidenceCitation& citation : theory.supporting_evidence) {
            const std::optional<std::string> rendered = format_citation_for_access(citation, state, viewer_access, archive_year);
            if (rendered.has_value()) {
                out << "- " << *rendered << "\n";
            }
        }
    }

    if (!theory.contradiction_ids.empty()) {
        out << "Caveats:\n";
        for (const std::string& contradiction_id : theory.contradiction_ids) {
            const Contradiction* contradiction = state.public_archive.find_contradiction(contradiction_id);
            if (contradiction == nullptr || !contradiction_visible_to(state, *contradiction, viewer_access, archive_year)) {
                continue;
            }
            out << "- " << contradiction->id << ": " << contradiction->public_resolution_status;
            if (can_view(viewer_access, contradiction->cause_min_access)) {
                out << " (assigned cause: " << to_string(contradiction->assigned_cause) << ")";
            }
            if (can_view(viewer_access, contradiction->hidden_resolution_min_access)) {
                out << " [hidden resolution: " << contradiction->hidden_truth_resolution << "]";
            }
            out << "\n";
        }
    }
    return out.str();
}

[[nodiscard]] std::string format_theories(const ArchiveEngineState& state, AccessLevel viewer_access, int archive_year) {
    std::ostringstream out;
    out << "Interpreter theories visible to " << to_string(viewer_access) << " at archive year " << archive_year_text(archive_year) << ":\n";
    bool any_visible = false;
    for (const Theory& theory : build_theories(state, archive_year)) {
        if (!can_view(viewer_access, theory.min_access)) {
            continue;
        }
        any_visible = true;
        out << "\n" << format_theory(theory, state, viewer_access, archive_year);
    }
    if (!any_visible) {
        out << "- no interpreter theories visible at this access level\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_discoveries(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::ostringstream out;
    out << "Discoveries visible to " << to_string(access) << " at archive year " << archive_year_text(archive_year) << ":\n";
    bool any = false;
    for (const Discovery& discovery : state.discovery_log) {
        if (discovery.discovery_year > archive_year || !can_view(access, discovery.min_access)) {
            continue;
        }
        const Artifact* artifact = state.public_archive.find_artifact(discovery.artifact_id);
        if (artifact == nullptr || !artifact_visible_to(*artifact, access, archive_year)) {
            continue;
        }
        any = true;
        out << "- " << discovery.discovery_year << " " << discovery.id
            << " -> " << discovery.artifact_id
            << " at " << discovery.site_id
            << " (" << artifact->title << ")\n";
    }
    if (!any) {
        out << "- no discoveries visible at this access level and archive year\n";
    }
    return out.str();
}

[[nodiscard]] std::string format_hidden_timeline(const ArchiveEngineState& state, AccessLevel access) {
    std::ostringstream out;
    if (!can_view(access, AccessLevel::Canon)) {
        out << "Hidden timeline requires canon or debug access.\n";
        return out.str();
    }

    std::vector<const Event*> timeline;
    for (const auto& [event_id, event] : state.hidden_truth.events()) {
        (void)event_id;
        timeline.push_back(&event);
    }
    std::sort(timeline.begin(), timeline.end(), [](const Event* lhs, const Event* rhs) {
        if (lhs->start_year != rhs->start_year) {
            return lhs->start_year < rhs->start_year;
        }
        if (lhs->end_year != rhs->end_year) {
            return lhs->end_year < rhs->end_year;
        }
        return lhs->id < rhs->id;
    });

    out << "Hidden canonical timeline:\n";
    for (const Event* event : timeline) {
        out << "- " << event->start_year;
        if (event->end_year != event->start_year) {
            out << "-" << event->end_year;
        }
        out << " " << event->title << ": " << event->canonical_description << "\n";
    }
    return out.str();
}

} // namespace archive
