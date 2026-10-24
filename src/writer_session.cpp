#include "writer_session.h"
#include "candidate_artifact_draft_model.h"
#include "contradiction_budget_api.h"
#include "evidence_potential_api.h"
#include "evidence_potential_model.h"
#include <sstream>
#include <cstdio>
#include <array>
#include <cctype>

namespace archive {
namespace {

std::string trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
    return std::string(s);
}

std::vector<std::string> split_ws(std::string_view s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::string access_name(AccessLevel a) {
    return to_string(a);
}

bool parse_access(std::string_view text, AccessLevel& out) {
    if (text == "public") { out = AccessLevel::Public; return true; }
    if (text == "scholar") { out = AccessLevel::Scholar; return true; }
    if (text == "curator") { out = AccessLevel::Curator; return true; }
    if (text == "canon") { out = AccessLevel::Canon; return true; }
    if (text == "debug") { out = AccessLevel::Debug; return true; }
    return false;
}

std::string status_text(const WriterSessionContext& ctx, const ArchiveEngineState& state) {
    std::ostringstream out;
    out << (ctx.display_name.empty() ? ctx.civilization_id : ctx.display_name);
    if (!ctx.civilization_id.empty() && ctx.display_name != ctx.civilization_id) {
        out << " (" << ctx.civilization_id << ")";
    }
    out << "\n";
    out << "access: " << access_name(ctx.access);
    if (ctx.archive_year_set) out << "  year: " << ctx.archive_year;
    if (!ctx.selected_id.empty()) out << "  selected: " << ctx.selected_id;
    out << "\n";

    // Evidence potentials
    std::size_t ep_total = state.evidence_potentials.size();
    std::size_t material = 0, ledger = 0, legal = 0, ritual = 0, other = 0;
    for (const auto& ep : state.evidence_potentials) {
        switch (ep.trace_type) {
            case EvidencePotentialTraceType::MaterialDepositTrace: ++material; break;
            case EvidencePotentialTraceType::LedgerTrace: ++ledger; break;
            case EvidencePotentialTraceType::LegalRecordTrace: ++legal; break;
            case EvidencePotentialTraceType::RitualTrace: ++ritual; break;
            default: ++other; break;
        }
    }
    out << "evidence potentials: " << ep_total
        << "  (material " << material << ", ledger " << ledger
        << ", legal " << legal << ", ritual " << ritual;
    if (other) out << ", other " << other;
    out << ")\n";

    // Drafts by status
    std::size_t need_review = 0, blocked = 0, ready = 0, other_d = 0;
    for (const auto& d : state.candidate_artifact_drafts) {
        switch (d.status) {
            case CandidateArtifactDraftStatus::NeedsCuratorReview: ++need_review; break;
            case CandidateArtifactDraftStatus::Blocked: ++blocked; break;
            case CandidateArtifactDraftStatus::ReadyForOutline: ++ready; break;
            default: ++other_d; break;
        }
    }
    out << "drafts: " << state.candidate_artifact_drafts.size()
        << "  (needs_curator_review " << need_review
        << ", blocked " << blocked
        << ", ready " << ready;
    if (other_d) out << ", other " << other_d;
    out << ")\n";

    // Contradiction budget one-liner via existing formatter (first status line)
    const std::string budget = format_contradiction_budget_summary(state, ctx.access);
    // Extract status if present
    auto pos = budget.find("status:");
    if (pos != std::string::npos) {
        auto end = budget.find('\n', pos);
        out << "contradiction budget: " << budget.substr(pos, end == std::string::npos ? std::string::npos : end - pos) << "\n";
    } else {
        out << "contradiction budget: (see pressures)\n";
    }
    return out.str();
}

std::string pressures_text(const WriterSessionContext& ctx, const ArchiveEngineState& state) {
    std::ostringstream out;
    out << format_contradiction_budget_summary(state, ctx.access);
    out << "\n";
    out << format_evidence_potential_summary(state, ctx.access);
    return out.str();
}

std::string drafts_text(const WriterSessionContext& ctx, const ArchiveEngineState& state) {
    std::ostringstream out;
    if (state.candidate_artifact_drafts.empty()) {
        out << "no drafts\n";
        return out.str();
    }
    // Public access gets only a count; detail requires curator+
    if (ctx.access == AccessLevel::Public || ctx.access == AccessLevel::Scholar) {
        out << "drafts: " << state.candidate_artifact_drafts.size()
            << " (detail requires curator or higher)\n";
        return out.str();
    }
    std::size_t shown = 0;
    for (const auto& d : state.candidate_artifact_drafts) {
        out << d.id << "  " << d.outline_title
            << "  " << to_string(d.status) << "\n";
        if (++shown >= 40) {
            out << "... (" << (state.candidate_artifact_drafts.size() - shown) << " more)\n";
            break;
        }
    }
    return out.str();
}

std::string show_text(const WriterSessionContext& ctx, const ArchiveEngineState& state, const std::string& id) {
    if (id.empty()) return "show: missing id (or select one first)\n";
    // Draft
    for (const auto& d : state.candidate_artifact_drafts) {
        if (d.id == id) {
            if (ctx.access == AccessLevel::Public || ctx.access == AccessLevel::Scholar) {
                return "draft detail restricted at this access level\n";
            }
            std::ostringstream out;
            out << "draft: " << d.id << "\n";
            out << "title: " << d.outline_title << "\n";
            out << "status: " << to_string(d.status) << "\n";
            out << "topic: " << d.target_topic << "\n";
            out << "intended_year: " << d.intended_creation_year << "\n";
            if (!d.claim_outline_lines.empty()) {
                out << "claim outline:\n";
                for (const auto& line : d.claim_outline_lines) out << "  - " << line << "\n";
            }
            if (!d.required_validation_gates.empty()) {
                out << "gates: ";
                for (std::size_t i = 0; i < d.required_validation_gates.size(); ++i) {
                    if (i) out << ", ";
                    out << d.required_validation_gates[i];
                }
                out << "\n";
            }
            return out.str();
        }
    }
    return "show: id not found among drafts (" + id + ")\n";
}

std::string report_text(const WriterSessionContext& ctx, const ArchiveEngineState& state) {
    std::ostringstream out;
    out << "=== writer report ===\n";
    out << status_text(ctx, state);
    out << "\n--- drafts needing attention ---\n";
    out << drafts_text(ctx, state);
    return out.str();
}


std::string expand_template(const CandidateArtifactDraft& draft) {
    std::ostringstream out;
    out << draft.outline_title << "\n\n";
    if (!draft.target_topic.empty()) out << "Subject: " << draft.target_topic << "\n";
    out << "Recorded in year " << draft.intended_creation_year << ".\n\n";
    if (!draft.claim_outline_lines.empty()) {
        out << "Claims preserved in outline form:\n";
        for (const auto& line : draft.claim_outline_lines) out << "  • " << line << "\n";
        out << "\n";
    }
    if (!draft.public_safe_summary_lines.empty()) {
        out << "Public-safe notes:\n";
        for (const auto& line : draft.public_safe_summary_lines) out << "  • " << line << "\n";
        out << "\n";
    }
    if (draft.touches_protected_mystery) out << "[Protected mystery pressure present; detail remains gated.]\n";
    out << "[Template expansion. Advisory only. Source draft: " << draft.id << "]\n";
    return out.str();
}

std::string expand_external(const CandidateArtifactDraft& draft, const WriterSessionContext& ctx) {
    if (ctx.external_command.empty()) {
        return "[external backend selected but no command configured; falling back]\n" + expand_template(draft);
    }

    std::string prompt = draft.outline_title + "\n";
    if (!draft.target_topic.empty()) prompt += "Subject: " + draft.target_topic + "\n";
    prompt += "Year: " + std::to_string(draft.intended_creation_year) + "\n";
    for (const auto& line : draft.claim_outline_lines) prompt += line + "\n";
    if (draft.touches_protected_mystery) prompt += "[protected mystery pressure]\n";

    // Invoke the user command as a filter: prompt on stdin, capture stdout.
    // The command string is used as-is; the caller is responsible for a safe command.
    std::string invoke = ctx.external_command;
    FILE* pipe = popen(invoke.c_str(), "w");
    if (!pipe) {
        return "[external backend failed to start: " + ctx.external_command + "]\n" + expand_template(draft);
    }
    // popen("w") feeds stdin but does not capture stdout.
    // Use a shell wrapper that both accepts stdin and lets us read stdout.
    pclose(pipe);

    // Portable filter: sh -c 'command' with prompt via a here-document.
    std::string shell = "sh -c '" + ctx.external_command + "' <<'IA_EOF'\n" + prompt + "IA_EOF\n";
    FILE* out = popen(shell.c_str(), "r");
    if (!out) {
        return "[external backend failed to launch shell]\n" + expand_template(draft);
    }
    std::string captured;
    std::array<char, 4096> buf{};
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), out)) {
        captured.append(buf.data());
    }
    int rc = pclose(out);
    if (captured.empty()) {
        return "[external backend produced no output (rc=" + std::to_string(rc) + "); falling back]\n" + expand_template(draft);
    }
    return captured;
}

