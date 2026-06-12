/*
 * Read-only originality/trope-pressure audit. This module may flag risk but must not rewrite archive state.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

[[nodiscard]] std::string lower_ascii(std::string text) {
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

[[nodiscard]] bool contains_ci(const std::string& haystack, std::string_view needle) {
    return lower_ascii(haystack).find(lower_ascii(std::string(needle))) != std::string::npos;
}

void add_unique_trope(std::vector<TropeFlag>& flags, TropeFlag flag) {
    if (std::find(flags.begin(), flags.end(), flag) == flags.end()) {
        flags.push_back(flag);
    }
}

void add_originality_rationale(OriginalitySignal& signal,
                               TropeFlag flag,
                               std::string matched_pattern,
                               std::string explanation) {
    add_unique_trope(signal.trope_flags, flag);
    const auto exists = std::any_of(signal.rationales.begin(), signal.rationales.end(), [&](const OriginalityRationale& rationale) {
        return rationale.flag == flag && rationale.matched_pattern == matched_pattern;
    });
    if (!exists) {
        signal.rationales.push_back(OriginalityRationale{flag, std::move(matched_pattern), std::move(explanation)});
    }
}

[[nodiscard]] std::string trope_flags_text(const std::vector<TropeFlag>& flags) {
    if (flags.empty()) {
        return "none";
    }
    std::ostringstream out;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << to_string(flags[i]);
    }
    return out.str();
}

[[nodiscard]] std::string dependency_list_text(const std::vector<std::string>& dependencies) {
    if (dependencies.empty()) {
        return "none";
    }
    std::ostringstream out;
    for (std::size_t i = 0; i < dependencies.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << dependencies[i];
    }
    return out.str();
}

[[nodiscard]] std::vector<std::string> detect_local_dependencies(const std::string& text) {
    const std::vector<std::string> local_terms = {
        "reservoir gate",
        "reservoir",
        "silt levy",
        "lower lock",
        "locks",
        "lock authority",
        "drowned chancellor",
        "three keepers",
        "moon-office",
        "moon office",
        "moon-lock",
        "moon lock",
        "moon",
        "lock",
        "green-seal",
        "green seal",
        "dry-count",
        "water-road",
        "salt-moon",
        "mortuary office",
        "seal",
    };

    std::vector<std::string> dependencies;
    for (const std::string& term : local_terms) {
        if (contains_ci(text, term)) {
            add_unique_string(dependencies, term);
        }
    }
    return dependencies;
}

[[nodiscard]] double local_dependency_weight(std::string_view dependency) {
    // v17.1 calibration: repeated broad archive words should not max out
    // originality. Institution-specific terms carry more weight than generic
    // atmospheric tokens such as seal/moon/lock/reservoir.
    if (dependency == "drowned chancellor" || dependency == "three keepers" || dependency == "silt levy") {
        return 0.20;
    }
    if (dependency == "reservoir gate" || dependency == "green-seal" || dependency == "green seal" ||
        dependency == "moon-office" || dependency == "moon office" || dependency == "moon-lock" ||
        dependency == "moon lock" || dependency == "lock authority") {
        return 0.16;
    }
    if (dependency == "mortuary office") {
        return 0.15;
    }
    if (dependency == "lower lock" || dependency == "dry-count" || dependency == "salt-moon") {
        return 0.12;
    }
    if (dependency == "water-road") {
        return 0.10;
    }
    if (dependency == "reservoir" || dependency == "locks") {
        return 0.04;
    }
    if (dependency == "seal") {
        return 0.03;
    }
    if (dependency == "moon" || dependency == "lock") {
        return 0.02;
    }
    return 0.02;
}

[[nodiscard]] double weighted_local_dependency_score(const std::vector<std::string>& dependencies) {
    double score = 0.0;
    for (const std::string& dependency : dependencies) {
        score += local_dependency_weight(dependency);
    }
    return clamp01(score);
}

[[nodiscard]] bool has_substantial_local_dependency(const std::vector<std::string>& dependencies) {
    return weighted_local_dependency_score(dependencies) >= 0.24;
}

[[nodiscard]] bool feature_kind_is(OriginalityFeatureKind actual, std::initializer_list<OriginalityFeatureKind> allowed) {
    return std::find(allowed.begin(), allowed.end(), actual) != allowed.end();
}

[[nodiscard]] OriginalitySignal score_originality_feature(const OriginalityFeature& feature) {
    OriginalitySignal signal;
    signal.feature_id = feature.id;
    signal.feature_kind = feature.kind;
    signal.required_local_dependencies = detect_local_dependencies(feature.description);

    const bool local_dependency_present = !signal.required_local_dependencies.empty();
    const bool substantial_local_dependency = has_substantial_local_dependency(signal.required_local_dependencies);
    const bool explicit_generic = contains_ci(feature.description, "generic");
    const bool local_moon_lock_system =
        contains_ci(feature.description, "moon-lock") ||
        contains_ci(feature.description, "moon lock") ||
        contains_ci(feature.description, "moon-office") ||
        contains_ci(feature.description, "moon office") ||
        contains_ci(feature.description, "drowned chancellor");

    if (contains_ci(feature.description, "generic moon cult") ||
        ((contains_ci(feature.description, "moon") &&
          (contains_ci(feature.description, "cult") || contains_ci(feature.description, "goddess") || contains_ci(feature.description, "worship"))) &&
         !local_moon_lock_system && !substantial_local_dependency)) {
        add_originality_rationale(
            signal,
            TropeFlag::GenericMoonCult,
            "moon + cult/goddess/worship without local lock-authority dependencies",
            "Moon imagery is trope-risky only when it lacks the archive's specific lock, office, reservoir, or legal machinery."
        );
    }

    const bool mortuary_legal_office = contains_ci(feature.description, "mortuary office") ||
                                       (contains_ci(feature.description, "seal") && contains_ci(feature.description, "legally active"));
    if ((contains_ci(feature.description, "divine king") || contains_ci(feature.description, "sacred king") ||
         (contains_ci(feature.description, "sacred blood") && contains_ci(feature.description, "king"))) &&
        !mortuary_legal_office && !substantial_local_dependency) {
        add_originality_rationale(
            signal,
            TropeFlag::DivineKingAnalogue,
            "divine/sacred king formula without local institutional mediation",
            "Sacred kingship phrasing risks direct analogue unless transformed by a specific local legal office or archival mechanism."
        );
        add_originality_rationale(
            signal,
            TropeFlag::GenericSacredKingship,
            "sacred kingship formula",
            "Generic sacred monarchy language is familiar unless grounded in local administrative or ritual procedure."
        );
    }

    if ((contains_ci(feature.description, "lost empire") ||
         (contains_ci(feature.description, "empire") && contains_ci(feature.description, "hubris"))) &&
        !local_dependency_present) {
        add_originality_rationale(
            signal,
            TropeFlag::LostEmpireHubrisCollapse,
            "lost empire / empire collapsed from hubris",
            "Collapse-by-hubris is a familiar macro-trope unless tied to local causal pressures."
        );
    }

    if ((contains_ci(feature.description, "chosen one") || contains_ci(feature.description, "chosen prophecy")) &&
        !local_dependency_present) {
        add_originality_rationale(
            signal,
            TropeFlag::GenericChosenProphecy,
            "chosen one / chosen prophecy",
            "Chosen-prophecy language is generic without locally constrained evidence and institutions."
        );
    }

    if (feature_kind_is(feature.kind, {OriginalityFeatureKind::Artifact, OriginalityFeatureKind::LegalFormula, OriginalityFeatureKind::Claim}) &&
        contains_ci(feature.description, "restored") &&
        (contains_ci(feature.description, "order") || contains_ci(feature.description, "road") || contains_ci(feature.description, "gate"))) {
        add_originality_rationale(
            signal,
            TropeFlag::RomanStyleRestorationInscription,
            "restored + order/road/gate authority formula",
            "Public restoration formulae resemble familiar imperial inscriptions unless offset by local water-road and gate institutions."
        );
    }

    if ((contains_ci(feature.description, "bureaucracy") || contains_ci(feature.description, "tax official") ||
         (contains_ci(feature.description, "ledger") && !contains_ci(feature.description, "silt levy") && !contains_ci(feature.description, "lower lock"))) &&
        !substantial_local_dependency) {
        add_originality_rationale(
            signal,
            TropeFlag::GenericAncientBureaucracy,
            "bureaucracy/tax/ledger without local accounting pressure",
            "Administrative evidence is generic unless it depends on local canal, lock, levy, or calendar pressures."
        );
    }

    const double trope_count = static_cast<double>(signal.trope_flags.size());
    const double kind_trope_sensitivity =
        feature_kind_is(feature.kind, {OriginalityFeatureKind::Artifact, OriginalityFeatureKind::LegalFormula}) ? 0.04 : 0.0;

    signal.trope_similarity_score = clamp01((0.18 * trope_count) + kind_trope_sensitivity + (explicit_generic ? 0.25 : 0.0));

    const double local_dependency_score = weighted_local_dependency_score(signal.required_local_dependencies);
    const double institutional_specificity =
        (contains_ci(feature.description, "drowned chancellor") || contains_ci(feature.description, "three keepers") ||
         contains_ci(feature.description, "moon-office") || contains_ci(feature.description, "moon office") ||
         contains_ci(feature.description, "moon-lock") || contains_ci(feature.description, "mortuary office")) ? 0.20 : 0.0;
    const double evidence_specificity =
        (contains_ci(feature.description, "silt levy") || contains_ci(feature.description, "lower lock") ||
         contains_ci(feature.description, "reservoir gate") || contains_ci(feature.description, "water-road")) ? 0.13 :
        (contains_ci(feature.description, "reservoir") ? 0.04 : 0.0);
    const double linguistic_specificity =
        (contains_ci(feature.description, "green-seal") || contains_ci(feature.description, "green seal") ||
         contains_ci(feature.description, "dry-count") || contains_ci(feature.description, "lattice")) ? 0.10 : 0.0;
    const double kind_specificity =
        feature_kind_is(feature.kind, {OriginalityFeatureKind::Office, OriginalityFeatureKind::Ritual, OriginalityFeatureKind::Script}) ? 0.06 :
        (feature_kind_is(feature.kind, {OriginalityFeatureKind::LegalFormula}) ? 0.04 : 0.0);

    // Keep even strong local transformation below perfect certainty. A candidate
    // may be highly civilization-specific, but originality audit should not
    // report mechanical token accumulation as a flawless 1.00 transformation.
    signal.transformed_trope_score = std::min(
        0.86,
        clamp01(local_dependency_score + institutional_specificity + evidence_specificity + linguistic_specificity + kind_specificity)
    );

    signal.direct_copy_risk_score = clamp01(
        signal.trope_similarity_score * (1.0 - signal.transformed_trope_score) +
        ((!signal.trope_flags.empty() && !local_dependency_present) ? 0.25 : 0.0)
    );

    signal.civilization_specificity_score = clamp01(
        0.10 + signal.transformed_trope_score - (0.35 * signal.direct_copy_risk_score)
    );

    if (signal.civilization_specificity_score >= 0.70) {
        signal.assessment = "highly civilization-specific; familiar material is transformed by local institutions and evidence";
    } else if (signal.civilization_specificity_score >= 0.45) {
        signal.assessment = "moderately civilization-specific; contains some trope pressure but has local dependencies";
    } else if (!signal.trope_flags.empty()) {
        signal.assessment = "trope risk; revise toward local dependencies before procedural expansion";
    } else {
        signal.assessment = "low specificity; lacks enough local dependencies";
    }
    return signal;
}

[[nodiscard]] OriginalitySignal score_originality_feature(const std::string& feature_id, const std::string& description) {
    return score_originality_feature(OriginalityFeature{feature_id, OriginalityFeatureKind::Unknown, description});
}

[[nodiscard]] std::vector<OriginalitySignal> build_originality_signals(const ArchiveEngineState& state) {
    std::vector<OriginalitySignal> signals;

    const Entity* drowned = state.hidden_truth.find_entity("office.drowned_chancellor");
    if (drowned != nullptr) {
        signals.push_back(score_originality_feature(OriginalityFeature{
            drowned->id,
            OriginalityFeatureKind::Office,
            drowned->canonical_name + " office created by the Salt-Moon Schism to hold authority over Reservoir Gate locks"
        }));
    }

    const Claim* three = state.public_archive.find_claim("claim.three_as_one");
    if (three != nullptr) {
        signals.push_back(score_originality_feature(OriginalityFeature{
            three->id,
            OriginalityFeatureKind::Ritual,
            three->literal_content + " Three Keepers ritual authority compressed into one moon judge at the Reservoir Gate locks"
        }));
    }

    const Artifact* aru = state.public_archive.find_artifact("artifact.aru_decree");
    if (aru != nullptr) {
        signals.push_back(score_originality_feature(OriginalityFeature{
            aru->id,
            OriginalityFeatureKind::Artifact,
            aru->title + " " + aru->public_text + " disputed decree tying Drowned Chancellor to locks and Moon-office language"
        }));
    }

    const Artifact* victory = state.public_archive.find_artifact("artifact.victory_inscription");
    if (victory != nullptr) {
        signals.push_back(score_originality_feature(OriginalityFeature{
            victory->id,
            OriginalityFeatureKind::Artifact,
            victory->title + " " + victory->public_text + " royal restoration inscription for Reservoir Gate water-road order"
        }));
    }

    const Entity* green_seal = state.hidden_truth.find_entity("script.green_seal");
    if (green_seal != nullptr) {
        signals.push_back(score_originality_feature(OriginalityFeature{
            green_seal->id,
            OriginalityFeatureKind::Script,
            green_seal->canonical_name + " script standardized after Reservoir Gate crisis to control canal decrees"
        }));
    }

    const Claim* levy = state.public_archive.find_claim("claim.levy_exists_607");
    if (levy != nullptr) {
        signals.push_back(score_originality_feature(OriginalityFeature{
            levy->id,
            OriginalityFeatureKind::Claim,
            levy->literal_content + " lower lock silt levy and temple exemption accounting"
        }));
    }

    return signals;
}

[[nodiscard]] std::string format_originality(const ArchiveEngineState& state, AccessLevel access) {
    std::ostringstream out;
    out << "Originality / trope pressure audit visible to " << to_string(access) << ":\n";
    if (!can_view(access, AccessLevel::Curator)) {
        out << "- originality audit is restricted to curator/canon/debug access\n";
        return out.str();
    }

    const std::vector<OriginalitySignal> signals = build_originality_signals(state);
    for (const OriginalitySignal& signal : signals) {
        out << "- " << signal.feature_id
            << " [" << to_string(signal.feature_kind) << "]"
            << ": trope_similarity=" << std::fixed << std::setprecision(2) << signal.trope_similarity_score
            << ", transformed_trope=" << std::fixed << std::setprecision(2) << signal.transformed_trope_score
            << ", direct_copy_risk=" << std::fixed << std::setprecision(2) << signal.direct_copy_risk_score
            << ", civilization_specificity=" << std::fixed << std::setprecision(2) << signal.civilization_specificity_score
            << ", flags=" << trope_flags_text(signal.trope_flags)
            << ", local_dependencies=" << dependency_list_text(signal.required_local_dependencies)
            << "\n";
        out << "  assessment: " << signal.assessment << "\n";
        if (access == AccessLevel::Debug) {
            if (signal.rationales.empty()) {
                out << "  rationale: no trope detector fired; specificity comes from local dependencies\n";
            } else {
                out << "  rationales:\n";
                for (const OriginalityRationale& rationale : signal.rationales) {
                    out << "    - " << to_string(rationale.flag) << ": " << rationale.matched_pattern
                        << " — " << rationale.explanation << "\n";
                }
            }
            out << "  debug: audit layer only; no hidden truth, artifact, claim, theory, or mystery mutation is performed\n";
        }
    }
    if (signals.empty()) {
        out << "- no originality signals available\n";
    }
    return out.str();
}

} // namespace archive
