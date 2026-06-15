# WNG Requirements for Crushline

Status: Draft request register  
Source design: `docs/crushline-design.md`  
Target engine milestone: Revised WNG v0.1 or formal WNG v0.1 milestone addendum

This document records the Whacky Node-Graph Layer requirements created by Crushline. It is intentionally separate from the game design document so WNG scope can be reviewed, accepted, sequenced, and implemented without mixing engine work with Crushline-specific production content.

Crushline may describe desired WNG capabilities, but Crushline production code must not depend on a requirement in this file until that requirement is accepted into the canonical WNG specification or a formal WNG milestone addendum.

## Ownership boundary

WNG owns reusable graph structure and editor behavior:

- `NodeId`, `PortId`, and `LinkId`
- node, port, and link storage
- graph mutation
- graph validation
- mutation summaries
- structural connection rules
- port side and direction model
- opaque host references
- graph serialization
- editor state
- selection
- hit testing
- graph rendering through WPL
- graph editor interaction

Crushline owns game meaning:

- `ResourceId`, `MachineId`, `RecipeId`, and `TechId`
- production rules
- recipe evaluation
- throughput calculations
- scenario objectives
- progression unlocks
- failure consequences
- player-facing production messages

WNG must preserve host references and structural graph data, but it must not interpret Crushline resources, machines, recipes, scenarios, or production consequences.

## Requirement status values

- `Requested`: Needed by Crushline, not yet accepted into WNG.
- `Accepted`: Approved for the WNG milestone/spec.
- `Implemented`: Implemented in WNG.
- `Deferred`: Not required for the current milestone.
- `Rejected`: Not accepted as a WNG responsibility.

All requirements below start as `Requested` until WNG planning updates their status.

## Revised WNG v0.1 requirements

| ID | Requirement | Status | Required before Crushline production? | Notes |
| --- | --- | --- | --- | --- |
| WNG-CRUSH-001 | Provide stable `NodeId`, `PortId`, and `LinkId` identifiers. | Requested | Yes | Required for Crushline metadata mappings and save/load stability. |
| WNG-CRUSH-002 | Store graph nodes, ports, and links as reusable engine structure. | Requested | Yes | Crushline maps game meaning onto these structures. |
| WNG-CRUSH-003 | Provide graph mutation APIs for node, port, and link creation/destruction. | Requested | Yes | Needed for editor actions and game-driven graph construction. |
| WNG-CRUSH-004 | Provide mutation summaries. | Requested | Yes | Needed for UI feedback, synchronization, and future undo/replay integration. |
| WNG-CRUSH-005 | Validate hard structural connection rules. | Requested | Yes | Examples include missing endpoints, self-links, illegal start/accept directions, duplicates that the engine cannot represent, and locked engine-level cardinality. |
| WNG-CRUSH-006 | Support custom port side values `Left`, `Bottom`, and `Right`. | Requested | Yes | Initial Crushline/WNG model has no top ports. |
| WNG-CRUSH-007 | Support custom port direction values `In`, `Out`, and `TwoWay`. | Requested | Yes | Direction is structural only; it does not imply resource compatibility. |
| WNG-CRUSH-008 | Store an opaque `HostRef` on nodes. | Requested | Yes | WNG preserves the reference but does not interpret it. |
| WNG-CRUSH-009 | Store an opaque `HostRef` on ports. | Requested | Yes | Needed to associate WNG ports with Crushline-owned port meaning. |
| WNG-CRUSH-010 | Store an opaque `HostRef` on links. | Requested | Yes | Needed to associate WNG links with Crushline-owned flow meaning. |
| WNG-CRUSH-011 | Serialize graph structure and opaque host references. | Requested | Yes | WNG should preserve opaque references; Crushline owns semantic data loading and interpretation. |
| WNG-CRUSH-012 | Provide a WPL rendering adapter for graph drawing. | Requested | Yes | WNG owns graph rendering behavior; WPL owns low-level drawing. |
| WNG-CRUSH-013 | Provide basic hit testing. | Requested | Yes | Required for node/port/link editor interaction. |
| WNG-CRUSH-014 | Provide basic selection state and behavior. | Requested | Yes | Required for graph editor manipulation. |
| WNG-CRUSH-015 | Provide node dragging interaction. | Requested | Yes | Required for first playable graph editing. |
| WNG-CRUSH-016 | Provide link creation interaction. | Requested | Yes | Required for first playable graph editing. |
| WNG-CRUSH-017 | Treat draw-list capacity failure as recoverable rendering failure. | Requested | Yes | Simulation and graph data remain valid even if a frame omits non-critical visuals. |
| WNG-CRUSH-018 | Avoid exact Crushline resource IDs as structural port types. | Requested | Yes | Exact resource compatibility belongs in Crushline evaluation, not WNG validation. |
| WNG-CRUSH-019 | Allow wrong-resource links when structural rules pass. | Requested | Yes | Wrong-resource connections are playable soft failures evaluated by Crushline. |
| WNG-CRUSH-020 | Keep attachment references out of WNG v0.1 scope. | Requested | No | Future WNG may store opaque attachment references for JSON, scripts, or external definitions. |

