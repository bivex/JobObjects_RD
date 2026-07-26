cmake_minimum_required(VERSION 3.20)
project(AgentJobEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Explicit output directories for Visual Studio & build outputs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra)
endif()

# AgentJobEngine Static Library
add_library(AgentJobEngine STATIC src/AgentJobEngine.cpp include/AgentJobEngine.hpp)
target_include_directories(AgentJobEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Integrated Validation PoC Executable
add_executable(AgentJobObject_Test tests/AgentJobObject_Test.cpp)

if(WIN32)
    target_link_libraries(AgentJobEngine PUBLIC kernel32 user32 advapi32 psapi)
    target_link_libraries(AgentJobObject_Test PRIVATE AgentJobEngine kernel32 user32 advapi32 psapi)
endif()

# Installation rules
install(TARGETS AgentJobEngine AgentJobObject_Test DESTINATION bin)
