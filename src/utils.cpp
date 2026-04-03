#include "utils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Utils {
    std::map<std::string, std::string> loadEnv(const std::string& filename) {
        std::map<std::string, std::string> envVars;
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);
                
                // Remove potential surrounding quotes
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                } else if (value.size() >= 2 && value.front() == '\'' && value.back() == '\'') {
                    value = value.substr(1, value.size() - 2);
                }
                
                envVars[key] = value;
            }
        }
        return envVars;
    }

    std::string slugify(const std::string& text) {
        std::string result = text;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        for (char& c : result) {
            if (c == ' ' || c == '/') c = '_';
            else if (!isalnum(c) && c != '_') c = '-';
        }
        return result;
    }

    void writeToFile(const std::string& filename, const std::string& content) {
        std::ofstream file(filename, std::ios::app); // Append mode
        if (file.is_open()) {
            file << content << "\n";
            file.close();
        }
    }
}
