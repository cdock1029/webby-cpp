cmake_minimum_required(VERSION 3.26.0)
project(webby VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libpqxx CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

include(CTest)

enable_testing()

file(
  GLOB_RECURSE TEMPLS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/templates/**.html)
foreach(TEMPL ${TEMPLS})
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TEMPL}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${TEMPL}
            ${CMAKE_CURRENT_BINARY_DIR}/${TEMPL}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${TEMPL})
endforeach()
list(TRANSFORM TEMPLS PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)

file(
  GLOB_RECURSE STATICS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/static/**)
foreach(STATIC ${STATICS})
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${STATIC}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${STATIC}
            ${CMAKE_CURRENT_BINARY_DIR}/${STATIC}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${STATIC})
endforeach()
list(TRANSFORM STATICS PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)

add_executable(webby main.cpp ${TEMPLS} ${STATICS})

include(cmake/CPM.cmake)

add_library(asio INTERFACE)
cpmaddpackage(NAME Crow GITHUB_REPOSITORY CrowCpp/Crow GIT_TAG "master")

target_link_libraries(webby PRIVATE asio Crow::Crow libpqxx::pqxx fmt::fmt)

target_compile_options(webby PRIVATE -Werror -Wall -Wextra)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
