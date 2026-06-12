/*
 * Fixed sample archive construction and deterministic seed-gated fixture behavior. This is not full procedural world generation.
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

namespace archive {

[[nodiscard]] std::vector<Contradiction> detect_contradictions(const ArchiveEngineState& state) {
    std::vector<Contradiction> detected;

    for (const auto& [claim_id, claim] : state.public_archive.claims()) {
        if (!claim.semantics.has_value() || !claim.semantics->claimed_year.has_value()) {
            continue;
        }

        const Artifact* source = state.public_archive.find_artifact(claim.source_artifact_id);
        if (source == nullptr) {
            continue;
        }

        const int claimed_year = *claim.semantics->claimed_year;
        const std::vector<std::pair<std::string_view, std::optional<std::string>>> semantic_entities = {
            {"subject", claim.semantics->subject_entity_id},
            {"object", claim.semantics->object_entity_id},
        };

        for (const auto& [role, maybe_entity_id] : semantic_entities) {
            (void)role;
            if (!maybe_entity_id.has_value()) {
                continue;
            }
            const Entity* entity = state.hidden_truth.find_entity(*maybe_entity_id);
            if (entity == nullptr || entity->existence_interval.contains(claimed_year)) {
                continue;
            }

            const std::optional<ContradictionCause> maybe_cause = contradiction_cause_from_evidence(*source);
            const ContradictionCause cause = maybe_cause.value_or(ContradictionCause::UnresolvedGenerationBug);
            const std::string contradiction_id = make_contradiction_id(
                "unavailable_entity",
                {claim.id, *maybe_entity_id}
            );

            AccessLevel cause_access = AccessLevel::Scholar;
            if (cause == ContradictionCause::Forgery) {
                cause_access = AccessLevel::Curator;
            }
            if (cause == ContradictionCause::UnresolvedGenerationBug) {
                cause_access = AccessLevel::Debug;
            }

            add_unique_contradiction(detected, Contradiction{
                contradiction_id,
                "unavailable_entity",
                {claim.id},
                {source->id},
                contradiction_type_for_unavailable_entity(entity->type),
                cause,
                cause_access,
                entity->canonical_name + " is not available in hidden chronology until " +
                    std::to_string(entity->existence_interval.start_year) +
                    "; the claim is dated " + std::to_string(claimed_year) + ".",
                AccessLevel::Canon,
                "A claim in " + source->title + " uses " + entity->canonical_name +
                    " outside its ordinary chronological range; catalogers treat the mismatch as an authenticity or transmission problem.",
                source->discovery_year,
            });
        }
    }

    for (const auto& [left_id, left] : state.public_archive.claims()) {
        if (!left.semantics.has_value() || left.semantics->predicate_type != PredicateType::Became ||
            !left.semantics->object_entity_id.has_value()) {
            continue;
        }

        const Artifact* left_source = state.public_archive.find_artifact(left.source_artifact_id);
        if (left_source == nullptr ||
            left.type != ClaimType::MythicCompression ||
            !has_evidence_modifier(*left_source, EvidenceModifier::MythicCompression)) {
            continue;
        }

        for (const auto& [right_id, right] : state.public_archive.claims()) {
            if (left_id == right_id || !right.semantics.has_value() ||
                right.semantics->predicate_type != PredicateType::Received) {
                continue;
            }

            const bool same_office_as_subject =
                right.semantics->subject_entity_id.has_value() &&
                *right.semantics->subject_entity_id == *left.semantics->object_entity_id;
            const bool same_office_as_object =
                right.semantics->object_entity_id.has_value() &&
                *right.semantics->object_entity_id == *left.semantics->object_entity_id;
            if (!same_office_as_subject && !same_office_as_object) {
                continue;
            }

            const Artifact* right_source = state.public_archive.find_artifact(right.source_artifact_id);
            if (right_source == nullptr) {
                continue;
            }

            add_unique_contradiction(detected, Contradiction{
                make_contradiction_id("mythic_identity_compression", {left.id, right.id}),
                "mythic_identity_compression",
                {left.id, right.id},
                {left_source->id, right_source->id},
                ContradictionType::RitualContradiction,
                ContradictionCause::MythologizedMemory,
                AccessLevel::Scholar,
                "The song compresses several post-revolt offices and lineages into one ritual judge; it is not a literal personnel record.",
                AccessLevel::Canon,
                "The oral song and chronicle disagree about whether one office or several keepers received authority over the locks.",
                std::max(left_source->discovery_year, right_source->discovery_year),
            });
        }
    }

    if (state.include_seeded_calendar_dispute) {
        const Claim* chronicle = state.public_archive.find_claim("claim.levy_before_revolt");
        const Claim* ledger = state.public_archive.find_claim("claim.levy_exists_607");
        if (chronicle != nullptr && ledger != nullptr &&
            chronicle->semantics.has_value() && ledger->semantics.has_value() &&
            chronicle->semantics->predicate_type == PredicateType::Preceded &&
            ledger->semantics->predicate_type == PredicateType::ExistedInYear) {
            const Artifact* chronicle_source = state.public_archive.find_artifact(chronicle->source_artifact_id);
            const Artifact* ledger_source = state.public_archive.find_artifact(ledger->source_artifact_id);
            if (chronicle_source != nullptr && ledger_source != nullptr) {
                add_unique_contradiction(detected, Contradiction{
                    make_contradiction_id("calendar_date_disagreement", {chronicle->id, ledger->id}),
                    "calendar_date_disagreement",
                    {chronicle->id, ledger->id},
                    {chronicle_source->id, ledger_source->id},
                    ContradictionType::DateContradiction,
                    ContradictionCause::CalendarConversionError,
                    AccessLevel::Scholar,
                    "The chronicle uses dry-count regnal numbering while the ledger uses lock-year accounting; both can describe the same levy sequence.",
                    AccessLevel::Canon,
                    "The chronicle and ledger use incompatible date systems but agree that levies predate revolt escalation.",
                    std::max(chronicle_source->discovery_year, ledger_source->discovery_year),
                });
            }
        }
    }

    return detected;
}

void add_detected_contradictions_to_archive(ArchiveEngineState& state) {
    for (Contradiction contradiction : detect_contradictions(state)) {
        if (state.public_archive.find_contradiction(contradiction.id) == nullptr) {
            state.public_archive.add_contradiction(std::move(contradiction));
        }
    }
}

[[nodiscard]] ArchiveEngineState initialize_archive_engine(std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    ArchiveEngineState state;
    state.seed = seed;
    state.include_seeded_calendar_dispute = random_between(rng, 0.0, 1.0) > 0.5;

    state.hidden_truth.add_entity(Entity{
        "person.ivara",
        EntityType::Person,
        "Regent Ivara of the Silt Court",
        {"Ivara-of-Gates"},
        Interval{580, 642},
        "",
        "",
        AccessLevel::Canon,
    });

    state.hidden_truth.add_entity(Entity{
        "person.aru",
        EntityType::Person,
        "King Aru the Unmoored",
        {"Aru Without Banks"},
        Interval{520, 559},
        "",
        "",
        AccessLevel::Canon,
    });

    state.hidden_truth.add_entity(Entity{
        "office.drowned_chancellor",
        EntityType::Office,
        "Drowned Chancellor",
        {"Seal-Bearer Beneath Water"},
        Interval{617, 900},
        "event.salt_moon_schism",
        "",
        AccessLevel::Canon,
    });

    state.hidden_truth.add_entity(Entity{
        "site.reservoir_gate",
        EntityType::Site,
        "Reservoir Gate",
        {"Gate of Silt", "South Lock"},
        Interval{500, 900},
        "",
        "",
        AccessLevel::Public,
    });

    state.hidden_truth.add_entity(Entity{
        "script.green_seal",
        EntityType::Script,
        "Green-Seal Script",
        {"late lock-hand"},
        Interval{612, 900},
        "event.green_seal_standardized",
        "",
        AccessLevel::Scholar,
    });

    state.hidden_truth.add_entity(Entity{
        "language.lattice_dialect",
        EntityType::Language,
        "Lattice Dialect",
        {"Canal Lattice"},
        Interval{500, 760},
        "",
        "",
        AccessLevel::Scholar,
    });


    state.hidden_truth.add_entity(Entity{
        "site.salt_cellar_archive",
        EntityType::Site,
        "Salt-Cellar Archive",
        {"Archive Below Salt"},
        Interval{620, 900},
        "event.salt_moon_schism",
        "",
        AccessLevel::Scholar,
    });

    state.hidden_truth.add_entity(Entity{
        "script.pre_green_seal",
        EntityType::Script,
        "Pre-Green-Seal lock script",
        {"old lock hand"},
        Interval{500, 611},
        "",
        "event.green_seal_standardized",
        AccessLevel::Scholar,
    });

    state.hidden_truth.add_entity(Entity{
        "script.modern_catalog_transcription",
        EntityType::Script,
        "Modern catalog transcription",
        {"modern archival transcription"},
        Interval{800, 900},
        "",
        "",
        AccessLevel::Scholar,
    });

    state.hidden_truth.add_entity(Entity{
        "dialect.lower_lattice",
        EntityType::Dialect,
        "Lower Lattice dialect",
        {"lower lock speech"},
        Interval{500, 640},
        "",
        "event.salt_moon_schism",
        AccessLevel::Scholar,
    });

    state.hidden_truth.add_entity(Entity{
        "dialect.upper_lattice",
        EntityType::Dialect,
        "Upper Lattice archive dialect",
        {"upper lock-hand"},
        Interval{620, 760},
        "event.salt_moon_schism",
        "",
        AccessLevel::Scholar,
    });

    state.hidden_truth.add_entity(Entity{
        "dialect.late_lock_hand",
        EntityType::Dialect,
        "Late lock-hand dialect",
        {"post-schism lock-hand"},
        Interval{640, 760},
        "event.salt_moon_schism",
        "",
        AccessLevel::Scholar,
    });

    state.hidden_truth.add_entity(Entity{
        "dialect.lower_lattice_late",
        EntityType::Dialect,
        "Late Lower Lattice dialect",
        {"festival lower lattice"},
        Interval{650, 760},
        "event.salt_moon_schism",
        "",
        AccessLevel::Scholar,
    });

    state.hidden_truth.add_event(Event{
        "event.reservoir_mismanagement",
        "ecological_change",
        "Reservoir mismanagement crisis",
        601,
        603,
        {"person.ivara"},
        {},
        {"site.reservoir_gate"},
        "The southern reservoirs were overdrawn by court administrators, making the lower canals fail before the dry years peaked.",
        TruthLayer::CanonicalTruth,
        AccessLevel::Canon,
    });

    state.hidden_truth.add_event(Event{
        "event.canal_tax_reform",
        "law",
        "Canal tax reform",
        604,
        604,
        {"person.ivara"},
        {"event.reservoir_mismanagement"},
        {"site.reservoir_gate"},
        "Regent Ivara imposed a silt levy to fund emergency dredging after the reservoir crisis.",
        TruthLayer::CanonicalTruth,
        AccessLevel::Canon,
    });

    state.hidden_truth.add_event(Event{
        "event.temple_revolt",
        "revolt",
        "Temple revolt at Reservoir Gate",
        610,
        611,
        {"person.ivara"},
        {"event.canal_tax_reform"},
        {"site.reservoir_gate"},
        "Temple storehouse officials resisted the levy, turning a tax dispute into a local revolt.",
        TruthLayer::CanonicalTruth,
        AccessLevel::Canon,
    });

    state.hidden_truth.add_event(Event{
        "event.green_seal_standardized",
        "script_reform",
        "Green-Seal standardization",
        612,
        612,
        {"person.ivara"},
        {"event.temple_revolt"},
        {"site.reservoir_gate"},
        "The court standardized Green-Seal script to prevent temple scribes from altering canal decrees.",
        TruthLayer::CanonicalTruth,
        AccessLevel::Scholar,
    });

    state.hidden_truth.add_event(Event{
        "event.salt_moon_schism",
        "religious_schism",
        "Salt-Moon Schism",
        616,
        618,
        {"person.ivara"},
        {"event.temple_revolt", "event.green_seal_standardized"},
        {"site.reservoir_gate", "script.green_seal"},
        "The post-revolt settlement created a mortuary office called the Drowned Chancellor and split canal ritual from temple accounting.",
        TruthLayer::CanonicalTruth,
        AccessLevel::Canon,
    });

    // Artifact 1: propaganda inscription.
    {
        Artifact artifact;
        artifact.id = "artifact.victory_inscription";
        artifact.type = ArtifactType::Inscription;
        artifact.voice_register = ArtifactVoiceRegister::RoyalInscription;
        artifact.title = "Reservoir Gate Victory Inscription";
        artifact.creator_id = "person.ivara";
        artifact.attributed_creator_id = "person.ivara";
        artifact.true_creation_year = 611;
        artifact.claimed_creation_year = 611;
        artifact.discovery_year = 804;
        artifact.location_created = "site.reservoir_gate";
        artifact.location_found = "site.reservoir_gate";
        artifact.language_id = "language.lattice_dialect";
        artifact.dialect_id = "dialect.lower_lattice";
        artifact.script_id = "script.pre_green_seal";
        artifact.material = "basalt lock-stone";
        artifact.preservation_quality = 0.82;
        artifact.damage_profile = "edge erosion, three missing titulary signs";
        artifact.transmission_history = "primary inscription; no known copy";
        artifact.creator_knowledge_scope = "court view of the temple revolt";
        artifact.creator_bias_profile = "royal legitimation; anti-temple rhetoric";
        artifact.creator_motive = "present tax victory as ritual restoration";
        artifact.intended_audience = "canal workers and temple accountants";
        artifact.public_text = "When the Gate forgot its hinge, Ivara restored the water-road. The storehouse mouths were closed; the silt returned to measure.";
        artifact.literal_translation = "The Gate forgot hinge; Ivara set water-road upright; storehouse mouths shut.";
        artifact.scholarly_translation = "A court inscription claiming Ivara ended the Reservoir Gate revolt and restored canal order.";
        artifact.hidden_event_links = {"event.temple_revolt", "event.canal_tax_reform"};
        artifact.referenced_entity_ids = {"person.ivara", "site.reservoir_gate"};
        artifact.distortion_profile = "propaganda; formulaic ritual restoration";
        artifact.evidence_modifiers = {EvidenceModifier::Propaganda};
        artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.86;
            value.preservation_integrity = 0.82;
            value.temporal_proximity = 0.90;
            value.creator_access_to_events = 0.68;
            value.bias_penalty = 0.45;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.72;
            value.external_corroboration = 0.45;
            value.contradiction_penalty = 0.15;
            return value;
        }();
        artifact.generation_trace = "seed=" + std::to_string(seed) + "; source_events=event.temple_revolt,event.canal_tax_reform; distortion=propaganda";

        add_claim_to_archive(state.public_archive, artifact, Claim{
            "claim.victory_restoration",
            artifact.id,
            ClaimType::PropagandaClaim,
            "Regent Ivara",
            "restored",
            "Reservoir Gate order",
            "Ivara restored the water-road.",
            0.61,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::Restored, std::optional<std::string>{"person.ivara"}, std::optional<std::string>{"site.reservoir_gate"}, std::optional<int>{611}},
        });
        add_claim_to_archive(state.public_archive, artifact, Claim{
            "claim.temple_blame",
            artifact.id,
            ClaimType::PropagandaClaim,
            "temple storehouses",
            "caused",
            "canal disorder",
            "The storehouse mouths were closed.",
            0.36,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::Caused, std::nullopt, std::optional<std::string>{"site.reservoir_gate"}, std::optional<int>{611}},
        }, ArtifactVoiceClaimRole::SecondaryLine);

        state.public_archive.add_artifact(finalize_artifact(std::move(artifact)));
    }

    // Artifact 2: damaged manuscript.
    {
        Artifact artifact;
        artifact.id = "artifact.broken_shelf_chronicle";
        artifact.type = ArtifactType::DamagedManuscript;
        artifact.voice_register = ArtifactVoiceRegister::DamagedChronicle;
        artifact.title = "Broken Shelf Chronicle";
        artifact.creator_id = "scribe.unknown_late_lattice";
        artifact.attributed_creator_id = "scribe.unknown_late_lattice";
        artifact.true_creation_year = 635;
        artifact.claimed_creation_year = 621;
        artifact.discovery_year = 807;
        artifact.location_created = "site.reservoir_gate";
        artifact.location_found = "site.salt_cellar_archive";
        artifact.language_id = "language.lattice_dialect";
        artifact.dialect_id = "dialect.upper_lattice";
        artifact.script_id = "script.green_seal";
        artifact.material = "reed paper packet";
        artifact.preservation_quality = 0.41;
        artifact.damage_profile = "water loss; central column lacunae; line-end numerals missing";
        artifact.transmission_history = "copy of administrative chronicle, probably harmonized";
        artifact.creator_knowledge_scope = "post-schism archival summary";
        artifact.creator_bias_profile = "bureaucratic harmonization";
        artifact.creator_motive = "merge conflicting canal accounts";
        artifact.intended_audience = "later court auditors";
        artifact.public_text = "In the seventeenth dry count, after the levy of silt, the Gate rose. [three lines lost] The Moon-office received the locks.";
        artifact.literal_translation = "Dry-count seventeen; after silt levy; Gate rose; lacuna; Moon-office took locks.";
        artifact.scholarly_translation = "A damaged chronicle placing the revolt after the canal levy, but its dates are uncertain.";
        artifact.hidden_event_links = {"event.canal_tax_reform", "event.temple_revolt", "event.salt_moon_schism"};
        artifact.referenced_entity_ids = {"site.reservoir_gate", "office.drowned_chancellor", "script.green_seal"};
        artifact.distortion_profile = "damage; later copy; bureaucratic harmonization";
        artifact.evidence_modifiers = {EvidenceModifier::Damage, EvidenceModifier::LaterCopy};
        artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.54;
            value.preservation_integrity = 0.41;
            value.temporal_proximity = 0.61;
            value.creator_access_to_events = 0.58;
            value.bias_penalty = 0.18;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.66;
            value.external_corroboration = 0.50;
            value.contradiction_penalty = 0.22;
            return value;
        }();
        artifact.generation_trace = "seed=" + std::to_string(seed) + "; source_events=event.canal_tax_reform,event.temple_revolt,event.salt_moon_schism; distortion=damage,later_copy";

        add_claim_to_archive(state.public_archive, artifact, Claim{
            "claim.levy_before_revolt",
            artifact.id,
            ClaimType::FactualClaim,
            "canal tax reform",
            "preceded",
            "Reservoir Gate revolt",
            "after the levy of silt, the Gate rose",
            0.70,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::Preceded, std::nullopt, std::nullopt, std::optional<int>{610}},
        });
        add_claim_to_archive(state.public_archive, artifact, Claim{
            "claim.moon_office_locks",
            artifact.id,
            ClaimType::TranslationGuess,
            "Moon-office",
            "received",
            "locks",
            "The Moon-office received the locks.",
            0.48,
            AccessLevel::Scholar,
            ClaimSemantics{PredicateType::Received, std::optional<std::string>{"office.drowned_chancellor"}, std::nullopt, std::optional<int>{621}},
        }, ArtifactVoiceClaimRole::TranslationNote);

        state.public_archive.add_artifact(finalize_artifact(std::move(artifact)));
    }

    // Artifact 3: forgery with justified anachronism.
    {
        Artifact artifact;
        artifact.id = "artifact.aru_decree";
        artifact.type = ArtifactType::ForgedDecree;
        artifact.voice_register = ArtifactVoiceRegister::DisputedDecree;
        artifact.title = "Aru Decree of the Submerged Seal";
        artifact.creator_id = "forger.silt_court_partisan";
        artifact.attributed_creator_id = "person.aru";
        artifact.true_creation_year = 660;
        artifact.claimed_creation_year = 553;
        artifact.discovery_year = 809;
        artifact.location_created = "site.salt_cellar_archive";
        artifact.location_found = "site.salt_cellar_archive";
        artifact.language_id = "language.lattice_dialect";
        artifact.dialect_id = "dialect.late_lock_hand";
        artifact.script_id = "script.green_seal";
        artifact.material = "thin fired clay tablet";
        artifact.preservation_quality = 0.77;
        artifact.damage_profile = "suspiciously clean seal rim";
        artifact.transmission_history = "single find with no stratigraphic witness";
        artifact.creator_knowledge_scope = "post-schism partisan knowledge";
        artifact.creator_bias_profile = "dynastic legitimation";
        artifact.creator_motive = "retroactively legitimize Drowned Chancellor office";
        artifact.intended_audience = "curators and court litigants";
        artifact.public_text = "Aru, whose river has not ended, appoints the Drowned Chancellor to hold the locks beneath the Moon.";
        artifact.literal_translation = "Aru-not-ended appoints Drowned Chancellor under Moon to locks.";
        artifact.scholarly_translation = "A decree attributed to King Aru, but its title forms appear much later than the claimed date.";
        artifact.hidden_event_links = {"event.salt_moon_schism"};
        artifact.referenced_entity_ids = {"person.aru", "office.drowned_chancellor", "script.green_seal"};
        artifact.distortion_profile = "forgery; anachronistic office and script";
        artifact.evidence_modifiers = {EvidenceModifier::Forgery};
        artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.18;
            value.preservation_integrity = 0.77;
            value.temporal_proximity = 0.05;
            value.creator_access_to_events = 0.20;
            value.bias_penalty = 0.55;
            value.forgery_penalty = 0.95;
            value.translation_confidence = 0.58;
            value.external_corroboration = 0.10;
            value.contradiction_penalty = 0.80;
            return value;
        }();
        artifact.generation_trace = "seed=" + std::to_string(seed) + "; source_events=event.salt_moon_schism; distortion=forgery,anachronism";

        add_claim_to_archive(state.public_archive, artifact, Claim{
            "claim.aru_created_office",
            artifact.id,
            ClaimType::LegalFiction,
            "King Aru",
            "appointed",
            "Drowned Chancellor",
            "Aru appoints the Drowned Chancellor.",
            0.20,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::CreatedOffice, std::optional<std::string>{"person.aru"}, std::optional<std::string>{"office.drowned_chancellor"}, std::optional<int>{553}},
        }, ArtifactVoiceClaimRole::CatalogNote);

        state.public_archive.add_artifact(finalize_artifact(std::move(artifact)));
    }

    // Artifact 4: oral history.
    {
        Artifact artifact;
        artifact.id = "artifact.three_keepers_song";
        artifact.type = ArtifactType::OralHistory;
        artifact.voice_register = ArtifactVoiceRegister::OralSong;
        artifact.title = "Song of the Three Keepers";
        artifact.creator_id = "singer.lower_lock_lineage";
        artifact.attributed_creator_id = "singer.lower_lock_lineage";
        artifact.true_creation_year = 700;
        artifact.claimed_creation_year = 700;
        artifact.discovery_year = 812;
        artifact.location_created = "site.reservoir_gate";
        artifact.location_found = "site.reservoir_gate";
        artifact.language_id = "language.lattice_dialect";
        artifact.dialect_id = "dialect.lower_lattice_late";
        artifact.script_id = "script.modern_catalog_transcription";
        artifact.material = "oral performance transcribed on paper";
        artifact.preservation_quality = 0.64;
        artifact.damage_profile = "variant refrain; performer memory uncertain";
        artifact.transmission_history = "oral chain; first written transcription in 812";
        artifact.creator_knowledge_scope = "lineage memory of revolt and schism";
        artifact.creator_bias_profile = "ritual compression; local prestige";
        artifact.creator_motive = "preserve lineage claim to lock tending";
        artifact.intended_audience = "festival listeners";
        artifact.public_text = "Three keepers entered the Moon and returned as one judge with wet hands.";
        artifact.literal_translation = "three keepers go moon; one judge returns wet-handed";
        artifact.scholarly_translation = "An oral song compressing several offices or persons into a single mythic figure.";
        artifact.hidden_event_links = {"event.temple_revolt", "event.salt_moon_schism"};
        artifact.referenced_entity_ids = {"site.reservoir_gate", "office.drowned_chancellor"};
        artifact.distortion_profile = "mythic compression; ritual genealogy";
        artifact.evidence_modifiers = {EvidenceModifier::MythicCompression, EvidenceModifier::RitualAnachronism};
        artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.42;
            value.preservation_integrity = 0.64;
            value.temporal_proximity = 0.28;
            value.creator_access_to_events = 0.35;
            value.bias_penalty = 0.25;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.51;
            value.external_corroboration = 0.34;
            value.contradiction_penalty = 0.44;
            return value;
        }();
        artifact.generation_trace = "seed=" + std::to_string(seed) + "; source_events=event.temple_revolt,event.salt_moon_schism; distortion=mythic_compression";

        add_claim_to_archive(state.public_archive, artifact, Claim{
            "claim.three_as_one",
            artifact.id,
            ClaimType::MythicCompression,
            "three keepers",
            "became",
            "one moon judge",
            "Three keepers entered the Moon and returned as one judge.",
            0.40,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::Became, std::nullopt, std::optional<std::string>{"office.drowned_chancellor"}, std::optional<int>{700}},
        });

        state.public_archive.add_artifact(finalize_artifact(std::move(artifact)));
    }

    // Artifact 5: trade ledger.
    {
        Artifact artifact;
        artifact.id = "artifact.silt_ledger";
        artifact.type = ArtifactType::TradeLedger;
        artifact.voice_register = ArtifactVoiceRegister::TradeLedger;
        artifact.title = "Lower Lock Silt Ledger";
        artifact.creator_id = "clerk.lower_lock";
        artifact.attributed_creator_id = "clerk.lower_lock";
        artifact.true_creation_year = 607;
        artifact.claimed_creation_year = 607;
        artifact.discovery_year = 806;
        artifact.location_created = "site.reservoir_gate";
        artifact.location_found = "site.reservoir_gate";
        artifact.language_id = "language.lattice_dialect";
        artifact.dialect_id = "dialect.lower_lattice";
        artifact.script_id = "script.pre_green_seal";
        artifact.material = "waxed tally board";
        artifact.preservation_quality = 0.58;
        artifact.damage_profile = "corner loss; quantities mostly legible";
        artifact.transmission_history = "primary mundane record";
        artifact.creator_knowledge_scope = "narrow trade and levy accounting";
        artifact.creator_bias_profile = "bureaucratic minimalism";
        artifact.creator_motive = "record levy receipts";
        artifact.intended_audience = "canal tax officials";
        artifact.public_text = "Silt levy: twenty-three baskets from lower locks; temple exemption denied twice.";
        artifact.literal_translation = "silt levy 23 baskets; lower locks; temple exemption no/no";
        artifact.scholarly_translation = "A mundane ledger confirming silt levies before the revolt.";
        artifact.hidden_event_links = {"event.canal_tax_reform"};
        artifact.referenced_entity_ids = {"site.reservoir_gate"};
        artifact.distortion_profile = "narrow scope; numeric record";
        artifact.evidence_modifiers = {EvidenceModifier::NarrowScope};
        artifact.reliability_components = [] {
            ReliabilityComponents value;
            value.provenance_confidence = 0.72;
            value.preservation_integrity = 0.58;
            value.temporal_proximity = 0.95;
            value.creator_access_to_events = 0.80;
            value.bias_penalty = 0.05;
            value.forgery_penalty = 0.0;
            value.translation_confidence = 0.76;
            value.external_corroboration = 0.55;
            value.contradiction_penalty = 0.05;
            return value;
        }();
        artifact.generation_trace = "seed=" + std::to_string(seed) + "; source_events=event.canal_tax_reform; distortion=narrow_scope";

        add_claim_to_archive(state.public_archive, artifact, Claim{
            "claim.levy_exists_607",
            artifact.id,
            ClaimType::FactualClaim,
            "silt levy",
            "existed_in_year",
            "607",
            "Silt levy: twenty-three baskets from lower locks.",
            0.83,
            AccessLevel::Public,
            ClaimSemantics{PredicateType::ExistedInYear, std::nullopt, std::nullopt, std::optional<int>{607}},
        });

        state.public_archive.add_artifact(finalize_artifact(std::move(artifact)));
    }

    // Discovery records are a view-layer chronology over the full archive.
    // They do not mutate hidden truth or erase undiscovered artifacts from the
    // canonical in-memory state; answer/theory builders filter through them.
    for (const auto& [artifact_id, artifact] : state.public_archive.artifacts()) {
        state.discovery_log.push_back(Discovery{
            "discovery." + claim_suffix_for_id(artifact_id),
            artifact_id,
            artifact.discovery_year,
            artifact.location_found,
            artifact.min_access,
        });
    }
    std::sort(state.discovery_log.begin(), state.discovery_log.end(), [](const Discovery& lhs, const Discovery& rhs) {
        if (lhs.discovery_year != rhs.discovery_year) {
            return lhs.discovery_year < rhs.discovery_year;
        }
        return lhs.artifact_id < rhs.artifact_id;
    });

    add_mystery(state, Mystery{
        "mystery.third_lock_authority",
        "The Third Lock Authority",
        "Was authority over the Reservoir Gate given to one office, three keepers, or a ritual fiction?",
        RevealMode::PartiallyResolvable,
        0.72,
        0.84,
        {"artifact.broken_shelf_chronicle", "artifact.three_keepers_song", "artifact.aru_decree"},
        {"artifact.aru_decree"},
        {
            MysteryEvidenceLink{"artifact.broken_shelf_chronicle", std::optional<std::string>{"claim.moon_office_locks"}, MysteryEvidenceRole::CoreClue, 1.0},
            MysteryEvidenceLink{"artifact.three_keepers_song", std::optional<std::string>{"claim.three_as_one"}, MysteryEvidenceRole::CoreClue, 1.0},
            MysteryEvidenceLink{"artifact.aru_decree", std::optional<std::string>{"claim.aru_created_office"}, MysteryEvidenceRole::MisleadingClue, 0.70},
            MysteryEvidenceLink{"artifact.broken_shelf_chronicle", std::optional<std::string>{"claim.levy_before_revolt"}, MysteryEvidenceRole::ContextClue, 0.30},
        },
        AccessLevel::Public,
    });

    // Contradictions are derived from typed claim semantics. The seed-gated
    // calendar dispute is a deterministic detector input, not a hand-authored
    // post-hoc ledger entry.
    add_detected_contradictions_to_archive(state);

    return state;
}

} // namespace archive