std::string generate_text_from_draft(WriterSessionContext& ctx, const ArchiveEngineState& state, const std::string& id) {
    if (ctx.access != AccessLevel::Curator && ctx.access != AccessLevel::Canon && ctx.access != AccessLevel::Debug) {
        return "generate: requires curator, canon, or debug access\n";
    }
    const CandidateArtifactDraft* draft = nullptr;
    for (const auto& d : state.candidate_artifact_drafts) {
        if (d.id == id) { draft = &d; break; }
    }
    if (!draft) return "generate: draft not found (" + id + ")\n";
    if (draft->status == CandidateArtifactDraftStatus::Blocked || draft->status == CandidateArtifactDraftStatus::Invalid) {
        return "generate: draft status does not allow generation (" + to_string(draft->status) + ")\n";
    }

    CandidateArtifactText rec;
    rec.id = "candidate_artifact_text." + std::to_string(ctx.generated_texts.size());
    rec.source_draft_id = draft->id;
    rec.title = draft->outline_title;
    rec.intended_year = draft->intended_creation_year;
    rec.backend = (ctx.text_backend == "external") ? "external" : "template";
    rec.constraints = draft->required_validation_gates;
    rec.text = (rec.backend == "external") ? expand_external(*draft, ctx) : expand_template(*draft);
    ctx.generated_texts.push_back(rec);

    std::ostringstream out;
    out << "=== generated artifact text (advisory record " << rec.id << ") ===\n";
    out << "source_draft: " << rec.source_draft_id << "\n";
    out << "backend: " << rec.backend << "\n";
    out << "title: " << rec.title << "\n";
    out << "intended_year: " << rec.intended_year << "\n";
    out << "stored: session-local (not in PublicArchive)\n\n";
    out << rec.text;
    return out.str();
}

} // namespace

