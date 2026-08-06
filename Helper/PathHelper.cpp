#include "Helper/PathHelper.h"

#include <algorithm>
#include <cctype>
#include <shared_mutex>
#include <system_error>

namespace fs = std::filesystem;

// prevent ../
bool containsParentDirectory(const fs::path& path) {
    for (const fs::path& component : path) {
        if (component == "..") {
            return true;
        }
    }

    return false;
}

bool resolvePathInsideHome(
    const ServerSession& session,
    const std::string& input,
    fs::path& resolvedPath
) {
    fs::path homeDir;
    fs::path currentDir;

    // prevent other thread to modified original homeDir and CurrentDir in a moment and after that we got homeDir, currentDir our own
    {
        std::shared_lock<std::shared_mutex> lock(session.sessionMutex);

        if (session.homeDir.empty()) {
            return false;
        }

        homeDir = session.homeDir;
        currentDir = session.currentDir.empty() ? fs::path(".") : session.currentDir;
    }
    // convert windows-style separators ('\') to '/' so paths are handled consistently between windows and linux
    std::string portableInput = input;
    std::replace(portableInput.begin(), portableInput.end(), '\\', '/');

    // convert the client input into a filesystem path. If no path is provided, "." represents the current directory
    const fs::path requestedPath = portableInput.empty()
        ? fs::path(".")
        : fs::path(portableInput);

    if (requestedPath.is_absolute() || containsParentDirectory(requestedPath)) {
        return false;
    }

    std::error_code error;
    const fs::path canonicalHome = fs::weakly_canonical(homeDir, error);
    if (error) {
        return false;
    }

    const fs::path target = fs::weakly_canonical(
        canonicalHome / currentDir / requestedPath,
        error
    );
    if (error) {
        return false;
    }

    const fs::path relativeTarget = target.lexically_relative(canonicalHome);
    if (relativeTarget.empty() || containsParentDirectory(relativeTarget)) {
        return false;
    }

    resolvedPath = target;
    return true;
}
