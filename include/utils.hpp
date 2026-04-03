#pragma once
#include <string>
#include <map>

namespace Utils {
    std::map<std::string, std::string> loadEnv(const std::string& filename);
    std::string slugify(const std::string& text);
    void writeToFile(const std::string& filename, const std::string& content);
}