WriterSessionContext make_writer_session_context(const ArchiveEngineState& state, AccessLevel access) {
    WriterSessionContext ctx;
    ctx.access = access;
    if (state.civilization_source) {
        ctx.civilization_id = state.civilization_source->civilization_id;
        ctx.display_name = state.civilization_source->display_name;
    } else {
        ctx.civilization_id = "fixed-fixture";
        ctx.display_name = "fixed fixture";
    }
    return ctx;
}

std::string format_writer_prompt(const WriterSessionContext& context) {
    std::ostringstream out;
    out << (context.civilization_id.empty() ? "?" : context.civilization_id)
        << " [" << access_name(context.access) << "]";
    if (!context.selected_id.empty()) {
        // short id tail
        auto pos = context.selected_id.rfind('.');
        out << " sel:" << (pos == std::string::npos ? context.selected_id : context.selected_id.substr(pos + 1));
    }
    out << " > ";
    return out.str();
}

std::string format_writer_help() {
    return
        "commands:\n"
        "  status                  civilization, access, evidence counts, draft counts, budget\n"
        "  pressures               contradiction budget + evidence potential summary\n"
        "  drafts                  list drafts (curator+ for titles; public sees count only)\n"
        "  show <id>               detail for one draft (uses selected id if omitted)\n"
        "  select <id>             set current selection\n"
        "  report                  compact writer report (status + drafts)\n"
        "  access <public|scholar|curator|canon|debug>\n"
        "  year <n>|clear          set or clear archive-year filter\n"
        "  help                    this list\n"
        "  generate [<id>]         advisory text from a draft (curator+; stores session record)\n"
        "  texts                   list generated text records in this session\n"
        "  backend [template|external [cmd]]  select text backend\n"
        "  raw <existing-query>    escape hatch to current CLI query syntax\n"
        "  end                     end session\n"
        "\n"
        "This REPL is a front-end only. It does not mutate the archive.\n"
        "A future GUI can call the same interpreter.\n";
}

