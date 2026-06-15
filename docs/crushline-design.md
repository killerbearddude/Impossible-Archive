# Crushline Design Document

Status: Final review draft baseline  
Primary showcase technology: Whacky Node-Graph Layer  
Primary platform foundation: Whacky Platform Layer  
Game genre: Node-graph production sandbox / expert-pack progression puzzle  
Target identity: A creativity-driven production graph game inspired by expert Minecraft modpack progression

This document defines the current Crushline product and architecture baseline. It is intentionally game-facing, but it also records first-party WNG and WPL requirements that must be accepted into their own specifications before Crushline production work depends on them.

## 1. Executive Summary

Crushline is a node-graph production sandbox where players solve large, interconnected production problems by building graph-based factory systems.

The game is inspired by expert Minecraft modpacks: broad technology progression, interdependent recipe chains, alternate routes, bottlenecks, byproducts, failures, surprises, and player-authored solutions. Crushline should not be a short linear puzzle game. It should become a deep progression game where players unlock capabilities, experiment with production graphs, fail safely, learn from consequences, and discover multiple routes to completion.

Crushline is also the showcase application for the Whacky technology stack:

| Layer | Responsibility |
| --- | --- |
| Crushline | Showcase game and production/progression content |
| Whacky Node-Graph Layer (WNG) | Reusable C++ node graph engine |
| Whacky Platform Layer (WPL) | Low-level Linux-native C platform foundation |

The core design rule is:

```text
Crushline owns game meaning.
WNG owns reusable node-graph structure and editor behavior.
WPL owns low-level platform services.
```

Because WPL, WNG, and Crushline are first-party studio projects, Crushline may define engine requirements for WNG. Any WNG requirement above the current WNG specification must be explicitly documented and folded into the revised WNG v0.1 specification or a formal WNG v0.1 milestone addendum before Crushline production work depends on it.

Crushline production work that requires WNG must wait until the required revised WNG v0.1 capabilities are complete.

## 2. Product Identity

Crushline is a production-chain graph game where the graph is the factory.

The player does not primarily place belts, pipes, terrain objects, or physical buildings. Instead, the player constructs production logic as a node graph. Machines, recipes, resources, byproducts, power, waste, and constraints are represented through graph nodes, ports, and links.

The intended player question is:

```text
I need to produce this advanced item.
What chain of resources, machines, recipes, catalysts, power systems,
waste handling, and unlocks can get me there?
```

The fun should come from:

- discovering alternate paths
- making experimental connections
- failing and learning why
- solving bottlenecks
- routing byproducts
- upgrading earlier chains
- using old waste as new material
- meeting constraints in different ways
- optimizing without being forced into a single correct solution

Crushline should feel closer to an expert modpack progression web than a fixed campaign of isolated puzzles.

## 3. Design Pillars

### 3.1 The graph is the factory

The production graph is not a visual aid. It is the main play space.

Nodes represent machines, sources, sinks, buffers, processors, converters, objectives, and eventually automation helpers.

Ports represent machine connection points.

Links represent intentional resource, energy, fluid, control, or service connections.

The player's graph is the solution attempt.

### 3.2 Failure is playable

Crushline must allow the player to fail.

Incorrect, inefficient, contaminated, bottlenecked, or unintended connections are not merely errors to prevent. They are part of the core puzzle loop. The player should be able to experiment, observe consequences, learn from bad graph designs, and discover unexpected production routes.

The game should not ask only whether a connection can be made. It should ask what happens if the connection is made.

Examples of allowed failure:

- connecting the wrong liquid
- feeding the wrong item into a machine
- creating a bottleneck
- routing a byproduct into an inappropriate processor
- underpowering a production chain
- overloading a machine
- producing unwanted waste
- producing zero output from an incorrect setup
- creating an inefficient but technically editable route
- discovering that a design does not satisfy the milestone

The result should be simulated, explained, and surfaced to the player.

