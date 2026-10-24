#include "writer_session.h"
#include "candidate_artifact_draft_model.h"
#include "contradiction_budget_api.h"
#include "evidence_potential_api.h"
#include "evidence_potential_model.h"
#include <sstream>
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

    result.accepted = false;
    result.text = "unknown command: " + cmd + "  (try help)\n";
    return result;
}

} // namespace archive
