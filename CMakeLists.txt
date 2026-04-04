cmake_minimum_required(VERSION 3.16)
project(kanban-cli C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find ncursesw package
find_package(Curses REQUIRED)

include_directories(${CURSES_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.c
    src/models.c
    src/storage.c
)

# Create executable
add_executable(kanban-cli ${SOURCES})

# Link ncursesw library
target_link_libraries(kanban-cli ${CURSES_LIBRARIES})

# Add compile options for warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(kanban-cli PRIVATE -Wall -Wextra -pedantic)
endif()