WriterCommandResult interpret_writer_command(const WriterSessionContext& context,
                                             std::string_view line,
                                             const ArchiveEngineState& state) {
    WriterCommandResult result;
    result.context = context;
    const std::string trimmed = trim(line);
    if (trimmed.empty()) {
        result.accepted = true;
        return result;
    }
    const auto tokens = split_ws(trimmed);
    const std::string& cmd = tokens[0];

    if (cmd == "end" || cmd == "quit" || cmd == "exit" || cmd == "end-session") {
        result.should_end = true;
        result.text = "session ended\n";
        return result;
    }
    if (cmd == "help" || cmd == "?") {
        result.text = format_writer_help();
        return result;
    }
    if (cmd == "status") {
        result.text = status_text(context, state);
        return result;
    }
    if (cmd == "pressures") {
        result.text = pressures_text(context, state);
        return result;
    }
    if (cmd == "drafts") {
        result.text = drafts_text(context, state);
        return result;
    }
    if (cmd == "report") {
        result.text = report_text(context, state);
        return result;
    }
    if (cmd == "show") {
        std::string id = tokens.size() > 1 ? tokens[1] : context.selected_id;
        result.text = show_text(context, state, id);
        return result;
    }
    if (cmd == "select") {
        if (tokens.size() < 2) {
            result.accepted = false;
            result.text = "select: missing id\n";
            return result;
        }
        result.context.selected_id = tokens[1];
        result.text = "selected: " + result.context.selected_id + "\n";
        return result;
    }
    if (cmd == "access") {
        if (tokens.size() < 2) {
            result.accepted = false;
            result.text = "access: missing level\n";
            return result;
        }
        AccessLevel a;
        if (!parse_access(tokens[1], a)) {
            result.accepted = false;
            result.text = "access: unknown level (public|scholar|curator|canon|debug)\n";
            return result;
        }
        result.context.access = a;
        result.text = "access: " + access_name(a) + "\n";
        return result;
    }
    if (cmd == "year") {
        if (tokens.size() < 2) {
            result.accepted = false;
            result.text = "year: missing n or 'clear'\n";
            return result;
        }
        if (tokens[1] == "clear") {
            result.context.archive_year_set = false;
            result.context.archive_year = kOpenEndedYear;
            result.text = "year cleared\n";
            return result;
        }
        try {
            result.context.archive_year = std::stoi(tokens[1]);
            result.context.archive_year_set = true;
            result.text = "year: " + std::to_string(result.context.archive_year) + "\n";
        } catch (...) {
            result.accepted = false;
            result.text = "year: invalid number\n";
        }
        return result;
    }
    if (cmd == "raw") {
        // Hand the rest of the line back to the CLI loop.
        auto pos = trimmed.find_first_of(" \t");
        result.raw_query_line = (pos == std::string::npos) ? std::string{} : trim(trimmed.substr(pos));
        if (result.raw_query_line.empty()) {
            result.accepted = false;
            result.text = "raw: missing query line\n";
        }
        return result;
    }

    if (cmd == "generate") {
        std::string id = tokens.size() > 1 ? tokens[1] : context.selected_id;
        if (id.empty()) {
            result.accepted = false;
            result.text = "generate: missing id (or select a draft first)\n";
            return result;
        }
        result.text = generate_text_from_draft(result.context, state, id);
        return result;
    }

    if (cmd == "backend") {
        if (tokens.size() < 2) {
            result.text = "backend: " + context.text_backend +
                          (context.external_command.empty() ? "" : " (" + context.external_command + ")") + "\n";
            return result;
        }
        if (tokens[1] == "template") {
            result.context.text_backend = "template";
            result.text = "backend: template\n";
            return result;
        }
        if (tokens[1] == "external") {
            result.context.text_backend = "external";
            if (tokens.size() > 2) result.context.external_command = tokens[2];
            result.text = "backend: external" +
                          (result.context.external_command.empty() ? " (no command set)" : " (" + result.context.external_command + ")") + "\n";
            return result;
        }
        result.accepted = false;
        result.text = "backend: use 'template' or 'external [command]'\n";
        return result;
    }
    if (cmd == "texts") {
        if (context.generated_texts.empty()) {
            result.text = "no generated texts in this session\n";
            return result;
        }
        std::ostringstream out;
        for (const auto& t : context.generated_texts) {
            out << t.id << "  " << t.title << "  [" << t.backend << "]  draft:" << t.source_draft_id << "\n";
        }
        result.text = out.str();
        return result;
    }

    if (cmd == "format") {
        if (tokens.size() < 2) {
            result.text = std::string("format: ") + (context.json_output ? "json" : "text") + "\n";
            return result;
        }
        if (tokens[1] == "json") {
            result.context.json_output = true;
            result.text = "format: json\n";
            return result;
        }
        if (tokens[1] == "text") {
            result.context.json_output = false;
            result.text = "format: text\n";
            return result;
        }
        result.accepted = false;
        result.text = "format: use 'json' or 'text'\n";
        return result;
    }

    result.accepted = false;
    result.text = "unknown command: " + cmd + "  (try help)\n";
    return result;
}


