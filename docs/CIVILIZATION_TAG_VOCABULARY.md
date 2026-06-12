# CivilizationSpec Tag Vocabulary

**Status:** advisory v1.1.x metadata vocabulary  
**Scope:** catalog inspection and authoring guidance only  
**Runtime effect:** none in v27.2

`CivilizationSpec.tags` are optional inspection metadata. They are intended to help catalog authors and reviewers understand the shape of a civilization recipe without changing generation behavior. The loader accepts lowercase snake_case tags; the validator warns on duplicates or non-standard formatting, but tags remain advisory rather than a hard schema lock.

## Geography / ecology

Recommended tags:

- `river_delta`
- `coastal_delta`
- `mountain_isolate`
- `arid_basin`
- `water_access`
- `steppe_ecology`
- `forest_lattice`
- `wetland`
- `island_polity`
- `desert_oasis`
- `highland`
- `tundra`
- `volcanic`

## Settlement / polity

Recommended tags:

- `urban`
- `nomadic`
- `mobile_polity`
- `city_state_network`
- `bureaucratic`
- `ritual_legal`
- `merchant_polity`
- `maritime_league`
- `monastic_polity`
- `clan_confederacy`
- `courtly_state`

## Artifact ecology

Recommended tags:

- `artifact_rich`
- `fragile_archive`
- `oral_archive`
- `salvage_archive`
- `merchant_archive`
- `shrine_archive`
- `ledger_heavy`
- `inscription_heavy`
- `preservation_bias`
- `copyist_interpolation`

## Truth and mystery

Recommended tags:

- `canon_restricted`
- `truth_partial`
- `protected_ambiguity`
- `mythic_truth_enabled`
- `forgery_pressure`
- `ritual_title_confusion`
- `chronology_dispute`
- `archive_loss`

## Strangeness / constraints

Recommended tags:

- `ecological_surrealism`
- `taboo_heavy`
- `low_real_world_similarity`
- `identity_ambiguity`
- `disaster_memory`
- `portable_authority`

## Technology / era

Recommended tags:

- `hydraulic_engineering`
- `script_reform`
- `calendar_reform`
- `canal_engineering`
- `maritime_navigation`
- `metallurgy`
- `terrace_agriculture`
- `glassmaking`

## Authoring notes

- Prefer lowercase snake_case.
- Prefer tags that describe durable structural features rather than transient plot details.
- Do not use tags to force generation behavior in v27.2.
- Duplicate tags should be treated as catalog hygiene warnings, not schema failures.
- Unknown but well-formed tags may be allowed while the vocabulary is still evolving.

## v27.3 derived registry

v27.3 adds `docs/CIVILIZATION_TAG_REGISTRY.md`, a controlled registry derived from the tags actually present in the bundled catalog. The vocabulary in this document remains advisory; the registry captures current catalog usage, counts, and authoring meanings for review. Neither document changes generation behavior.
