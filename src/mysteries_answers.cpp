/*
 * Mystery assessment and evidence-driven answers. Mystery confidence caps and access filters must be applied before formatting.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

[[nodiscard]] std::vector<const Artifact*> visible_artifacts(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::vector<const Artifact*> result;
    for (const auto& [artifact_id, artifact] : state.public_archive.artifacts()) {
        (void)artifact_id;
        if (artifact_visible_to(artifact, access, archive_year)) {
            result.push_back(&artifact);
        }
    }
    return result;
}

[[nodiscard]] std::vector<const Claim*> visible_claims(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::vector<const Claim*> result;
    for (const auto& [claim_id, claim] : state.public_archive.claims()) {
        (void)claim_id;
        if (claim_visible_to(state, claim, access, archive_year)) {
            result.push_back(&claim);
        }
    }
    return result;
}

[[nodiscard]] std::vector<const Contradiction*> visible_contradictions(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::vector<const Contradiction*> result;
    for (const auto& [contradiction_id, contradiction] : state.public_archive.contradictions()) {
        (void)contradiction_id;
        if (contradiction_visible_to(state, contradiction, access, archive_year)) {
            result.push_back(&contradiction);
        }
    }
    return result;
}

[[nodiscard]] double citation_weight(const ArchiveEngineState& state, const Claim& claim);

[[nodiscard]] std::optional<std::string> format_citation_for_access(const EvidenceCitation& citation,
                                                                    const ArchiveEngineState& state,
                                                                    AccessLevel access,
                                                                    int archive_year);

[[nodiscard]] bool mystery_has_visible_evidence_link(const ArchiveEngineState& state,
                                                            const Mystery& mystery,
                                                            AccessLevel access,
                                                            int archive_year) {
    for (const MysteryEvidenceLink& link : mystery.evidence_links) {
        const Artifact* artifact = state.public_archive.find_artifact(link.artifact_id);
        if (artifact == nullptr || !artifact_visible_to(*artifact, access, archive_year)) {
            continue;
        }
        if (!link.claim_id.has_value()) {
            return true;
        }
        const Claim* claim = state.public_archive.find_claim(*link.claim_id);
        if (claim != nullptr && claim_visible_to(state, *claim, access, archive_year) && claim->source_artifact_id == link.artifact_id) {
            return true;
        }
        // Context evidence can make a mystery visible when the clue artifact is visible,
        // even if the direct claim is restricted; the assessment will say direct evidence
        // is not yet visible.
        if (link.role == MysteryEvidenceRole::ContextClue) {
            return true;
        }
    }

    return std::any_of(mystery.clue_artifact_ids.begin(), mystery.clue_artifact_ids.end(),
        [&](const std::string& artifact_id) {
            const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
            return artifact != nullptr && artifact_visible_to(*artifact, access, archive_year);
        });
}

[[nodiscard]] std::vector<const Mystery*> visible_mysteries(const ArchiveEngineState& state,
                                                            AccessLevel access,
                                                            int archive_year) {
    std::vector<const Mystery*> result;
    for (const Mystery& mystery : state.mysteries) {
        if (!can_view(access, mystery.min_access)) {
            continue;
        }
        if (mystery_has_visible_evidence_link(state, mystery, access, archive_year)) {
            result.push_back(&mystery);
        }
    }
    return result;
}

[[nodiscard]] double confidence_cap_for_mystery(const Mystery& mystery, AccessLevel access) {
    double cap = can_view(access, AccessLevel::Scholar) ? mystery.max_scholar_confidence : mystery.max_public_confidence;
    if (mystery.reveal_mode == RevealMode::NeverFullyResolvable) {
        cap = std::min(cap, 0.95);
    }
    if (mystery.reveal_mode == RevealMode::ResolvedOnlyInMythicTerms && !can_view(access, AccessLevel::Canon)) {
        cap = std::min(cap, 0.80);
    }
    if (mystery.reveal_mode == RevealMode::AccessLocked && !can_view(access, AccessLevel::Canon)) {
        cap = std::min(cap, 0.50);
    }
    return clamp01(cap);
}

[[nodiscard]] std::string mystery_status_for_confidence(double confidence, const Mystery& mystery) {
    if (mystery.reveal_mode == RevealMode::NeverFullyResolvable) {
        return "protected unresolved";
    }
    if (mystery.reveal_mode == RevealMode::ContradictoryByDesign) {
        return "contradictory by design";
    }
    if (confidence >= 0.80) {
        return "strongly constrained";
    }
    if (confidence >= 0.55) {
        return "partially resolved";
    }
    if (confidence >= 0.25) {
        return "open but constrained";
    }
    return "unresolved";
}

[[nodiscard]] bool contradiction_relevant_to_citations(const Contradiction& contradiction,
                                                       const std::vector<EvidenceCitation>& citations);

[[nodiscard]] std::vector<const Claim*> visible_claims_for_artifact(const ArchiveEngineState& state,
                                                                    const Artifact& artifact,
                                                                    AccessLevel access,
                                                                    int archive_year) {
    std::vector<const Claim*> result;
    for (const std::string& claim_id : artifact.claim_ids) {
        const Claim* claim = state.public_archive.find_claim(claim_id);
        if (claim != nullptr && claim_visible_to(state, *claim, access, archive_year)) {
            result.push_back(claim);
        }
    }
    return result;
}

void add_unique_citation(std::vector<EvidenceCitation>& citations, EvidenceCitation citation) {
    const auto exists = std::any_of(citations.begin(), citations.end(), [&](const EvidenceCitation& existing) {
        return existing.artifact_id == citation.artifact_id && existing.claim_id == citation.claim_id;
    });
    if (!exists) {
        citations.push_back(std::move(citation));
    }
}

[[nodiscard]] bool link_visible_to(const ArchiveEngineState& state,
                                   const MysteryEvidenceLink& link,
                                   AccessLevel access,
                                   int archive_year) {
    const Artifact* artifact = state.public_archive.find_artifact(link.artifact_id);
    if (artifact == nullptr || !artifact_visible_to(*artifact, access, archive_year)) {
        return false;
    }
    if (!link.claim_id.has_value()) {
        return true;
    }
    const Claim* claim = state.public_archive.find_claim(*link.claim_id);
    return claim != nullptr && claim->source_artifact_id == link.artifact_id && claim_visible_to(state, *claim, access, archive_year);
}

[[nodiscard]] MysteryAssessment assess_mystery(const ArchiveEngineState& state,
                                               const Mystery& mystery,
                                               AccessLevel access,
                                               int archive_year) {
    MysteryAssessment assessment;
    assessment.mystery_id = mystery.id;
    assessment.title = mystery.title;
    assessment.confidence_cap = confidence_cap_for_mystery(mystery, access);

    double raw = 0.35;
    int visible_artifact_only_clues = 0;

    for (const MysteryEvidenceLink& link : mystery.evidence_links) {
        const Artifact* artifact = state.public_archive.find_artifact(link.artifact_id);
        if (artifact == nullptr || !artifact_visible_to(*artifact, access, archive_year)) {
            continue;
        }
        add_unique_string(assessment.visible_clue_artifact_ids, link.artifact_id);

        const Claim* claim = nullptr;
        if (link.claim_id.has_value()) {
            claim = state.public_archive.find_claim(*link.claim_id);
            if (claim == nullptr || claim->source_artifact_id != link.artifact_id || !claim_visible_to(state, *claim, access, archive_year)) {
                if (link.role == MysteryEvidenceRole::ContextClue) {
                    ++assessment.visible_context_clues;
                    ++visible_artifact_only_clues;
                    raw += 0.03 * link.weight_multiplier;
                }
                continue;
            }
        }

        const double weight = claim == nullptr ? clamp01(artifact->reliability_score * link.weight_multiplier)
                                               : clamp01(citation_weight(state, *claim) * link.weight_multiplier);
        EvidenceCitation citation{link.artifact_id, link.claim_id, weight};

        switch (link.role) {
            case MysteryEvidenceRole::CoreClue:
                ++assessment.visible_core_clues;
                add_unique_citation(assessment.supporting_evidence, std::move(citation));
                raw += 0.32 * link.weight_multiplier + 0.15 * weight;
                break;
            case MysteryEvidenceRole::ContextClue:
                ++assessment.visible_context_clues;
                add_unique_citation(assessment.context_evidence, std::move(citation));
                raw += 0.05 * link.weight_multiplier + 0.02 * weight;
                break;
            case MysteryEvidenceRole::MisleadingClue:
                ++assessment.visible_misleading_clues;
                add_unique_string(assessment.visible_misleading_artifact_ids, link.artifact_id);
                add_unique_citation(assessment.misleading_evidence, std::move(citation));
                raw += 0.16 * link.weight_multiplier + 0.04 * weight;
                raw -= 0.10;
                break;
            case MysteryEvidenceRole::FalseResolution:
                ++assessment.visible_misleading_clues;
                add_unique_string(assessment.visible_misleading_artifact_ids, link.artifact_id);
                add_unique_citation(assessment.misleading_evidence, std::move(citation));
                raw += can_view(access, AccessLevel::Curator) ? -0.10 : 0.08 * link.weight_multiplier;
                break;
        }
    }

    // Compatibility fallback for older mysteries that still specify artifact IDs
    // without explicit evidence links. New mysteries should use evidence_links.
    if (mystery.evidence_links.empty()) {
        for (const std::string& artifact_id : mystery.clue_artifact_ids) {
            const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
            if (artifact == nullptr || !artifact_visible_to(*artifact, access, archive_year)) {
                continue;
            }
            add_unique_string(assessment.visible_clue_artifact_ids, artifact_id);
            ++visible_artifact_only_clues;
            raw += 0.10;
        }
    }

    const std::vector<const Contradiction*> contradictions = visible_contradictions(state, access, archive_year);
    std::vector<EvidenceCitation> linked_citations = assessment.supporting_evidence;
    linked_citations.insert(linked_citations.end(), assessment.context_evidence.begin(), assessment.context_evidence.end());
    linked_citations.insert(linked_citations.end(), assessment.misleading_evidence.begin(), assessment.misleading_evidence.end());

    for (const Contradiction* contradiction : contradictions) {
        if (contradiction == nullptr) {
            continue;
        }
        if (contradiction_relevant_to_citations(*contradiction, linked_citations)) {
            add_unique_string(assessment.contradiction_ids, contradiction->id);
            raw += 0.04;
        }
    }

    if (assessment.visible_clue_artifact_ids.empty()) {
        raw = 0.0;
    }

    assessment.raw_confidence = clamp01(raw);
    assessment.capped_confidence = std::min(assessment.raw_confidence, assessment.confidence_cap);
    assessment.status = mystery_status_for_confidence(assessment.capped_confidence, mystery);

    std::ostringstream summary;
    summary << mystery.protected_question << " ";
    if (assessment.visible_clue_artifact_ids.empty()) {
        summary << "No clue artifacts are visible at this archive year.";
    } else {
        summary << "Visible linked evidence: core=" << assessment.visible_core_clues
                << ", context=" << assessment.visible_context_clues
                << ", misleading=" << assessment.visible_misleading_clues
                << "; reveal_mode=" << to_string(mystery.reveal_mode) << ".";
        if (assessment.visible_core_clues == 0 && (assessment.visible_context_clues > 0 || visible_artifact_only_clues > 0)) {
            summary << " A clue artifact is visible, but no directly relevant claim is visible at this access level.";
        }
        if (!assessment.visible_misleading_artifact_ids.empty()) {
            summary << " Misleading evidence is visible and reduces confidence rather than resolving the question.";
        }
        if (assessment.raw_confidence > assessment.capped_confidence) {
            summary << " Confidence is capped to preserve the mystery from over-resolution.";
        }
    }
    assessment.summary = summary.str();
    return assessment;
}

[[nodiscard]] std::string format_mystery_assessment(const MysteryAssessment& assessment,
                                                    const ArchiveEngineState& state,
                                                    AccessLevel access,
                                                    int archive_year) {
    std::ostringstream out;
    out << assessment.title << " [" << assessment.mystery_id << "]\n";
    out << "Status: " << assessment.status << "; confidence=" << std::fixed << std::setprecision(2)
        << assessment.capped_confidence << " (raw=" << assessment.raw_confidence
        << ", cap=" << assessment.confidence_cap << ")\n";
    out << assessment.summary << "\n";

    if (!assessment.supporting_evidence.empty()) {
        out << "Visible core mystery evidence:\n";
        for (const EvidenceCitation& citation : assessment.supporting_evidence) {
            const std::optional<std::string> rendered = format_citation_for_access(citation, state, access, archive_year);
            if (rendered.has_value()) {
                out << "- " << *rendered << "\n";
            }
        }
    }

    if (!assessment.context_evidence.empty()) {
        out << "Visible contextual mystery evidence:\n";
        for (const EvidenceCitation& citation : assessment.context_evidence) {
            const std::optional<std::string> rendered = format_citation_for_access(citation, state, access, archive_year);
            if (rendered.has_value()) {
                out << "- " << *rendered << "\n";
            }
        }
    }

    if (!assessment.misleading_evidence.empty()) {
        out << "Visible misleading mystery evidence:\n";
        for (const EvidenceCitation& citation : assessment.misleading_evidence) {
            const std::optional<std::string> rendered = format_citation_for_access(citation, state, access, archive_year);
            if (rendered.has_value()) {
                out << "- " << *rendered << "\n";
            }
        }
    }

    if (!assessment.contradiction_ids.empty()) {
        out << "Mystery caveats:\n";
        for (const std::string& contradiction_id : assessment.contradiction_ids) {
            const Contradiction* contradiction = state.public_archive.find_contradiction(contradiction_id);
            if (contradiction == nullptr || !contradiction_visible_to(state, *contradiction, access, archive_year)) {
                continue;
            }
            out << "- " << contradiction->id << ": " << contradiction->public_resolution_status;
            if (can_view(access, contradiction->cause_min_access)) {
                out << " (assigned cause: " << to_string(contradiction->assigned_cause) << ")";
            }
            if (can_view(access, contradiction->hidden_resolution_min_access)) {
                out << " [hidden resolution: " << contradiction->hidden_truth_resolution << "]";
            }
            out << "\n";
        }
    }
    return out.str();
}

[[nodiscard]] std::string format_mysteries(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::ostringstream out;
    out << "Mysteries visible to " << to_string(access) << " at archive year " << archive_year_text(archive_year) << ":\n";
    const std::vector<const Mystery*> mysteries = visible_mysteries(state, access, archive_year);
    if (mysteries.empty()) {
        out << "- no mysteries visible at this access level and archive year\n";
        return out.str();
    }
    for (const Mystery* mystery : mysteries) {
        if (mystery == nullptr) {
            continue;
        }
        out << "\n" << format_mystery_assessment(assess_mystery(state, *mystery, access, archive_year), state, access, archive_year);
    }
    return out.str();
}

[[nodiscard]] const Claim* find_visible_claim_by_id(const std::vector<const Claim*>& claims, std::string_view id) {
    const auto it = std::find_if(claims.begin(), claims.end(), [&](const Claim* claim) {
        return claim != nullptr && claim->id == id;
    });
    return it == claims.end() ? nullptr : *it;
}

[[nodiscard]] std::vector<const Claim*> claims_by_predicate(const std::vector<const Claim*>& claims, PredicateType predicate) {
    std::vector<const Claim*> result;
    for (const Claim* claim : claims) {
        if (claim != nullptr && claim->semantics.has_value() && claim->semantics->predicate_type == predicate) {
            result.push_back(claim);
        }
    }
    return result;
}

[[nodiscard]] std::vector<const Claim*> high_confidence_claims(const std::vector<const Claim*>& claims, double threshold) {
    std::vector<const Claim*> result;
    for (const Claim* claim : claims) {
        if (claim != nullptr && claim->confidence >= threshold) {
            result.push_back(claim);
        }
    }
    return result;
}

[[nodiscard]] double citation_weight(const ArchiveEngineState& state, const Claim& claim) {
    const Artifact* source = state.public_archive.find_artifact(claim.source_artifact_id);
    if (source == nullptr) {
        return claim.confidence;
    }
    return clamp01(claim.confidence * source->reliability_score);
}

[[nodiscard]] std::string evidence_strength_text(double weight) {
    if (weight >= 0.60) {
        return "strongly supports";
    }
    if (weight >= 0.35) {
        return "supports";
    }
    if (weight >= 0.15) {
        return "weakly supports";
    }
    return "barely supports";
}

[[nodiscard]] std::string interpretive_confidence_label(double confidence) {
    if (confidence >= 0.70) {
        return "strong";
    }
    if (confidence >= 0.45) {
        return "moderate";
    }
    if (confidence >= 0.20) {
        return "tentative";
    }
    return "weak";
}

[[nodiscard]] std::optional<std::string> format_citation_for_access(const EvidenceCitation& citation,
                                                                          const ArchiveEngineState& state,
                                                                          AccessLevel access,
                                                                          int archive_year) {
    const Artifact* artifact = state.public_archive.find_artifact(citation.artifact_id);
    if (artifact == nullptr || !artifact_visible_to(*artifact, access, archive_year)) {
        return std::nullopt;
    }

    std::ostringstream out;
    if (citation.claim_id.has_value()) {
        const Claim* claim = state.public_archive.find_claim(*citation.claim_id);
        if (claim == nullptr || !can_view(access, claim->min_access) || claim->source_artifact_id != citation.artifact_id) {
            return std::nullopt;
        }
        const Artifact* claim_source = state.public_archive.find_artifact(claim->source_artifact_id);
        if (claim_source == nullptr || !artifact_visible_to(*claim_source, access, archive_year)) {
            return std::nullopt;
        }

        out << *citation.claim_id << " from " << citation.artifact_id
            << " [" << to_string(claim->type) << ", confidence=" << std::fixed << std::setprecision(2) << claim->confidence << "]";
    } else {
        out << citation.artifact_id;
    }

    out << " — " << artifact->title;
    out << " (weight=" << std::fixed << std::setprecision(2) << citation.weight << ")";
    return out.str();
}

[[nodiscard]] std::vector<const Event*> hidden_events_sorted_by_year(const HiddenTruthGraph& graph) {
    std::vector<const Event*> timeline;
    for (const auto& [event_id, event] : graph.events()) {
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
    return timeline;
}

[[nodiscard]] bool contains_rule_contradiction(const std::vector<const Contradiction*>& contradictions, std::string_view rule_name) {
    return std::any_of(contradictions.begin(), contradictions.end(), [&](const Contradiction* contradiction) {
        return contradiction != nullptr && contradiction->detector_rule == rule_name;
    });
}

void add_claim_citation_if_present(AnswerBlock& block, const ArchiveEngineState& state, const Claim* claim) {
    if (claim == nullptr) {
        return;
    }
    block.supporting_evidence.push_back(EvidenceCitation{
        claim->source_artifact_id,
        claim->id,
        citation_weight(state, *claim),
    });
}

[[nodiscard]] bool contradiction_relevant_to_citations(const Contradiction& contradiction,
                                                       const std::vector<EvidenceCitation>& citations) {
    for (const EvidenceCitation& citation : citations) {
        if (citation.claim_id.has_value() &&
            std::find(contradiction.involved_claim_ids.begin(), contradiction.involved_claim_ids.end(), *citation.claim_id) != contradiction.involved_claim_ids.end()) {
            return true;
        }
        if (std::find(contradiction.involved_artifact_ids.begin(), contradiction.involved_artifact_ids.end(), citation.artifact_id) != contradiction.involved_artifact_ids.end()) {
            return true;
        }
    }
    return false;
}

void add_relevant_contradictions_to_block(AnswerBlock& block, const std::vector<const Contradiction*>& contradictions) {
    for (const Contradiction* contradiction : contradictions) {
        if (contradiction == nullptr || !contradiction_relevant_to_citations(*contradiction, block.supporting_evidence)) {
            continue;
        }
        if (std::find(block.contradiction_ids.begin(), block.contradiction_ids.end(), contradiction->id) == block.contradiction_ids.end()) {
            block.contradiction_ids.push_back(contradiction->id);
        }
    }
}

[[nodiscard]] std::vector<const Claim*> materialized_candidate_claims_for_answer(const std::vector<const Claim*>& claims) {
    std::vector<const Claim*> result;
    for (const Claim* claim : claims) {
        if (claim == nullptr) {
            continue;
        }
        const bool materialized = has_prefix(claim->id, "claim.materialized_candidate_generated") ||
                                  has_prefix(claim->id, "claim.materialized_candidate_structured") ||
                                  has_prefix(claim->id, "claim.materialized_candidate_target_dossier") ||
                                  claim->id == "claim.materialized_candidate_structured_lock_fragment";
        if (!materialized || !claim->semantics.has_value()) {
            continue;
        }
        switch (claim->semantics->predicate_type) {
            case PredicateType::ExistedInYear:
            case PredicateType::Received:
            case PredicateType::Preceded:
            case PredicateType::Became:
            case PredicateType::LocatedAt:
                result.push_back(claim);
                break;
            case PredicateType::Restored:
            case PredicateType::Caused:
            case PredicateType::Appointed:
            case PredicateType::CreatedOffice:
            case PredicateType::UsesScript:
                break;
        }
    }
    std::sort(result.begin(), result.end(), [&](const Claim* lhs, const Claim* rhs) {
        if (lhs == nullptr || rhs == nullptr) {
            return lhs != nullptr;
        }
        return lhs->id < rhs->id;
    });
    if (result.size() > 2U) {
        result.resize(2U);
    }
    return result;
}

[[nodiscard]] AnswerBlock build_public_answer_block(const ArchiveEngineState& state,
                                                    const std::vector<const Claim*>& claims,
                                                    const std::vector<const Contradiction*>& contradictions) {
    const Claim* levy = find_visible_claim_by_id(claims, "claim.levy_exists_607");
    const Claim* levy_before_revolt = find_visible_claim_by_id(claims, "claim.levy_before_revolt");
    const Claim* restoration = find_visible_claim_by_id(claims, "claim.victory_restoration");
    const Claim* three_as_one = find_visible_claim_by_id(claims, "claim.three_as_one");
    const Claim* aru_decree = find_visible_claim_by_id(claims, "claim.aru_created_office");

    AnswerBlock block;
    block.heading = "Public answer";
    block.min_access = AccessLevel::Public;

    std::ostringstream summary;
    if (state.civilization_source.has_value() && claims.empty()) {
        summary << "The selected CivilizationSpec runtime has a validated bootstrapped hidden state for "
                << state.civilization_source->display_name
                << ", but no public artifact claims have been generated or materialized yet.";
    } else if (levy != nullptr && levy_before_revolt != nullptr) {
        summary << "The visible archive supports a cautious sequence: the silt ledger "
                << evidence_strength_text(citation_weight(state, *levy))
                << " a levy in 607, and a damaged chronicle places unrest after that levy.";
    } else if (levy != nullptr) {
        summary << "The visible archive " << evidence_strength_text(citation_weight(state, *levy))
                << " the existence of a silt levy in 607, but the visible claim set is not enough to confirm the revolt sequence.";
    } else if (levy_before_revolt != nullptr) {
        summary << "The visible archive preserves a damaged sequence claim that unrest followed a levy, but no visible ledger claim currently confirms the 607 levy.";
    } else {
        summary << "The visible archive does not currently contain enough visible claim evidence to confirm a levy-before-revolt sequence.";
    }

    if (restoration != nullptr) {
        summary << " Ivara's restoration claim is visible, but its source is propagandistic and should not be treated as neutral chronicle evidence.";
    }
    if (three_as_one != nullptr) {
        summary << " The Three Keepers song preserves ritualized or mythically compressed memory rather than a literal personnel record.";
    }
    if (aru_decree != nullptr) {
        summary << " The Aru decree is part of the visible dispute, but public evidence supports only an authenticity caveat, not the internal catalog verdict.";
    }

    const std::vector<const Claim*> materialized_claims = materialized_candidate_claims_for_answer(claims);
    if (!materialized_claims.empty()) {
        summary << " Additional materialized candidate evidence is now visible and is cited as provisional archive evidence.";
    }

    const bool has_aru_caveat = contains_rule_contradiction(contradictions, "unavailable_entity");
    const bool has_mythic_caveat = contains_rule_contradiction(contradictions, "mythic_identity_compression");
    const bool has_calendar_caveat = contains_rule_contradiction(contradictions, "calendar_date_disagreement");
    if (has_aru_caveat || has_mythic_caveat || has_calendar_caveat) {
        summary << " Visible contradictions require caution.";
    }

    block.summary = summary.str();
    add_claim_citation_if_present(block, state, levy);
    add_claim_citation_if_present(block, state, levy_before_revolt);
    add_claim_citation_if_present(block, state, restoration);
    add_claim_citation_if_present(block, state, three_as_one);
    add_claim_citation_if_present(block, state, aru_decree);
    for (const Claim* materialized_claim : materialized_claims) {
        add_claim_citation_if_present(block, state, materialized_claim);
    }

    add_relevant_contradictions_to_block(block, contradictions);
    return block;
}

[[nodiscard]] std::optional<AnswerBlock> build_scholar_answer_block(const ArchiveEngineState& state,
                                                                     const std::vector<const Claim*>& claims,
                                                                     const std::vector<const Contradiction*>& contradictions) {
    const Claim* moon_office = find_visible_claim_by_id(claims, "claim.moon_office_locks");
    const Claim* ledger = find_visible_claim_by_id(claims, "claim.levy_exists_607");
    if (moon_office == nullptr && ledger == nullptr) {
        return std::nullopt;
    }

    AnswerBlock block;
    block.heading = "Scholar evidence assessment";
    block.min_access = AccessLevel::Scholar;

    std::ostringstream summary;
    if (ledger != nullptr) {
        summary << "The ledger has the strongest visible evidentiary weight among public claims and is narrow rather than narratively expansive.";
    }
    if (moon_office != nullptr) {
        if (!summary.str().empty()) {
            summary << " ";
        }
        summary << "The Moon-office translation is visible at scholar level, but its confidence is limited by damage and later-copy transmission.";
    }
    if (contains_rule_contradiction(contradictions, "calendar_date_disagreement")) {
        summary << " The dry-count numbering caveat is present in this deterministic seed and should be kept separate from the basic levy-before-revolt sequence.";
    }
    block.summary = summary.str();
    add_claim_citation_if_present(block, state, ledger);
    add_claim_citation_if_present(block, state, moon_office);
    for (const Contradiction* contradiction : contradictions) {
        if (contradiction != nullptr && contradiction->detector_rule == "calendar_date_disagreement") {
            block.contradiction_ids.push_back(contradiction->id);
        }
    }
    return block;
}

[[nodiscard]] std::optional<AnswerBlock> build_curator_forgery_block(const ArchiveEngineState& state,
                                                                      const std::vector<const Claim*>& claims,
                                                                      const std::vector<const Contradiction*>& contradictions) {
    const Claim* aru = find_visible_claim_by_id(claims, "claim.aru_created_office");
    const Contradiction* forgery_contradiction = nullptr;
    for (const Contradiction* contradiction : contradictions) {
        if (contradiction != nullptr &&
            contradiction->assigned_cause == ContradictionCause::Forgery &&
            can_view(AccessLevel::Curator, contradiction->cause_min_access)) {
            forgery_contradiction = contradiction;
            break;
        }
    }
    if (aru == nullptr && forgery_contradiction == nullptr) {
        return std::nullopt;
    }

    AnswerBlock block;
    block.heading = "Curator restricted assessment";
    block.min_access = AccessLevel::Curator;
    block.summary = "The Aru decree should be treated as a likely partisan forgery: its claimed date conflicts with the known chronological range of the Drowned Chancellor office and Green-Seal script.";
    add_claim_citation_if_present(block, state, aru);
    if (forgery_contradiction != nullptr) {
        block.contradiction_ids.push_back(forgery_contradiction->id);
    }
    return block;
}

[[nodiscard]] std::optional<AnswerBlock> build_canon_hidden_sequence_block(const ArchiveEngineState& state) {
    const std::vector<const Event*> events = hidden_events_sorted_by_year(state.hidden_truth);
    if (events.empty()) {
        return std::nullopt;
    }

    AnswerBlock block;
    block.heading = "Canon hidden sequence";
    block.min_access = AccessLevel::Canon;
    std::ostringstream summary;
    summary << "The hidden timeline contains " << events.size()
            << " canonical events; they are listed below in chronological order from the hidden truth graph.";
    block.summary = summary.str();
    for (const Event* event : events) {
        if (event != nullptr) {
            block.hidden_event_ids.push_back(event->id);
        }
    }
    return block;
}

[[nodiscard]] std::optional<AnswerBlock> build_debug_trace_block(const ArchiveEngineState& state,
                                                                  const std::vector<const Contradiction*>& contradictions) {
    AnswerBlock block;
    block.heading = "Debug trace";
    block.min_access = AccessLevel::Debug;
    std::ostringstream summary;
    summary << "seed=" << state.seed
            << "; seeded_calendar_dispute=" << (state.include_seeded_calendar_dispute ? "true" : "false")
            << "; detected_contradictions=" << contradictions.size()
            << "; public contradictions are derived from typed claim semantics and never applied back to canon.";
    block.summary = summary.str();
    for (const Contradiction* contradiction : contradictions) {
        if (contradiction != nullptr) {
            block.contradiction_ids.push_back(contradiction->id);
        }
    }
    return block;
}

[[nodiscard]] Answer build_answer_what_happened(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    const std::vector<const Claim*> claims = visible_claims(state, access, archive_year);
    const std::vector<const Contradiction*> contradictions = visible_contradictions(state, access, archive_year);

    Answer answer;
    answer.access = access;
    answer.archive_year = archive_year;
    answer.blocks.push_back(build_public_answer_block(state, claims, contradictions));

    if (can_view(access, AccessLevel::Scholar)) {
        const std::optional<AnswerBlock> scholar = build_scholar_answer_block(state, claims, contradictions);
        if (scholar.has_value()) {
            answer.blocks.push_back(*scholar);
        }
    }
    if (can_view(access, AccessLevel::Curator)) {
        const std::optional<AnswerBlock> curator = build_curator_forgery_block(state, claims, contradictions);
        if (curator.has_value()) {
            answer.blocks.push_back(*curator);
        }
    }
    if (can_view(access, AccessLevel::Canon)) {
        const std::optional<AnswerBlock> canon = build_canon_hidden_sequence_block(state);
        if (canon.has_value()) {
            answer.blocks.push_back(*canon);
        }
    }
    if (access == AccessLevel::Debug) {
        const std::optional<AnswerBlock> debug = build_debug_trace_block(state, contradictions);
        if (debug.has_value()) {
            answer.blocks.push_back(*debug);
        }
    }
    return answer;
}

[[nodiscard]] std::string format_answer(const Answer& answer, const ArchiveEngineState& state) {
    std::ostringstream out;
    for (const AnswerBlock& block : answer.blocks) {
        if (!can_view(answer.access, block.min_access)) {
            continue;
        }
        out << block.heading << ": " << block.summary << "\n";

        if (!block.supporting_evidence.empty()) {
            out << "Evidence:\n";
            for (const EvidenceCitation& citation : block.supporting_evidence) {
                const std::optional<std::string> rendered = format_citation_for_access(citation, state, answer.access, answer.archive_year);
                if (rendered.has_value()) {
                    out << "- " << *rendered << "\n";
                }
            }
        }

        if (!block.contradiction_ids.empty()) {
            out << "Caveats:\n";
            for (const std::string& contradiction_id : block.contradiction_ids) {
                const Contradiction* contradiction = state.public_archive.find_contradiction(contradiction_id);
                if (contradiction == nullptr) {
                    continue;
                }
                if (!contradiction_visible_to(state, *contradiction, answer.access, answer.archive_year)) {
                    continue;
                }
                out << "- " << contradiction->id << ": " << contradiction->public_resolution_status;
                if (can_view(answer.access, contradiction->cause_min_access)) {
                    out << " (assigned cause: " << to_string(contradiction->assigned_cause) << ")";
                }
                if (can_view(answer.access, contradiction->hidden_resolution_min_access)) {
                    out << " [hidden resolution: " << contradiction->hidden_truth_resolution << "]";
                }
                out << "\n";
            }
        }

        if (!block.hidden_event_ids.empty() && can_view(answer.access, AccessLevel::Canon)) {
            out << "Hidden events:\n";
            for (const std::string& event_id : block.hidden_event_ids) {
                const Event* event = state.hidden_truth.find_event(event_id);
                if (event == nullptr) {
                    continue;
                }
                out << "- " << event->id << " (" << event->start_year;
                if (event->end_year != event->start_year) {
                    out << "-" << event->end_year;
                }
                out << "): " << event->title << "\n";
            }
        }
        out << "\n";
    }
    return out.str();
}

[[nodiscard]] std::string answer_what_happened(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    return format_answer(build_answer_what_happened(state, access, archive_year), state);
}

} // namespace archive
