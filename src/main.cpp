#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <set>
#include "utils.hpp"
#include "JobCrawler.hpp"
#include "AIAgent.hpp"

int main() {
    std::cout << "=========================================\n";
    std::cout << "      AI-Powered C++ Job Crawler         \n";
    std::cout << "=========================================\n\n";

    // Load API keys
    auto envVars = Utils::loadEnv(".env");
    if (envVars.find("SEARCH_API_KEY") == envVars.end() || envVars.find("GEMINI_API_KEY") == envVars.end() || envVars["SEARCH_API_KEY"].find("placeholder") != std::string::npos || envVars["GEMINI_API_KEY"].find("placeholder") != std::string::npos) {
        std::cerr << "Error: SEARCH_API_KEY and GEMINI_API_KEY must be properly set in the .env file.\n";
        return 1;
    }

    std::string searchApiKey = envVars["SEARCH_API_KEY"];
    std::string geminiApiKey = envVars["GEMINI_API_KEY"];

    // Gather user inputs
    std::string jobTitle, experience, skills, frameworks, maxJobsStr, delayStr, timeLimitStr;
    
    std::cout << "Enter target job title (e.g., 'SDE 1 Google'): ";
    std::getline(std::cin, jobTitle);

    std::cout << "Enter years of experience (e.g., '1-3 years'): ";
    std::getline(std::cin, experience);

    std::cout << "Enter core skills (e.g., 'C++, Python'): ";
    std::getline(std::cin, skills);

    std::cout << "Enter preferred frameworks/tools: ";
    std::getline(std::cin, frameworks);

    std::cout << "Enter max number of jobs to analyze (default 20): ";
    std::getline(std::cin, maxJobsStr);
    int maxJobs = maxJobsStr.empty() ? 20 : std::stoi(maxJobsStr);

    std::cout << "Enter delay between AI calls in seconds (default 12): ";
    std::getline(std::cin, delayStr);
    int delaySeconds = delayStr.empty() ? 12 : std::stoi(delayStr);

    std::cout << "Enter total time limit for analysis in minutes (default 10): ";
    std::getline(std::cin, timeLimitStr);
    int timeLimitMinutes = timeLimitStr.empty() ? 10 : std::stoi(timeLimitStr);

    std::string criteria = "Target Job Title: " + jobTitle + "\n"
                         + "Required Experience: " + experience + "\n"
                         + "Skills: " + skills + "\n"
                         + "Frameworks: " + frameworks + "\n";

    AIAgent agent(geminiApiKey);
    JobCrawler crawler(searchApiKey);

    std::cout << "\n[1/4] Expanding search queries using AI...\n";
    std::vector<std::string> expandedQueries = agent.generateSearchQueries(jobTitle, skills);
    
    std::cout << "AI generated queries:\n";
    for (const auto& q : expandedQueries) std::cout << " - " << q << "\n";

    std::cout << "\n[2/4] Searching multiple sources (this may take a minute)...\n";
    std::vector<JobPosting> allJobs;
    std::set<std::string> seenUrls;

    for (const auto& query : expandedQueries) {
        std::cout << "Searching for: " << query << "...\n";
        auto results = crawler.searchJobs(query);
        for (auto& job : results) {
            if (seenUrls.find(job.url) == seenUrls.end()) {
                allJobs.push_back(job);
                seenUrls.insert(job.url);
            }
        }
        // Small delay between searches to be nice to SerpApi
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (allJobs.empty()) {
        std::cout << "No job postings found.\n";
        return 0;
    }

    std::cout << "\n[3/4] Found " << allJobs.size() << " unique job postings. Analyzing with AI filtering...\n";
    if (allJobs.size() > (size_t)maxJobs) {
        std::cout << "Limiting analysis to the first " << maxJobs << " jobs as requested.\n";
        allJobs.resize(maxJobs);
    }
    
    std::string outputFilename = Utils::slugify(jobTitle) + "_jobs.txt";
    int matchCount = 0;
    auto startTime = std::chrono::steady_clock::now();

    for (size_t i = 0; i < allJobs.size(); ++i) {
        // Check total time limit
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedMinutes = std::chrono::duration_cast<std::chrono::minutes>(currentTime - startTime).count();
        if (elapsedMinutes >= timeLimitMinutes) {
            std::cout << "\n[TIME LIMIT REACHED] Analysis stopped after " << elapsedMinutes << " minutes.\n";
            break;
        }

        std::cout << "Analyzing " << (i+1) << "/" << allJobs.size() << ": " << allJobs[i].title << " @ " << allJobs[i].company << "... " << std::flush;
        
        AIResponse aiRes = agent.evaluateJob(allJobs[i].snippet, criteria);
        
        if (aiRes.quotaExceeded) {
            std::cout << "[QUOTA EXCEEDED]\n";
            std::cerr << "\nAPI Quota reached. Skipping remaining AI analyses to avoid further blocks.\n";
            break; 
        }

        if (aiRes.isMatch) {
            std::cout << "[MATCH]\n";
            matchCount++;
            std::string logEntry = "----------------------------------------\n";
            logEntry += "Title: " + allJobs[i].title + "\n";
            logEntry += "Company: " + allJobs[i].company + "\n";
            logEntry += "Job Application Link: " + allJobs[i].url + "\n";
            logEntry += "AI Summary: " + aiRes.summary + "\n";
            Utils::writeToFile(outputFilename, logEntry);
        } else {
            std::cout << "[SKIP]\n";
        }

        // Delay to respect Gemini rate limits
        if (i < allJobs.size() - 1) {
            std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));
        }
    }

    std::cout << "\n[4/4] Done! Found " << matchCount << " matching jobs.\n";
    std::cout << "Results saved to: " << outputFilename << "\n";

    return 0;
}
