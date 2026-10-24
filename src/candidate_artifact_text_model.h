#pragma once
#include "candidate_artifact_draft_model.h"
#include <string>
#include <vector>

namespace archive {

// Advisory text produced from a draft. Never inserts into PublicArchive.
struct CandidateArtifactText {
    std::string id;
    std::string source_draft_id;
    std::string title;
    int intended_year = 0;
    std::string backend;          // "template" or "external"
    std::string text;
    std::vector<std::string> constraints;
    bool artifact_insertion_enabled = false;
    bool public_claim_insertion_enabled = false;
    bool discovery_scheduling_enabled = false;
    bool public_archive_mutation_enabled = false;
};

} // namespace archive