std::string format_writer_json(std::string_view command, const WriterCommandResult& result, const ArchiveEngineState& state) {
    std::ostringstream out;
    out << "{\"command\":\"" << command << "\""
        << ",\"accepted\":" << (result.accepted ? "true" : "false")
        << ",\"text\":\"";
    // minimal JSON string escape
    for (char c : result.text) {
        if (c == '\\' || c == '"') out << '\\' << c;
        else if (c == '\n') out << "\\n";
        else if (c == '\r') continue;
        else out << c;
    }
    out << "\"";

    // Structured extras for list-like commands
    std::string cmd(command);
    if (cmd == "drafts" || cmd.rfind("drafts", 0) == 0) {
        out << ",\"drafts\":[";
        bool first = true;
        for (const auto& d : state.candidate_artifact_drafts) {
            if (!first) out << ",";
            first = false;
            out << "{\"id\":\"" << d.id << "\",\"title\":\"" << d.outline_title
                << "\",\"status\":\"" << to_string(d.status) << "\"}";
        }
        out << "]";
    }
    if (cmd == "texts") {
        out << ",\"texts\":[";
        bool first = true;
        for (const auto& t : result.context.generated_texts) {
            if (!first) out << ",";
            first = false;
            out << "{\"id\":\"" << t.id << "\",\"title\":\"" << t.title
                << "\",\"backend\":\"" << t.backend << "\",\"source_draft_id\":\"" << t.source_draft_id << "\"}";
        }
        out << "]";
    }
    out << "}";
    return out.str();
}

} // namespace archive