### 3.3 Progression is extensive and interconnected

Crushline progression should be broad, deep, and authored with long-term dependencies in mind.

A technology unlock should not simply grant the next machine. It should unlock a new capability space.

Examples:

| Capability | Unlock space |
| --- | --- |
| Basic Mechanical Processing | Crushing, sorting, and basic ore preparation |
| Fluid Handling | Water inputs, slurry outputs, pumps, tanks, and fluid recipes |
| Thermal Processing | Smelting, roasting, calcination, and high-power conversion |
| Chemical Processing | Acids, catalysts, separation, and higher-yield routes |
| Waste Management | Byproduct handling, recycling, disposal, and reuse chains |
| Automation | Repeatable routing, control nodes, and throughput stabilization |
| Advanced Materials | Alloys, composites, purified intermediates, and late-game targets |

Progression should create new solution options, not only higher numbers.

### 3.4 Multiple paths to completion

Most major objectives should support more than one valid route.

A target such as Iron Ingot should eventually be reachable through several chains:

| Route | Chain | Strength | Cost |
| --- | --- | --- | --- |
| Mechanical | Iron Ore -> Crusher -> Washer -> Smelter -> Iron Ingot | Simple and early | Lower yield, more waste |
| Thermal | Iron Ore -> Roaster -> Furnace -> Iron Ingot | Compact | High energy demand |
| Chemical | Iron Slurry -> Separator -> Iron Powder -> Furnace -> Iron Ingot | High yield | Fluids, catalysts, waste streams |
| Recycling | Scrap Metal -> Sorter -> Reprocessor -> Iron Ingot | Handles surplus and scrap | Depends on inconsistent inputs |
| Advanced | Purified Iron Solution -> Crystallizer -> High-Efficiency Smelter -> Iron Ingot | Efficient | Deep progression gate |

The game should reward players for finding routes that match their unlocked capabilities and scenario constraints.

### 3.5 Byproducts are not trash forever

Byproducts are central to Crushline.

Early game byproducts may be problems. Mid-game byproducts may become recoverable. Late-game byproducts may become valuable inputs.

Example:

- Early game: Iron Slurry is a waste-handling problem.
- Mid game: Iron Slurry can be filtered into Iron Dust and Dirty Water.
- Late game: Iron Slurry becomes part of a catalyst chain for advanced alloy production.

This creates long-term progression satisfaction. The player learns that earlier waste may become future opportunity.

### 3.6 Player creativity matters

Crushline should not over-prescribe graph layouts.

The game should avoid requiring one exact production chain unless a special challenge explicitly says so. Most objectives should be expressed as conditions, such as:

- Produce at least 50 Iron Ingots per minute.
- Handle all Iron Slurry.
- Stay under 200 kW.
- Use no more than 12 machines.
- Generate less than 5 Waste per minute.
- Complete using only mechanical processing.
- Complete without chemical processing.

A good scenario defines the problem. The player defines the solution.

## 4. Architecture Overview

Crushline sits above WNG and WPL.

| Layer | Owns |
| --- | --- |
| Crushline Game Layer | Resources, machines, recipes, tech progression, scenario objectives, production evaluator, save-game meaning, game-specific UI panels, failure and consequence rules |
| WNG Engine Layer | Graph data, nodes, ports, links, mutation, validation, mutation summaries, port layout and direction, host references, serialization, editor state, hit testing, selection, rendering adapter, graph-editor behavior |
| WPL Platform Layer | Linux windowing, input snapshots, timing, draw command buffers, software rendering, canvas/screen transforms, file I/O, replay infrastructure |

Crushline should not duplicate WNG graph behavior.

Crushline should not implement low-level WPL behavior.

Crushline should specialize WNG graph concepts into game concepts.

## 5. First-Party Engine Contract

WPL, WNG, and Crushline are first-party studio projects.

