#pragma once
#include <string>
#include <map>

using namespace std;

namespace Utils {
    map<string, string> loadEnv(const string& filename);
    string slugify(const string& text);
    void writeToFile(const string& filename, const string& content);
}
