cmake_minimum_required(VERSION 3.10)
project(chess VERSION 1.0)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------
#   Source Files
# ----------------------
# Grab all .cpp files inside src/
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Create executable
add_executable(chess ${SOURCES})

# ----------------------
#   SFML (LOCAL COPY)
# ----------------------
# Path: chess/SFML/include and chess/SFML/lib
include_directories(${CMAKE_SOURCE_DIR}/SFML/include)
link_directories(${CMAKE_SOURCE_DIR}/SFML/lib)

# Link SFML libraries
target_link_libraries(chess PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
)