Crushline may require WNG capabilities when those capabilities are genuinely graph-engine features. A reviewer may point out that a feature is not present in the current WNG specification, but that does not automatically invalidate the design. It means the feature must be explicitly documented and added to the revised WNG v0.1 specification or formal WNG v0.1 milestone plan before Crushline production work depends on it.

The correct review questions are:

- Does this belong in WNG rather than Crushline?
- Is this required before Crushline implementation?
- Is the dependency labeled clearly?
- Is the milestone order correct?

The revised WNG v0.1 specification must incorporate all WNG requirements marked as required for Crushline production.

Crushline production work that depends on WNG must wait until the required revised WNG v0.1 capabilities are complete.

## 6. Relationship to WNG

WNG is the reusable node-graph engine used by Crushline.

WNG owns generic graph behavior:

- `NodeId`
- `PortId`
- `LinkId`
- graph mutation
- graph validation
- mutation summaries
- structural connection rules
- port side and direction model
- host references
- graph serialization
- editor state
- selection
- hit testing
- graph rendering through WPL
- graph editor interaction

Crushline owns domain meaning:

- `ResourceId`
- `MachineId`
- `RecipeId`
- `TechId`
- scenario objectives
- production rules
- route evaluation
- throughput calculations
- progression unlocks
- failure consequences
- player-facing production messages

A WNG node does not mean Crusher by itself. Crushline maps a WNG node to a game node instance.

A WNG port does not mean Iron Ore input by itself. Crushline maps a WNG port to machine connection meaning.

A WNG link does not calculate production by itself. Crushline evaluates the production meaning of graph links.

## 7. Required WNG Node Reference Model

WNG nodes, ports, and links must support an opaque host reference.

The host reference allows a game or tool layer to associate a WNG graph object with a domain object, such as:

- a Crushline machine instance
- a future JSON-defined node
- a script-backed node
- another application-specific object

WNG stores and preserves the reference but does not interpret it.

The host application owns all meaning behind the reference, including loading, parsing, validation, execution, simulation, UI interpretation, and domain-specific error handling.

Suggested shape:

```cpp
struct HostRef
{
    uint32_t type = 0;
    uint64_t value = 0;
};
```

WNG v0.1 does not need attachment references. Future WNG may support opaque node attachment references for assets such as JSON, scripts, or external definitions. WNG should store and serialize those references only. The host application owns loading, parsing, validation, execution, and semantic interpretation.

## 8. Required WNG Port Model

Crushline requires WNG to support customizable graph ports.

This is a WNG engine requirement, not a Crushline-only workaround.

A WNG port must support:

- port number or index
- side: `Left`, `Bottom`, or `Right`
- direction: `In`, `Out`, or `TwoWay`
- opaque host reference

There are no top ports in the initial Crushline/WNG model. Top ports are excluded initially to simplify layout, hit testing, and visual link routing. This is an initial Crushline/WNG constraint, not necessarily a permanent engine limitation.

Port side convention:

| Side | Common use |
| --- | --- |
| Left | Inputs |
| Right | Outputs |
| Bottom | Machine-dependent special ports, byproducts, service ports, alternate outputs, two-way ports |

Port direction meaning:

| Direction | WNG structural meaning |
| --- | --- |
| In | Can accept links |
| Out | Can start links |
| TwoWay | Can start and accept links |

This is graph structure, not game semantics.

WNG stores an opaque host reference on each port. WNG must not interpret this reference as a resource, machine, recipe, fluid, item, gas, or consequence type.

Examples of Crushline-owned mappings:

- `PortId -> expected resource`
- `PortId -> accepted resource family`
- `PortId -> recipe role`
- `PortId -> declared rate`
- `PortId -> byproduct flag`
- `PortId -> service connection behavior`

## 9. WNG Compatibility and Milestone Contract

Crushline should not be implemented against a vague future WNG. It has an explicit engine dependency contract.

Crushline's first playable production slice requires revised WNG v0.1 to provide:

