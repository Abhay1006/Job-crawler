#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include "utils.hpp"
#include "JobCrawler.hpp"
#include "AIAgent.hpp"

using namespace std;

int main() {
    cout << "=========================================\n";
    cout << "      AI-Powered C++ Job Crawler         \n";
    cout << "=========================================\n\n";

    // Load API keys
    auto envVars = Utils::loadEnv(".env");
    if (envVars.find("SEARCH_API_KEY") == envVars.end() || envVars.find("GEMINI_API_KEY") == envVars.end() || envVars["SEARCH_API_KEY"].find("placeholder") != string::npos || envVars["GEMINI_API_KEY"].find("placeholder") != string::npos) {
        cerr << "Error: SEARCH_API_KEY and GEMINI_API_KEY must be properly set in the .env file.\n";
        return 1;
    }

    string searchApiKey = envVars["SEARCH_API_KEY"];
    string geminiApiKey = envVars["GEMINI_API_KEY"];

    // Gather user inputs
    string jobTitle, experience, skills, frameworks, maxJobsStr, delayStr, timeLimitStr;
    
    cout << "Enter target job title (e.g., 'SDE 1 Google'): ";
    getline(cin, jobTitle);

    cout << "Enter years of experience (e.g., '1-3 years'): ";
    getline(cin, experience);

    cout << "Enter core skills (e.g., 'C++, Python'): ";
    getline(cin, skills);

    cout << "Enter preferred frameworks/tools: ";
    getline(cin, frameworks);

    cout << "Enter max number of jobs to analyze (default 20): ";
    getline(cin, maxJobsStr);
    int maxJobs = maxJobsStr.empty() ? 20 : stoi(maxJobsStr);

    cout << "Enter delay between AI calls in seconds (default 12): ";
    getline(cin, delayStr);
    int delaySeconds = delayStr.empty() ? 12 : stoi(delayStr);

    cout << "Enter total time limit for analysis in minutes (default 10): ";
    getline(cin, timeLimitStr);
    int timeLimitMinutes = timeLimitStr.empty() ? 10 : stoi(timeLimitStr);

    string criteria = "Target Job Title: " + jobTitle + "\n"
                         + "Required Experience: " + experience + "\n"
                         + "Skills: " + skills + "\n"
                         + "Frameworks: " + frameworks + "\n";

    AIAgent agent(geminiApiKey);
    JobCrawler crawler(searchApiKey);

    cout << "\n[1/4] Expanding search queries using AI...\n";
    vector<string> expandedQueries = agent.generateSearchQueries(jobTitle, skills);
    
    cout << "AI generated queries:\n";
    for (const auto& q : expandedQueries) cout << " - " << q << "\n";

    cout << "\n[2/4] Searching multiple sources (this may take a minute)...\n";
    vector<JobPosting> allJobs;
    set<string> seenUrls;

    for (const auto& query : expandedQueries) {
        cout << "Searching for: " << query << "...\n";
        auto results = crawler.searchJobs(query);
        for (auto& job : results) {
            if (seenUrls.find(job.url) == seenUrls.end()) {
                allJobs.push_back(job);
                seenUrls.insert(job.url);
            }
        }
        // Small delay between searches to be nice to SerpApi
        this_thread::sleep_for(chrono::seconds(1));
    }

    if (allJobs.empty()) {
        cout << "No job postings found.\n";
        return 0;
    }

    cout << "\n[3/4] Found " << allJobs.size() << " unique job postings. Analyzing with AI filtering...\n";
    if (allJobs.size() > (size_t)maxJobs) {
        cout << "Limiting analysis to the first " << maxJobs << " jobs as requested.\n";
        allJobs.resize(maxJobs);
    }
    
    string outputFilename = Utils::slugify(jobTitle) + "_jobs.txt";
    int matchCount = 0;
    auto startTime = chrono::steady_clock::now();

    for (size_t i = 0; i < allJobs.size(); ++i) {
        // Check total time limit
        auto currentTime = chrono::steady_clock::now();
        auto elapsedMinutes = chrono::duration_cast<chrono::minutes>(currentTime - startTime).count();
        if (elapsedMinutes >= timeLimitMinutes) {
            cout << "\n[TIME LIMIT REACHED] Analysis stopped after " << elapsedMinutes << " minutes.\n";
            break;
        }

        cout << "Analyzing " << (i+1) << "/" << allJobs.size() << ": " << allJobs[i].title << " @ " << allJobs[i].company << "... " << flush;
        
        AIResponse aiRes = agent.evaluateJob(allJobs[i].snippet, criteria);
        
        if (aiRes.quotaExceeded) {
            cout << "[QUOTA EXCEEDED]\n";
            cerr << "\nAPI Quota reached. Skipping remaining AI analyses to avoid further blocks.\n";
            break; 
        }

        if (aiRes.isMatch) {
            cout << "[MATCH]\n";
            matchCount++;
            string logEntry = "----------------------------------------\n";
            logEntry += "Title: " + allJobs[i].title + "\n";
            logEntry += "Company: " + allJobs[i].company + "\n";
            logEntry += "Job Application Link: " + allJobs[i].url + "\n";
            logEntry += "AI Summary: " + aiRes.summary + "\n";
            Utils::writeToFile(outputFilename, logEntry);
        } else {
            cout << "[SKIP]\n";
        }

        // Delay to respect Gemini rate limits
        if (i < allJobs.size() - 1) {
            this_thread::sleep_for(chrono::seconds(delaySeconds));
        }
    }

    cout << "\n[4/4] Done! Found " << matchCount << " matching jobs.\n";
    cout << "Results saved to: " << outputFilename << "\n";

    return 0;
}
