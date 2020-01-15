cmake_minimum_required(VERSION 3.1)
project(Glacius)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_subdirectory(dependencies/libcfx2)
add_subdirectory(dependencies/littl)

set(SRC
        src/Config.cpp
        src/Config.hpp
        src/Database.cpp
        src/Database.hpp
        src/Glacius.cpp
        src/LoginServer.cpp
        src/LoginServer.hpp
        src/StatusServer.cpp
        src/StatusServer.hpp
        src/Util.hpp
        src/WorldServer.cpp
        src/WorldServer.hpp

        3rdparty/sqlite3.c
        )

add_executable(${PROJECT_NAME} ${SRC})

target_compile_definitions(${PROJECT_NAME} PRIVATE -DSQLITE_OMIT_LOAD_EXTENSION=1)
target_include_directories(${PROJECT_NAME} PUBLIC include)

# libcfx2-0.8
add_dependencies(${PROJECT_NAME} cfx2_08)
target_link_libraries(${PROJECT_NAME} PUBLIC cfx2_08)

# littl
add_dependencies(${PROJECT_NAME} littl)
target_link_libraries(${PROJECT_NAME} PRIVATE littl)

## Glacius Remote Control Console

add_executable(GlaciusRCC RCC/src/main.cpp)

add_dependencies(GlaciusRCC littl)
target_link_libraries(GlaciusRCC PRIVATE littl)
