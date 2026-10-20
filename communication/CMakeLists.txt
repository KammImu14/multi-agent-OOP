cmake_minimum_required(VERSION 3.14)
project(AgentCommunication LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/W4 /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

# ---- Communication module library (Observer Pattern) ----
add_library(communication
  src/Message.cpp
  src/TopicMatcher.cpp
  src/MessageBus.cpp
  src/CommunicatingAgent.cpp
  src/LLMAgent.cpp
)
target_include_directories(communication PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(communication PUBLIC Threads::Threads)

# ---- Demo ----
add_executable(comm_demo demo/main.cpp)
target_link_libraries(comm_demo PRIVATE communication)

# ---- Unit test ----
enable_testing()
add_executable(comm_tests tests/test_communication.cpp)
target_link_libraries(comm_tests PRIVATE communication)
add_test(NAME communication_tests COMMAND comm_tests)
