cmake_minimum_required(VERSION 3.20)
project(AgentJobEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra)
endif()

# AgentJobEngine Library
add_library(AgentJobEngine STATIC AgentJobEngine.cpp AgentJobEngine.hpp)
target_include_directories(AgentJobEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Validation PoC Executable
add_executable(AgentJobObject_Test AgentJobObject_Test.cpp)

if(WIN32)
    target_link_libraries(AgentJobEngine PUBLIC kernel32 user32 advapi32 psapi)
    target_link_libraries(AgentJobObject_Test PRIVATE AgentJobEngine kernel32 user32 advapi32 psapi)
endif()

install(TARGETS AgentJobEngine AgentJobObject_Test DESTINATION bin)