- graph core
- `NodeId`, `PortId`, and `LinkId`
- node storage
- port storage
- link storage
- graph mutation
- mutation summaries
- connection creation/destruction
- custom port side: `Left`, `Bottom`, `Right`
- custom port direction: `In`, `Out`, `TwoWay`
- opaque `HostRef` on nodes, ports, and links
- graph serialization
- WPL rendering adapter
- basic hit testing
- basic selection
- node dragging
- link creation interaction

This is stronger than the earlier WNG-0.1 graph-core-only scope. That is acceptable because WNG is a first-party project and will revise its v0.1 specification based on Crushline needs.

Before Crushline depends on any WNG requirement above the current WNG specification, that requirement must be accepted into the canonical revised WNG v0.1 specification or recorded as a formal WNG v0.1 milestone addendum.

The recommended sequence is:

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

WPL may be used directly for the application shell, frame lifecycle, input snapshots, draw command submission, canvas transforms, debug overlay, and file I/O for Crushline-owned data. WPL must not become the permanent home for graph-engine behavior.

Temporary WPL-only graph visualization may be used only if it is isolated, documented as temporary, does not create a competing graph engine, and is scheduled for replacement by WNG editor/rendering.

WNG rendering and Crushline UI must treat WPL draw-list capacity failure as a recoverable rendering failure, not a simulation failure.

## 10. WNG Port Type Usage Rule

WNG must not use exact Crushline resource IDs as structural port types.

If WNG keeps a type field on ports, Crushline should use only broad structural categories or a permissive value.

Allowed examples:

- empty string
- `any`
- `material`
- `energy`
- `signal`
- `service`

Disallowed examples:

- `IronOre`
- `Water`
- `SulfuricAcid`
- `CrushedIronOre`
- exact `ResourceId` keys

Exact resource compatibility belongs in Crushline evaluation, not WNG validation. This rule exists because wrong-resource connections are important gameplay.

## 11. Hard Invalid vs. Soft Failure

Crushline distinguishes between hard invalid graph operations and soft game failures.

Hard invalid connections are graph operations that cannot be represented safely or coherently by the engine. Examples:

- connecting a port to itself
- connecting from a port that cannot start links
- connecting into a port that cannot accept links
- creating duplicate structural links that the graph engine cannot represent
- connecting to a missing or destroyed port
- violating a locked engine-level cardinality rule

These may be blocked by WNG.

Soft failure connections are game-domain mistakes or risky decisions that the simulation can resolve. Examples:

- water connected where acid was expected
- oil connected to a coolant input
- iron ore fed into a machine expecting crushed iron ore
- waste routed into a recycler that cannot process it efficiently
- a machine under-supplied by a bottleneck
- a production chain starved by insufficient power
- a byproduct connected to a machine that produces zero useful output

These should generally be allowed. The evaluator determines the consequence.

## 12. Connection Philosophy

Crushline connections should be permissive by default after WNG structural rules are satisfied.

A connection should be blocked only when:

1. WNG cannot represent the graph structure safely.
2. The interaction is impossible at the graph-engine level.
3. The player is attempting to connect missing, destroyed, or disabled graph elements.
4. A scenario explicitly forbids a class of connection as a challenge constraint.

A connection should not be blocked merely because:

- the resource is wrong
- the item is inefficient
- the liquid causes zero output
- the machine will not produce the intended output
- the chain bottlenecks
- the result is wasteful
- the output is unknown
- the player has not yet discovered the consequence

Those are simulation outcomes, not editor failures.

## 13. Core Game Loop

The core loop is:

1. The player selects or receives a milestone objective.
2. The player reviews unlocked resources, machines, recipes, and capabilities.
3. The player builds or modifies a production graph.
4. The player connects ports.
5. The evaluator resolves resource flow and consequences through the graph.
6. The game reports deficits, bottlenecks, byproducts, waste, power, unintended zero-output connections, discoveries, and objective progress.
7. The player iterates on the graph.
8. Completing objectives unlocks new capabilities, recipes, machines, or progression branches.
9. New unlocks make old graphs improvable, obsolete, or reusable in new ways.

