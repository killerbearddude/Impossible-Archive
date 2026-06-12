# CivilizationSpec Compatibility Notes

**Status:** v27.3 non-runtime design notes  
**Scope:** compatibility planning for future content work  
**Runtime effect:** none

These notes use the v27.2/v27.3 tag registry to outline likely compatibility and conflict patterns for future CivilizationSpec v1.2 design. They do not define resolver behavior, generation behavior, or runtime composition semantics.

## Compatible tag groups

### Water bureaucracy group

Useful together:

- `water_access`
- `bureaucratic`
- `ledger_heavy`
- `ritual_legal`
- `artifact_rich`
- `forgery_pressure`

Likely future fragment families:

- canal authority offices
- irrigation taxation ledgers
- ritual gate calendars
- flood-damage archive traces

### Mobile oath polity group

Useful together:

- `nomadic`
- `mobile_polity`
- `pastoral`
- `oral_archive`
- `oath_culture`
- `chronology_dispute`

Likely future fragment families:

- camp-court memory systems
- herd tally records
- oath cloth or knot evidence
- seasonal route contradictions

### Coastal trade archive group

Useful together:

- `coastal`
- `harbor_trade`
- `merchant_polity`
- `merchant_archive`
- `ledger_heavy`
- `forgery_pressure`
- `salvage_archive`

Likely future fragment families:

- pilotage ledgers
- customs-house copies
- salvage court awards
- foreign merchant name translation traps

### Fragile mystery group

Useful together:

- `fragile_archive`
- `truth_partial`
- `protected_ambiguity`
- `chronology_dispute`
- `disaster_memory`
- `dynastic_reconstruction`

Likely future fragment families:

- archive-loss pressure
- later copyist normalization
- disaster-date compression
- unresolved ritual title ambiguity

### Highland ritual-landscape group

Useful together:

- `highland`
- `terrace_landscape`
- `ritual_legal`
- `bureaucratic`
- `fragile_archive`
- `ledger_heavy`

Likely future fragment families:

- terrace repair rosters
- oracle/boundary court conflicts
- spring or salt access claims
- landslide provenance drift

## Potential conflicting tag pairs

These are advisory design tensions, not schema errors.

| Pair | Why it may conflict | Future handling idea |
|---|---|---|
| `nomadic` + `urban` | settlement assumptions can point in opposite directions | require a bridge concept such as seasonal capital, market town, or conquered administrative seat |
| `oral_archive` + `ledger_heavy` | evidence systems emphasize different recordkeeping media | allow if social strata or institutions are separated |
| `low_real_world_similarity` + `bureaucratic` | familiar office-heavy models can reduce novelty | require stronger artifact voice and non-obvious institution names |
| `protected_ambiguity` + `artifact_rich` | more evidence can over-resolve mysteries | require confidence caps and misleading/countervailing evidence |
| `mobile_polity` + `water_access` | mobile governance and fixed hydraulic infrastructure can clash | require seasonal water rights, routes, or portable office rituals |
| `disaster_memory` + `dynastic_reconstruction` | catastrophe evidence may be overwritten by later legitimation | model the later narrative as a distortion layer, not a replacement |
| `coastal` + `arid_basin` | ecology is mixed unless carefully scoped | require coastal-desert, salt lagoon, or oasis-port framing |
| `truth_partial` + `bureaucratic` | abundant records can imply high certainty | include damaged rosters, deliberate omissions, or archive silences |

## Compatibility review checklist

Before any future runtime composition work, a proposed composed spec should answer:

1. Which tag group provides the dominant evidence ecology?
2. Which tag group provides the dominant authority conflict?
3. Which tags are intentionally in tension?
4. Which artifacts prevent over-resolution of protected mysteries?
5. Which local institutions make the spec civilization-specific rather than generic?
6. Which tags are only authoring notes and should not enter runtime logic?

## Boundary statement

v27.3 does not add fragment parsing, composition resolving, patch strategies, generation branching on tags, cross-civilization merge, or multi-spec runtime state.
