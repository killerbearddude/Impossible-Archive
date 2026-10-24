#pragma once
#include "archive_engine_state.h"
#include "candidate_artifact_text_model.h"
#include <string>
#include <string_view>

namespace archive {

// Writer-facing session context.
// This is the seam a future GUI can drive: same context fields, same command
// interpreter, different presentation.
struct WriterSessionContext {
    std::string civilization_id;
    std::string display_name;
    AccessLevel access = AccessLevel::Public;
    int archive_year = kOpenEndedYear;
    bool archive_year_set = false;
    std::string selected_id;          // optional current draft/plan/etc.
    bool writer_format = true;        // false = prefer raw formatter output
    std::vector<CandidateArtifactText> generated_texts;  // session-local advisory records
    std::string text_backend = "template";               // "template" or "external"
    std::string external_command;                        // command to run when backend=external
};

struct WriterCommandResult {
    std::string text;
    WriterSessionContext context;
    bool should_end = false;
    bool accepted = true;
    // When non-empty, the CLI loop should treat the remainder as a raw
    // existing query line (escape hatch). Empty means the interpreter
    // fully handled the command.
    std::string raw_query_line;
};

[[nodiscard]] WriterSessionContext make_writer_session_context(const ArchiveEngineState& state,
                                                              AccessLevel access);

// Interpret one line. Does not mutate ArchiveEngineState.
// Future GUI: call this (or the individual command helpers) instead of
// parsing argv.
[[nodiscard]] WriterCommandResult interpret_writer_command(const WriterSessionContext& context,
                                                           std::string_view line,
                                                           const ArchiveEngineState& state);

[[nodiscard]] std::string format_writer_prompt(const WriterSessionContext& context);
[[nodiscard]] std::string format_writer_help();

} // namespace archive
