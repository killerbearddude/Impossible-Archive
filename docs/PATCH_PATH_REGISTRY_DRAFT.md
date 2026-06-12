# Draft Patch-Path Registry

**Status:** v27.3 planning document only  
**Scope:** future CivilizationSpec v1.2 design review  
**Runtime effect:** none

This document lists possible patch paths that a future composition system might need to reason about. It is not a schema, not executable code, not a resolver, and not a commitment to any implementation.

## Registry principles

- Patch paths must resolve into one ordinary complete `CivilizationSpec` before runtime.
- Patch paths should be explicit and reviewable by humans.
- Patch behavior must preserve catalog cardinality and single-civilization runtime semantics.
- Patch behavior must never merge live `ArchiveEngineState` objects.
- Patch strategies, if ever implemented, require separate acceptance criteria.

## Candidate patch-path families

| Draft path family | Example field area | Future concern | Notes |
|---|---|---|---|
| identity fields | `id`, `display_name`, `description` | collision and provenance | likely base spec owns these unless explicit varianting exists |
| chronology fields | `earliest_year`, `latest_year` | conflicting time spans | must not silently widen chronology beyond artifact/event assumptions |
| geography arrays | `geographic_features`, `major_sites` | duplicate or incompatible places | may need normalized site keys and local naming rules |
| pressure arrays | `environmental_pressures`, `economic_pressures`, `ritual_pressures` | overfitting and generic bloat | should preserve dominant pressure hierarchy |
| institutions | `institution_archetypes`, `authority_conflicts` | unresolved conflict participants | conflicts must resolve to known institutions/actors/sites after composition |
| social actors | `social_actor_archetypes` | role ambiguity | must avoid duplicate labels with different meanings |
| trade goods | `trade_goods` | genre drift | goods should connect to economy, artifact media, or institutions |
| religious/mythic archetypes | `religious_or_mythic_archetypes` | protected mystery leakage | mythic truth must not over-resolve hidden truth |
| writing systems | `writing_system_archetypes` | anachronism risk | availability windows would be needed before runtime use |
| records/media | `recordkeeping_styles`, `artifact_media` | artifact voice mismatch | media and record styles should support each other |
| distortion modes | `evidence_distortion_modes` | too many universal explanations | distortion modes should be weighted or scoped in future work |
| mysteries | `mystery_archetypes` | over-resolution | preserve confidence caps and ambiguity policies |
| metadata | `tags`, `profile` | authoring guidance only | metadata remains non-runtime in v27.3 |

## Draft conflict review questions

1. Does every authority conflict still have resolvable participants?
2. Does every new artifact medium have at least one compatible recordkeeping style?
3. Does every writing system fit the stated chronology?
4. Do economic pressures connect to trade goods, institutions, or sites?
5. Do mystery archetypes remain protected by evidence distortion or archive loss?
6. Are tags descriptive, or are they being treated as hidden generator switches?

## Explicit non-implementation boundary

v27.3 does not add `SpecPatch`, patch strategies, patch-path registry code, fragment loading, composition resolving, CLI resolve commands, or JSON emission for composed specs.


## v28.0 implementation note

The v28.0 codebase validates inert `SpecPatch` records against a conservative draft-known patch-path registry, but it does not apply patches, resolve fragments, emit resolved specs, or alter generation behavior. The registry remains a validation seam only.
