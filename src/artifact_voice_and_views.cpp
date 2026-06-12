/*
 * Artifact voice rendering and access/archive-year filtered public views. Rendering must change presentation only; it must not mutate artifacts, claims, or truth.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

[[nodiscard]] std::string archive_year_text(int archive_year) {
    if (archive_year == kOpenEndedYear) {
        return "complete archive";
    }
    return std::to_string(archive_year);
}

[[nodiscard]] bool discovered_by_year(const Artifact& artifact, int archive_year) {
    return artifact.discovery_year <= archive_year;
}

[[nodiscard]] bool artifact_visible_to(const Artifact& artifact, AccessLevel access, int archive_year) {
    return can_view(access, artifact.min_access) && discovered_by_year(artifact, archive_year);
}

[[nodiscard]] bool claim_visible_to(const ArchiveEngineState& state, const Claim& claim, AccessLevel access, int archive_year) {
    if (!can_view(access, claim.min_access)) {
        return false;
    }
    const Artifact* source = state.public_archive.find_artifact(claim.source_artifact_id);
    return source != nullptr && artifact_visible_to(*source, access, archive_year);
}

[[nodiscard]] std::string display_artifact_type(const Artifact& artifact, AccessLevel access) {
    if (artifact.type == ArtifactType::ForgedDecree && !can_view(access, AccessLevel::Curator)) {
        return "royal decree, disputed";
    }
    return to_string(artifact.type);
}

[[nodiscard]] std::string display_reliability(const Artifact& artifact, AccessLevel access) {
    if (can_view(access, AccessLevel::Scholar)) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(2) << artifact.reliability_score;
        return out.str();
    }

    if (artifact.reliability_score >= 0.75) {
        return "high";
    }
    if (artifact.reliability_score >= 0.50) {
        return "moderate";
    }
    if (artifact.reliability_score >= 0.25) {
        return "low-moderate";
    }
    return "low";
}

[[nodiscard]] ArtifactVoiceProfile voice_profile_for(ArtifactVoiceRegister voice) {
    switch (voice) {
        case ArtifactVoiceRegister::RoyalInscription:
            return ArtifactVoiceProfile{
                voice,
                {"By seal and gate", "let the water-road stand"},
                {"edge erosion", "missing titulary signs"},
                {"formulaic restoration language"},
                false,
            };
        case ArtifactVoiceRegister::DamagedChronicle:
            return ArtifactVoiceProfile{
                voice,
                {"in the dry count", "after the levy"},
                {"[lacuna]", "[uncertain reading]"},
                {"later-copy harmonization possible"},
                false,
            };
        case ArtifactVoiceRegister::TradeLedger:
            return ArtifactVoiceProfile{
                voice,
                {"entry", "quantity", "exemption"},
                {"corner loss", "quantity partially legible"},
                {"administrative narrowness"},
                false,
            };
        case ArtifactVoiceRegister::OralSong:
            return ArtifactVoiceProfile{
                voice,
                {"refrain", "returned as one", "wet hands"},
                {"variant refrain", "performer uncertainty"},
                {"ritual compression"},
                false,
            };
        case ArtifactVoiceRegister::DisputedDecree:
            return ArtifactVoiceProfile{
                voice,
                {"decree formula", "seal claim", "appointment formula"},
                {"clean seal rim", "catalog caution"},
                {"authenticity disputed"},
                true,
            };
    }
    return ArtifactVoiceProfile{};
}

[[nodiscard]] std::vector<const Claim*> visible_claims_for_artifact_id(const ArchiveEngineState& state,
                                                                       const std::string& artifact_id,
                                                                       AccessLevel access,
                                                                       int archive_year) {
    std::vector<const Claim*> result;
    const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
    if (artifact == nullptr || !artifact_visible_to(*artifact, access, archive_year)) {
        return result;
    }

    for (const std::string& claim_id : artifact->claim_ids) {
        const Claim* claim = state.public_archive.find_claim(claim_id);
        if (claim != nullptr && claim_visible_to(state, *claim, access, archive_year)) {
            result.push_back(claim);
        }
    }
    return result;
}

[[nodiscard]] bool voice_role_publicly_renderable(ArtifactVoiceClaimRole role, AccessLevel access) {
    if (role == ArtifactVoiceClaimRole::OmittedFromPublicRendering && !can_view(access, AccessLevel::Scholar)) {
        return false;
    }
    return true;
}

[[nodiscard]] std::vector<VoiceClaimView> visible_voice_claims_for_artifact(
    const Artifact& artifact,
    const ArchiveEngineState& state,
    AccessLevel access,
    int archive_year,
    std::optional<ArtifactVoiceClaimRole> role_filter
) {
    std::vector<VoiceClaimView> result;
    if (!artifact_visible_to(artifact, access, archive_year)) {
        return result;
    }

    for (const ArtifactVoiceClaimLink& link : artifact.voice_claim_links) {
        if (role_filter.has_value() && link.role != *role_filter) {
            continue;
        }
        if (!voice_role_publicly_renderable(link.role, access)) {
            continue;
        }
        const Claim* claim = state.public_archive.find_claim(link.claim_id);
        if (claim == nullptr || claim->source_artifact_id != artifact.id || !claim_visible_to(state, *claim, access, archive_year)) {
            continue;
        }
        result.push_back(VoiceClaimView{claim, link.role, link.weight});
    }

    std::sort(result.begin(), result.end(), [](const VoiceClaimView& lhs, const VoiceClaimView& rhs) {
        if (lhs.role != rhs.role) {
            return static_cast<int>(lhs.role) < static_cast<int>(rhs.role);
        }
        if (lhs.weight != rhs.weight) {
            return lhs.weight > rhs.weight;
        }
        const std::string lhs_id = lhs.claim == nullptr ? std::string{} : lhs.claim->id;
        const std::string rhs_id = rhs.claim == nullptr ? std::string{} : rhs.claim->id;
        return lhs_id < rhs_id;
    });
    return result;
}

[[nodiscard]] std::string trim_copy(std::string text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.erase(text.begin());
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.pop_back();
    }
    return text;
}

[[nodiscard]] bool is_sentence_punctuation(char ch) {
    return ch == '.' || ch == ';' || ch == ':' || ch == ',' || ch == '!' || ch == '?';
}

[[nodiscard]] std::string trim_trailing_sentence_punctuation(std::string text) {
    text = trim_copy(std::move(text));
    while (!text.empty() && (is_sentence_punctuation(text.back()) || std::isspace(static_cast<unsigned char>(text.back())) != 0)) {
        text.pop_back();
        text = trim_copy(std::move(text));
    }
    return text;
}

[[nodiscard]] std::string ensure_sentence_punctuation(std::string text) {
    text = trim_copy(std::move(text));
    if (text.empty()) {
        return text;
    }
    const char last = text.back();
    if (last == '.' || last == '!' || last == '?') {
        return text;
    }
    return text + ".";
}

[[nodiscard]] std::string compact_claim_phrase(const Claim& claim) {
    if (!claim.literal_content.empty()) {
        return trim_trailing_sentence_punctuation(claim.literal_content);
    }
    return trim_trailing_sentence_punctuation(claim.subject + " " + claim.predicate + " " + claim.object);
}

[[nodiscard]] std::string joined_voice_claim_phrases(const std::vector<VoiceClaimView>& claims, std::string_view separator) {
    std::ostringstream out;
    bool first = true;
    for (const VoiceClaimView& view : claims) {
        if (view.claim == nullptr) {
            continue;
        }
        if (!first) {
            out << separator;
        }
        first = false;
        out << compact_claim_phrase(*view.claim);
    }
    return out.str();
}

[[nodiscard]] std::string joined_claim_phrases(const std::vector<const Claim*>& claims, std::string_view separator) {
    std::ostringstream out;
    for (std::size_t i = 0; i < claims.size(); ++i) {
        if (i != 0) {
            out << separator;
        }
        if (claims[i] != nullptr) {
            out << compact_claim_phrase(*claims[i]);
        }
    }
    return out.str();
}

[[nodiscard]] std::string no_duplicate_punctuation(std::string text) {
    const std::vector<std::string> bad_patterns = {"..", ".;", ";.", ";;", " ,", " .", " ;"};
    bool changed = true;
    while (changed) {
        changed = false;
        for (const std::string& bad : bad_patterns) {
            const std::size_t pos = text.find(bad);
            if (pos != std::string::npos) {
                if (bad == "..") {
                    text.replace(pos, bad.size(), ".");
                } else if (bad == ".;" || bad == ";.") {
                    text.replace(pos, bad.size(), ";");
                } else if (bad == ";;") {
                    text.replace(pos, bad.size(), ";");
                } else {
                    text.erase(pos, 1);
                }
                changed = true;
            }
        }
    }
    return text;
}

[[nodiscard]] std::string render_artifact_text(const Artifact& artifact,
                                               const ArchiveEngineState& state,
                                               AccessLevel access,
                                               int archive_year) {
    const ArtifactVoiceProfile profile = voice_profile_for(artifact.voice_register);
    const std::vector<VoiceClaimView> primary_claims = visible_voice_claims_for_artifact(artifact, state, access, archive_year, ArtifactVoiceClaimRole::PrimaryLine);
    const std::vector<VoiceClaimView> secondary_claims = visible_voice_claims_for_artifact(artifact, state, access, archive_year, ArtifactVoiceClaimRole::SecondaryLine);
    const std::vector<VoiceClaimView> catalog_claims = visible_voice_claims_for_artifact(artifact, state, access, archive_year, ArtifactVoiceClaimRole::CatalogNote);
    const std::vector<VoiceClaimView> translation_claims = visible_voice_claims_for_artifact(artifact, state, access, archive_year, ArtifactVoiceClaimRole::TranslationNote);

    const std::string primary_text = primary_claims.empty() ? std::string("no visible primary voice claim at this access level") : joined_voice_claim_phrases(primary_claims, "; ");
    const std::string secondary_text = joined_voice_claim_phrases(secondary_claims, "; ");
    const std::string catalog_text = catalog_claims.empty() ? primary_text : joined_voice_claim_phrases(catalog_claims, "; ");
    const std::string translation_text = joined_voice_claim_phrases(translation_claims, "; ");

    std::ostringstream out;
    switch (artifact.voice_register) {
        case ArtifactVoiceRegister::RoyalInscription:
            out << "Formulaic royal inscription: " << profile.formulae[0]
                << ", " << primary_text;
            if (!secondary_text.empty()) {
                out << "; secondary line: " << secondary_text;
            }
            out << "; " << profile.formulae[1] << ".";
            if (has_visible_evidence_modifier(artifact, EvidenceModifier::Propaganda, access)) {
                out << " The register is declarative and politically interested.";
            }
            break;

        case ArtifactVoiceRegister::DamagedChronicle:
            out << "Damaged chronicle rendering: " << profile.formulae[0]
                << " ... " << profile.damage_markers[0] << " ... " << primary_text
                << " " << profile.damage_markers[1] << ".";
            if (!translation_text.empty() && can_view(access, AccessLevel::Scholar)) {
                out << " Translation line: " << ensure_sentence_punctuation(translation_text);
            }
            if (can_view(access, AccessLevel::Scholar)) {
                out << " Translation note: " << ensure_sentence_punctuation(profile.translation_notes[0]);
            }
            break;

        case ArtifactVoiceRegister::TradeLedger:
            out << "Administrative ledger entries: quantity/accounting record; " << primary_text;
            if (!secondary_text.empty()) {
                out << "; secondary notation: " << secondary_text;
            }
            out << "; exemptions and receipts are tersely noted.";
            if (artifact.preservation_quality < 0.65) {
                out << " Damage note: " << ensure_sentence_punctuation(profile.damage_markers[0]);
            }
            break;

        case ArtifactVoiceRegister::OralSong:
            out << "Oral song refrain: " << primary_text << " — refrain repeated: wet hands, wet hands, returned as one.";
            if (!secondary_text.empty()) {
                out << " Secondary verse: " << ensure_sentence_punctuation(secondary_text);
            }
            if (has_visible_evidence_modifier(artifact, EvidenceModifier::MythicCompression, access)) {
                out << " The voice compresses lineage memory into ritual language.";
            }
            break;

        case ArtifactVoiceRegister::DisputedDecree:
            if (can_view(access, AccessLevel::Curator)) {
                out << "Curator rendering of forged/disputed decree: " << catalog_text
                    << ". Catalog classification: forged decree with anachronistic seal and office language.";
            } else {
                out << "Public catalog rendering: disputed royal decree; visible catalog claim: " << catalog_text
                    << ". Authenticity remains catalog-disputed.";
            }
            break;
    }

    if (can_view(access, AccessLevel::Scholar) && !artifact.scholarly_translation.empty()) {
        out << " Scholarly gloss: " << ensure_sentence_punctuation(artifact.scholarly_translation);
    }
    return no_duplicate_punctuation(out.str());
}

[[nodiscard]] std::string format_artifacts(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::ostringstream out;
    out << "Artifacts visible to " << to_string(access) << " at archive year " << archive_year_text(archive_year) << ":\n";
    out << std::left
        << std::setw(34) << "id"
        << std::setw(28) << "type"
        << std::setw(10) << "claimed"
        << std::setw(10) << "found"
        << std::setw(16) << "reliability"
        << "title\n";
    out << std::string(116, '-') << "\n";

    for (const auto& [artifact_id, artifact] : state.public_archive.artifacts()) {
        if (!artifact_visible_to(artifact, access, archive_year)) {
            continue;
        }
        out << std::left
            << std::setw(34) << artifact.id
            << std::setw(28) << display_artifact_type(artifact, access)
            << std::setw(10) << artifact.claimed_creation_year
            << std::setw(10) << artifact.discovery_year
            << std::setw(16) << display_reliability(artifact, access)
            << artifact.title << "\n";

        out << "  rendered text: " << render_artifact_text(artifact, state, access, archive_year) << "\n";
        if (can_view(access, AccessLevel::Scholar)) {
            out << "  scholarly: " << artifact.scholarly_translation << "\n";
        }
        if (can_view(access, AccessLevel::Canon)) {
            out << "  true_creation_year: " << artifact.true_creation_year
                << "; evidence_profile: " << artifact.distortion_profile
                << "; evidence_modifiers: " << evidence_modifier_list_text(artifact) << "\n";
        }
        if (can_view(access, AccessLevel::Debug)) {
            out << "  trace: " << artifact.generation_trace << "\n";
        }
    }

    return out.str();
}

[[nodiscard]] std::string format_claims(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::ostringstream out;
    out << "Claims visible to " << to_string(access) << " at archive year " << archive_year_text(archive_year) << ":\n";

    for (const auto& [claim_id, claim] : state.public_archive.claims()) {
        if (!claim_visible_to(state, claim, access, archive_year)) {
            continue;
        }
        const Artifact* artifact = state.public_archive.find_artifact(claim.source_artifact_id);
        if (artifact == nullptr) {
            continue;
        }
        out << "- " << claim.id << " [" << to_string(claim.type) << ", confidence="
            << std::fixed << std::setprecision(2) << claim.confidence << "] "
            << claim.subject << " " << claim.predicate << " " << claim.object
            << " (source: " << claim.source_artifact_id << ")";
        if (can_view(access, AccessLevel::Scholar) && claim.semantics.has_value()) {
            out << " {semantic_predicate=" << to_string(claim.semantics->predicate_type);
            if (claim.semantics->claimed_year.has_value()) {
                out << ", claimed_year=" << *claim.semantics->claimed_year;
            }
            out << "}";
        }
        out << "\n";
    }

    return out.str();
}

[[nodiscard]] bool contradiction_visible_to(const ArchiveEngineState& state,
                                                                const Contradiction& contradiction,
                                                                AccessLevel access,
                                                                int archive_year) {
    if (contradiction.detected_year > archive_year) {
        return false;
    }
    if (contradiction.involved_artifact_ids.empty()) {
        return false;
    }
    const bool artifacts_visible = std::all_of(contradiction.involved_artifact_ids.begin(), contradiction.involved_artifact_ids.end(),
        [&](const std::string& artifact_id) {
            const Artifact* artifact = state.public_archive.find_artifact(artifact_id);
            return artifact != nullptr && artifact_visible_to(*artifact, access, archive_year);
        });
    if (!artifacts_visible) {
        return false;
    }

    if (contradiction.involved_claim_ids.empty()) {
        return false;
    }
    return std::all_of(contradiction.involved_claim_ids.begin(), contradiction.involved_claim_ids.end(),
        [&](const std::string& claim_id) {
            const Claim* claim = state.public_archive.find_claim(claim_id);
            return claim != nullptr && claim_visible_to(state, *claim, access, archive_year);
        });
}

[[nodiscard]] std::string format_contradictions(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::ostringstream out;
    out << "Contradictions visible to " << to_string(access) << " at archive year " << archive_year_text(archive_year) << ":\n";

    for (const auto& [contradiction_id, contradiction] : state.public_archive.contradictions()) {
        (void)contradiction_id;
        if (!contradiction_visible_to(state, contradiction, access, archive_year)) {
            continue;
        }
        out << "- " << contradiction.id << " [" << to_string(contradiction.type) << "]\n";
        out << "  public: " << contradiction.public_resolution_status << "\n";
        if (can_view(access, contradiction.cause_min_access)) {
            out << "  assigned cause: " << to_string(contradiction.assigned_cause) << "\n";
        } else if (can_view(access, AccessLevel::Scholar)) {
            out << "  assigned cause: restricted; competing explanations remain under review\n";
        }
        if (can_view(access, contradiction.hidden_resolution_min_access)) {
            out << "  hidden resolution: " << contradiction.hidden_truth_resolution << "\n";
        }
    }

    return out.str();
}

[[nodiscard]] std::string format_anachronisms(const ArchiveEngineState& state, AccessLevel access, int archive_year) {
    std::ostringstream out;
    out << "Anachronism reports visible to " << to_string(access) << " at archive year " << archive_year_text(archive_year) << ":\n";

    const std::vector<AnachronismReport> reports = detect_anachronisms(state);
    if (reports.empty()) {
        out << "- none\n";
        return out.str();
    }

    bool emitted = false;
    std::vector<std::string> public_notices;
    for (const AnachronismReport& report : reports) {
        const Artifact* artifact = state.public_archive.find_artifact(report.artifact_id);
        if (artifact == nullptr || !artifact_visible_to(*artifact, access, archive_year)) {
            continue;
        }
        if (!can_view(access, AccessLevel::Scholar)) {
            // Public users can see that authenticity is disputed, but not the
            // validator's internal availability table or forgery classification.
            if (report.status == AnachronismStatus::ValidBecauseForged &&
                std::find(public_notices.begin(), public_notices.end(), report.artifact_id) == public_notices.end()) {
                public_notices.push_back(report.artifact_id);
                out << "- " << report.artifact_id << ": authenticity disputed by catalogers\n";
                emitted = true;
            }
            continue;
        }

        emitted = true;
        out << "- " << report.artifact_id
            << ": " << report.referenced_item
            << ", " << report.checked_year_kind << "=" << report.checked_year
            << ", valid_range=" << availability_range_text(report)
            << ", status=" << to_string(report.status);
        if (can_view(access, AccessLevel::Curator)) {
            out << ", explanation=" << report.allowed_explanation;
        }
        out << "\n";
    }

    if (!emitted) {
        out << "- none visible\n";
    }

    return out.str();
}

} // namespace archive
