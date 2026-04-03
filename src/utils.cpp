#include "utils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

namespace Utils {
    map<string, string> loadEnv(const string& filename) {
        map<string, string> envVars;
        ifstream file(filename);
        string line;
        while (getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto delimiterPos = line.find('=');
            if (delimiterPos != string::npos) {
                string key = line.substr(0, delimiterPos);
                string value = line.substr(delimiterPos + 1);
                
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

    string slugify(const string& text) {
        string result = text;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        for (char& c : result) {
            if (c == ' ' || c == '/') c = '_';
            else if (!isalnum(c) && c != '_') c = '-';
        }
        return result;
    }

    void writeToFile(const string& filename, const string& content) {
        ofstream file(filename, ios::app); // Append mode
        if (file.is_open()) {
            file << content << "\n";
            file.close();
        }
    }
}
