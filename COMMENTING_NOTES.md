# Commenting Notes

v16 comments document architectural invariants rather than implementation trivia.

The most important invariants remain:

- generation is non-mutating,
- candidate evaluation is non-mutating,
- materialization is the explicit mutating boundary,
- materialization requires curator/debug authority,
- access/archive-year filtering precedes answer/theory/mystery construction,
- structured metadata and typed mediation override prose heuristics,
- structured-candidate originality scoring must include typed links and claims,
- generated candidates should carry structured metadata and claims when they are intended to be materializable.

Future comments should explain why a trust boundary exists, not restate obvious C++ syntax.
