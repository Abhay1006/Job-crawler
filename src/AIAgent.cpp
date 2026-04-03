#include "AIAgent.hpp"
#include <iostream>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

AIAgent::AIAgent(const std::string& geminiApiKey) : geminiApiKey(geminiApiKey) {}

size_t AIAgent::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

AIResponse AIAgent::evaluateJob(const std::string& jobSnippet, const std::string& criteria) {
    AIResponse aiRes = {false, "Failed to analyze."};
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-3-flash-preview:generateContent?key=" + geminiApiKey;
        
        std::string prompt = "You are an AI job assistant. I am looking for a job with the following criteria:\n" + criteria + 
                             "\n\nHere is a job description snippet:\n" + jobSnippet +
                             "\n\nCRITICAL FILTERING: If the user is looking for a 'Junior' or 'SDE 1' role, you MUST REJECT roles that are "
                             "'Senior', 'Lead', 'Staff', 'Principal', 'SDE 2', 'SDE 3', 'II', 'III', or require 5+ years of experience. "
                             "Only return is_match: true if the level is exactly what the user is looking for."
                             "\n\nRespond with a JSON object exactly in this format without markdown blocks: {\"is_match\": true/false, \"summary\": \"Brief explanation of why it matches or not, and key points\"}";

        json requestJson;
        requestJson["contents"][0]["parts"][0]["text"] = prompt;
        
        std::string payload = requestJson.dump();

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L); // 60s timeout for AI
        
        // Some systems need this for HTTPS
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if(res == CURLE_OK) {
            try {
                json response = json::parse(readBuffer);
                if(response.contains("candidates") && response["candidates"].size() > 0) {
                    std::string aiText = response["candidates"][0]["content"]["parts"][0]["text"];
                    
                    // The text might be wrapped in ```json ... ``` markdown
                    size_t firstBrace = aiText.find('{');
                    size_t lastBrace = aiText.rfind('}');
                    if (firstBrace != std::string::npos && lastBrace != std::string::npos) {
                        std::string jsonStr = aiText.substr(firstBrace, lastBrace - firstBrace + 1);
                        json aiResult = json::parse(jsonStr);
                        if (aiResult.contains("is_match") && aiResult.contains("summary")) {
                            aiRes.isMatch = aiResult["is_match"];
                            aiRes.summary = aiResult["summary"];
                        }
                    } else {
                        aiRes.summary = "Could not parse JSON from AI response. Raw Text: " + aiText;
                    }
                } else if (response.contains("error")) {
                     std::string errorMsg = response["error"]["message"];
                     std::cerr << "Gemini API Error: " << errorMsg << " (Code: " << response["error"]["code"] << ")\n";
                     if (response["error"]["code"] == 429 || errorMsg.find("quota") != std::string::npos) {
                         aiRes.quotaExceeded = true;
                     }
                } else {
                    std::cerr << "Unexpected Gemini Response Format. Raw Body: " << readBuffer << '\n';
                }
            } catch (const std::exception& e) {
                std::cerr << "JSON/Parse Error in AIAgent: " << e.what() << '\n';
                std::cerr << "Raw Output: " << readBuffer << '\n';
            }
        } else {
            std::cerr << "cURL Network Error in AIAgent: " << curl_easy_strerror(res) << " (Code: " << res << ")\n";
        }
    }
    return aiRes;
}

std::vector<std::string> AIAgent::generateSearchQueries(const std::string& jobTitle, const std::string& skills) {
    std::vector<std::string> queries;
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-3-flash-preview:generateContent?key=" + geminiApiKey;
        
        std::string prompt = "Input Job Title: '" + jobTitle + "'\n" +
                             "Input Skills: '" + skills + "'\n\n" +
                             "Generate a JSON list of 5 high-quality search queries for Google Jobs. "
                             "Be broad and inclusive. If the title includes a company name (e.g., 'cred'), "
                             "ensure several queries focus on that company. Expand abbreviations (e.g., 'SDE' to 'Software Development Engineer'). "
                             "Include level-specific synonyms (e.g., 'SDE 1' -> 'Junior', 'Entry Level', 'Associate'). "
                             "Example output: {\"queries\": [\"SDE CRED\", \"Junior Software Engineer CRED\", \"Entry level jobs CRED\", \"Software Development Engineer CRED\", \"CRED careers\"]}\n"
                             "Return ONLY the JSON object.";

        json requestJson;
        requestJson["contents"][0]["parts"][0]["text"] = prompt;
        std::string payload = requestJson.dump();

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if(res == CURLE_OK) {
            try {
                json response = json::parse(readBuffer);
                if(response.contains("candidates") && response["candidates"].size() > 0) {
                    std::string aiText = response["candidates"][0]["content"]["parts"][0]["text"];
                    size_t firstBrace = aiText.find('{');
                    size_t lastBrace = aiText.rfind('}');
                    if (firstBrace != std::string::npos && lastBrace != std::string::npos) {
                        std::string jsonStr = aiText.substr(firstBrace, lastBrace - firstBrace + 1);
                        json result = json::parse(jsonStr);
                        if (result.contains("queries")) {
                            for (auto& q : result["queries"]) queries.push_back(q);
                        }
                    }
                } else if (response.contains("error")) {
                     std::cerr << "Gemini Query Expansion Error: " << response["error"]["message"] << "\n";
                }
            } catch (...) {}
        }
    }
    
    // Fallback if AI fails
    if (queries.empty()) {
        queries.push_back(jobTitle);
    }
    return queries;
}
