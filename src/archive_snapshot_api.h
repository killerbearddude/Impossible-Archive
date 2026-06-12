#pragma once
#include "archive_engine_state.h"
#include "archive_snapshot_model.h"

namespace archive {

[[nodiscard]] ArchiveSnapshot build_archive_snapshot(
    const ArchiveEngineState& state,
    const std::string& source_fixture_id,
    std::uint64_t fixture_seed,
    int fixture_archive_year,
    int effective_archive_year
);

[[nodiscard]] std::string format_archive_snapshot(const ArchiveSnapshot& snapshot);

[[nodiscard]] ArchiveSnapshotComparison compare_archive_snapshots(
    const ArchiveSnapshot& before,
    const ArchiveSnapshot& after
);

[[nodiscard]] std::string format_archive_snapshot_comparison(
    const ArchiveSnapshot& before,
    const ArchiveSnapshot& after
);

} // namespace archive
