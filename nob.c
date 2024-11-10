#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "./src/nob.h"

// Stolen from https://github.com/tsoding/musializer
typedef enum {
    TARGET_LINUX,
    TARGET_WIN64_MINGW,
    COUNT_TARGETS
} Target;

static_assert(2 == COUNT_TARGETS, "Amount of targets have changed");
const char *targetNames[] = {
    [TARGET_LINUX]       = "linux",
    [TARGET_WIN64_MINGW] = "win64-mingw",
};

void logAvailableTargets(Log_Level level) {
    nob_log(level, "Available targets:");
    for (size_t i = 0; i < COUNT_TARGETS; ++i) {
        nob_log(level, "    %s", targetNames[i]);
    }
}
////

bool parseTarget(const char* value, Target* target) {
    bool found = false;
    for (size_t i = 0; !found && i < COUNT_TARGETS; ++i) {
        if (strcmp(targetNames[i], value) == 0) {
            *target = i;
            found = true;
        }
    }

    if (!found) {
        nob_log(ERROR, "Unknown target %s", value);
        logAvailableTargets(ERROR);
        return false;
    }

    return true;
}

static char* sourceFiles[] = {
    "dactylichexameter.c",
    "main.c",
};

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char* program = shift(argv, argc);

    if (!mkdir_if_not_exists("./build")) return 1;

#ifdef _WIN32
    Target target = TARGET_WIN64_MINGW;
#else
    Target target = TARGET_LINUX;
#endif
    if (argc > 0) {
        const char* subcommand = shift(argv, argc);
        if (strcmp(subcommand, "--help") == 0 || strcmp(subcommand, "-h") == 0 || strcmp(subcommand, "help") == 0) {
            nob_log(INFO, "Usage: %s [target]");
            logAvailableTargets(INFO);
        }

        if (!parseTarget(subcommand, &target)) return 1;
    }

    Cmd cmd = {0};
    if (target == TARGET_WIN64_MINGW)
        cmd_append(&cmd, "x86_64-w64-mingw32-gcc");
    else
        cmd_append(&cmd, "gcc");
    cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");
    cmd_append(&cmd, "-o", "./build/main");
    for (size_t i = 0; i < ARRAY_LEN(sourceFiles); ++i) {
        cmd_append(&cmd, nob_temp_sprintf("./src/%s", sourceFiles[i]));
    }

    if (target == TARGET_WIN64_MINGW)
        cmd_append(&cmd, "-static");


    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    return 0;
}