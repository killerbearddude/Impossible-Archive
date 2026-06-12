/*
 * Thin executable entry point. Keep engine behavior outside main().
 *
 * v14.2 note: comments in this file are documentation only and should not
 * change runtime behavior. Preserve the existing tests when extending this
 * subsystem in future versions.
 */
#include "impossible_archive.h"

int main(int argc, char** argv) {
    try {
        return archive::run_cli(archive::make_cli_args(argc, argv));
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n\n";
        std::cerr << archive::usage(argc > 0 ? argv[0] : "impossible_archive_mvp");
        return EXIT_FAILURE;
    }
}
