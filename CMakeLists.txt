cmake_minimum_required(VERSION 3.10)
project(JobCrawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find Curl
find_package(CURL REQUIRED)

# Include directories
include_directories(include)
include_directories(third_party)
include_directories(${CURL_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/main.cpp
    src/utils.cpp
    src/JobCrawler.cpp
    src/AIAgent.cpp
)

# Executable
add_executable(JobCrawler ${SOURCES})

# Link libraries
target_link_libraries(JobCrawler ${CURL_LIBRARIES})
