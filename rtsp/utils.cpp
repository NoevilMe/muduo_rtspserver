#include "utils.h"

namespace utils {
bool StartsWith(const std::string &str, const std::string &prefix) {
    if (str.size() < prefix.size()) {
        return false;
    } else {
        return str.compare(0, prefix.size(), prefix) == 0;
    }
}

bool EndsWith(const std::string &str, const std::string &postfix) {
    if (str.size() < postfix.size()) {
        return false;
    } else {
        return str.compare(str.size() - postfix.size(), postfix.size(),
                           postfix) == 0;
    }
}

bool Contains(const std::string &str, const std::string &substr) {
    return str.find(substr) != std::string::npos;
}

} // namespace utils