## Host reference model

WNG must support an opaque host reference that can be stored and preserved without interpretation.

Suggested shape:

```cpp
struct HostRef
{
    uint32_t type = 0;
    uint64_t value = 0;
};
```

Required locations for revised WNG v0.1:

- Node
- Port
- Link

The host application owns all meaning behind a host reference, including loading, parsing, validation, execution, simulation, UI interpretation, and domain-specific error handling.

## Port model

A WNG port must support:

- port number or index
- side: `Left`, `Bottom`, or `Right`
- direction: `In`, `Out`, or `TwoWay`
- opaque host reference

Initial side convention:

| Side | Typical Crushline use |
| --- | --- |
| Left | Inputs |
| Right | Outputs |
| Bottom | Byproducts, service ports, alternate outputs, machine-dependent special ports, and two-way ports |

Initial direction meaning:

| Direction | WNG structural meaning |
| --- | --- |
| In | Can accept links |
| Out | Can start links |
| TwoWay | Can start and accept links |

These conventions are visual and structural. They are not Crushline resource compatibility rules.

## Port type usage rule

WNG must not use exact Crushline resource IDs as structural port types.

Allowed broad structural categories include:

- empty or unspecified type
- `any`
- `material`
- `energy`
- `signal`
- `service`

Disallowed structural type values include:

- `IronOre`
- `Water`
- `SulfuricAcid`
- `CrushedIronOre`
- exact `ResourceId` keys

This rule exists because wrong-resource connections are core Crushline gameplay. WNG should allow any connection that satisfies structural graph rules. Crushline then evaluates whether the link produces useful output, zero output, contamination, waste, bottlenecks, or another authored consequence.

## Hard invalid versus soft failure

WNG may block hard invalid graph operations that cannot be represented safely or coherently by the engine.

Examples:

- connecting a port to itself
- connecting from a port that cannot start links
- connecting into a port that cannot accept links
- creating duplicate structural links that the graph engine cannot represent
- connecting to missing or destroyed ports
- violating a locked engine-level cardinality rule

WNG should not block soft game failures after structural rules pass.

Examples:

- water connected where acid was expected
- oil connected to a coolant input
- iron ore fed into a machine expecting crushed iron ore
- waste routed into a recycler that cannot process it efficiently
- a production chain starved by insufficient power
- a byproduct connected to a machine that produces zero useful output

Soft failures are Crushline simulation outcomes.

## Required implementation order

Recommended order before Crushline production work depends on WNG:

1. WPL v0.1 foundation available.
2. WNG revised v0.1 specification updated with Crushline-required capabilities.
3. WNG graph core implemented.
4. WNG custom port side/direction implemented.
5. WNG `HostRef` implemented.
6. WNG serialization implemented.
7. WNG rendering adapter implemented.
8. WNG hit testing, selection, node dragging, and link creation implemented.
9. Crushline showcase shell begins.
10. Crushline production model begins.

## Deferred future requests

The following are explicitly not required for WNG v0.1:

- opaque node attachment references for JSON, scripts, external definitions, or assets
- WNG loading, parsing, validation, execution, or semantic interpretation of external node definitions
- WNG execution of scripts or recipes
- WNG evaluation of Crushline production resources
- WNG objective/scenario evaluation

Future WNG may store opaque attachment references, but the host application must own the attached asset's meaning.