The loop should encourage long-term graph evolution, not disposable one-off puzzle boards.

## 14. Discovery and Experimentation

The game should support discovery through failed or unexpected connections.

For Slice 1, wrong-resource behavior defaults to zero output.

Special wrong-input outcomes may be added case by case, but they are not part of the default evaluator behavior. This prevents the failure system from growing out of control too early.

Future special outcomes may include:

- unexpected input -> failed output
- wrong fluid -> contaminated output
- unknown combination -> discovery outcome

Those future outcomes must be authored deliberately and reviewed case by case.

## 15. Progression Model

Progression should be based on capabilities, not arbitrary levels.

A capability is a type of production power the player has gained. Examples:

- can crush solids
- can wash crushed solids
- can handle fluids
- can route byproducts
- can use thermal processing
- can use basic catalysts
- can separate slurry
- can recycle scrap
- can stabilize power demand
- can automate repeated chains

Recipes and machines should often require capabilities rather than only tier numbers.

Milestones are major progression gates. Examples:

- Basic Ore Processing: produce Iron Ingot and Copper Ingot through mechanical chains.
- Fluid Handling: produce clean water flow and use it in a washing recipe.
- Waste Accountability: handle all produced slurry or waste from a target chain.
- Basic Chemistry: produce Sulfuric Acid using a multi-step chemical route.
- Precision Materials: produce an advanced alloy using at least two processing branches.
- Automation Foundation: create a stable graph that satisfies a target rate for multiple resources.
- Closed-Loop Recovery: use a byproduct from one chain as an input to another.

Milestones should unlock multiple future options.

Some unlocks should support alternate requirements. Requirement composition should eventually support `AllOf`, `AnyOf`, `CompletedMilestone`, `ProducedResourceRate`, `HandledByproduct`, `DiscoveredRecipe`, `UnlockedCapability`, and `BuiltGraphPattern`.

## 16. Resource System

Resources are items, fluids, gases, energy units, waste products, catalysts, and intermediates that move through graph links.

Suggested resource categories:

- Solid
- Fluid
- Gas
- Energy
- Waste
- Catalyst
- Control Signal
- Service Fluid

Example early resources:

- Iron Ore
- Crushed Iron Ore
- Washed Iron Ore
- Iron Slurry
- Iron Dust
- Iron Ingot
- Dirty Water
- Water
- Waste
- Copper Ore
- Copper Ingot
- Coal
- Steam
- Power
- Impure Iron Ingot

Suggested shape:

```cpp
struct ResourceDef
{
    ResourceId id;
    std::string key;
    std::string display_name;
    ResourceKind kind;
    std::vector<std::string> tags;
    bool is_raw = false;
    bool is_waste = false;
    bool is_byproduct = false;
    bool is_final = false;
};
```

Resource tags are important for long-term content authoring.

## 17. Machine System

Machines define processing capabilities.

A machine is not a recipe. A machine is a host for one or more compatible recipes or behaviors.

Examples:

- Resource Source
- Crusher
- Washer
- Smelter
- Filter
- Waste Sink
- Basic Generator
- Sorter
- Furnace
- Roaster
- Separator
- Mixer
- Chemical Reactor
- Electrolyzer
- Compressor
- Recycler
- Storage Buffer

Suggested shape:

```cpp
struct MachineDef
{
    MachineId id;
    std::string key;
    std::string display_name;
    MachineClass machine_class;
    std::vector<CapabilityId> required_capabilities;
    float base_power_kw = 0.0f;
    float throughput_multiplier = 1.0f;
};
```

The game should eventually allow upgraded machine variants, but the first implementation can keep throughput fixed.

## 18. Recipe System

Recipes define transformations.

A normal recipe is:

