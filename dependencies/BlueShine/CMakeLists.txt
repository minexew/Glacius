cmake_minimum_required(VERSION 3.1)
project(BlueShine)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

set(SRC
        src/BlueShine.cpp
        src/DynamicArray.hpp
        src/IdHandling.hpp
        src/mysql.hpp
        src/ShineDB.cpp
        src/ShineDB.hpp
        src/ShineResult.cpp
        src/ShineResult.hpp
        )

add_library(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC include)

# MySQL client library
find_package(MySQL)
target_include_directories(${PROJECT_NAME} PRIVATE ${MYSQL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${MYSQL_LIBRARY})