```text
Inputs -> Outputs + Byproducts
```

For Slice 1, wrong-resource input defaults to zero output unless an authored exception exists.

Recipes should support required machine class, required capabilities, input rates, output rates, byproduct rates, power cost, unlock requirements, and tags.

Suggested shape:

```cpp
struct ResourceRate
{
    ResourceId resource_id;
    float rate_per_minute = 0.0f;
};

struct RecipeDef
{
    RecipeId id;
    std::string key;
    std::string display_name;
    MachineClass required_machine_class;
    std::vector<CapabilityId> required_capabilities;
    std::vector<ResourceRate> inputs;
    std::vector<ResourceRate> outputs;
    std::vector<ResourceRate> byproducts;
    float power_kw = 0.0f;
    std::vector<std::string> tags;
};
```

The initial recipe model is rate-based. Batch recipes can be added later as a separate model.

## 19. Production Graph Model

A Crushline production graph is a WNG graph plus Crushline-owned meaning.

A game node instance maps to a WNG node through `HostRef`:

```cpp
struct ProductionNode
{
    wng::NodeId graph_node_id;
    MachineId machine_id;
    std::optional<RecipeId> recipe_id;
};
```

Crushline maps WNG ports to production meaning:

```cpp
struct ProductionPort
{
    wng::PortId graph_port_id;
    ResourceId expected_resource_id;
    PortRole role;
    float declared_rate_per_minute = 0.0f;
};
```

Suggested `PortRole` values include input, output, byproduct, waste, energy input, energy output, control, and service.

A WNG link represents graph structure. Crushline interprets it as resource flow, energy flow, service flow, or control flow.

## 20. Slice 1 Evaluator Contract

Slice 1 should prove the loop with a deterministic evaluator before expanding into complex simulation.

Slice 1 evaluator assumptions:

- graph is directed and acyclic for production flow
- recipes use rate-per-minute values
- machines consume all required inputs at the same utilization ratio
- missing required input reduces utilization
- wrong-resource input produces zero useful output by default
- byproducts are produced at the same utilization as the parent recipe
- hard invalid graph structure is blocked or reported separately
- soft domain failure is evaluated and reported, not blocked by the editor

The evaluator should report:

- scenario completion
- resource production rates
- resource consumption rates
- deficits
- surplus
- unmanaged byproducts
- waste rate
- power use
- bottlenecks
- starved nodes
- zero-output consequences
- player-facing messages

## 21. Objective System

Objectives should be typed, not only text. Initial objective kinds should include:

- produce at least a resource rate
- handle all produced byproduct
- stay below a waste rate
- stay below a power limit
- stay below a machine count

Later objective kinds can include route constraints, discovered recipes, graph patterns, unlocked capabilities, and prohibited technology paths.

## 22. Tech Progression Structure

Technology progression should unlock capability space. It should support long chains, alternate routes, and byproduct reuse.

A technology or milestone should be able to unlock:

- machines
- recipes
- resource categories
- UI information
- scenario objectives
- alternate routes
- capability tags

The progression model should not assume a single linear campaign.

## 23. First Content Slice

The first content slice should demonstrate a minimal but real production chain:

```text
Iron Ore Source -> Crusher -> Washer -> Smelter -> Iron Ingot Target
                         |
                         v
                    Iron Slurry -> Waste Sink
```

The slice should prove:

- graph construction
- recipe-driven port generation
- resource flow
- missing input behavior
- byproduct handling
- objective completion
- soft failure messaging
- save/load readiness for graph meaning

## 24. Slice 1 Valid Routes

Slice 1 should include one primary route and room for one or more alternate routes later.

The baseline route is:

```text
Iron Ore -> Crushed Iron Ore -> Washed Iron Ore -> Iron Ingot
```

Byproduct responsibility:

```text
Washer -> Iron Slurry -> Waste Sink
```

The first milestone should require Iron Ingot production and byproduct accountability.

## 25. User Interface Direction

The user interface should make the graph readable and explain evaluator consequences.

Primary UI regions:

| Region | Purpose |
| --- | --- |
| Top bar | Current milestone, unlock state, objective progress |
| Left panel | Available machines, recipes, and tech/progression browser |
| Center canvas | Production graph |
| Right panel | Selected node, recipe, port, link, and diagnostic details |
| Bottom/debug region | Evaluator messages, warnings, event log, and developer diagnostics |

The UI must communicate why a graph fails without over-blocking player actions.

## 26. Save and Load Direction

Save/load must preserve graph structure and Crushline meaning.

WNG should own graph serialization for reusable graph structure and opaque host references.

Crushline should own game save data, including production metadata, current scenario, unlocks, discoveries, and player progress.

The save model should avoid embedding exact Crushline production semantics inside WNG structures except through opaque references.

## 27. Diagnostics and Authoring Tools

Crushline needs diagnostics for both players and developers.

Useful diagnostics include:

- invalid structural links
- soft failure links
- zero-output nodes
- starved nodes
- bottlenecks
- unmanaged byproducts
- waste surplus
- power deficits
- objective status
- recipe compatibility information
- missing unlock/capability information

Authoring tools should eventually validate catalogs, recipes, milestones, and scenario objective composition.

## 28. Non-Goals for Early Implementation

Early implementation should not attempt:

- full factory-scale simulation
- arbitrary scripting
- external data authoring pipelines
- multiplayer
- complex cycles/feedback loops
- advanced logistics automation
- full tech tree UI
- procedural campaign generation
- final art polish

The first playable slice should prove the production graph loop with narrow, testable systems.

## 29. WNG Requirements Driven by Crushline

The separate WNG requirement register is `docs/wng-crushline-requirements.md`.

That document records WNG requirements in a reviewable form so they can be accepted, implemented, deferred, or rejected independently from Crushline game design.

## 30. Explicit Non-WNG Responsibilities

WNG must not own:

- Crushline resource IDs
- machine definitions
- recipe definitions
- tech progression
- scenario objectives
- production throughput evaluation
- wrong-resource outcome rules
- player-facing production text
- save-game progression meaning
- external JSON/script execution

These belong to Crushline or the host application using WNG.

## 31. WPL Feature Requests

WPL may need support for stable window/input/timing behavior, draw command submission, canvas transforms, debug overlay support, file I/O, and replay infrastructure.

WPL draw-list capacity failure must be treated as a recoverable rendering failure, not a simulation failure.

## 32. Implementation Roadmap

Recommended order:

1. Keep WPL v0.1 foundation stable.
2. Accept the revised WNG v0.1 requirements.
3. Build WNG graph core.
4. Add WNG port side/direction and host references.
5. Add WNG serialization.
6. Add WNG rendering/editor interaction.
7. Keep Crushline shell thin until WNG editor behavior exists.
8. Build Crushline production model and evaluator against accepted WNG contracts.
9. Add first playable content slice.
10. Expand progression and alternate routes only after Slice 1 is stable.

## 33. Review Questions

Use these questions when reviewing future work:

- Does the change belong in Crushline, WNG, or WPL?
- Does the PR mix game meaning with reusable graph structure?
- Does the PR block a connection that should be a soft failure?
- Does the evaluator explain consequences clearly?
- Does the change preserve room for multiple routes?
- Does the change make byproducts meaningful?
- Does the change keep the first playable slice narrow?
- Does Crushline depend on a WNG requirement that has not yet been accepted?

## 34. Final Design Statement

Crushline is a graph-first production game. The graph is not decoration; it is the factory. The player should be free to build, fail, learn, and optimize inside a permissive node graph where the evaluator explains consequences instead of the editor over-policing game-domain mistakes.

The implementation should stay disciplined: WNG owns reusable graph/editor behavior, WPL owns platform behavior, and Crushline owns game meaning.
