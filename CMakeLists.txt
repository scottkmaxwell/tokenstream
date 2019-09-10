cmake_minimum_required(VERSION 3.16.3)
project(tokenstream)

set(CMAKE_CXX_STANDARD 14)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)
FetchContent_MakeAvailable(googletest)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)

add_library(tokenstream STATIC)

target_sources(tokenstream PRIVATE
        include/TokenStream/Generic.h
        include/TokenStream/Reader.h
        include/TokenStream/TokenStream.h
        include/TokenStream/Writer.h
        src/EndianTypes.h
        src/Generic.cpp
        src/Reader.cpp
        src/Serializable.cpp
        src/Writer.cpp)

target_include_directories(tokenstream PUBLIC include)

if (MSVC)
    target_compile_options(tokenstream PRIVATE /W4 /WX)
else ()
    target_compile_options(tokenstream PRIVATE -Wall -Wextra -pedantic -Werror)
endif ()

add_executable(Google_Tests_run ${PROTO_SRCS} ${PROTO_HDRS}
        tests/TokenStreamTest.cpp
        tests/PackageData.cpp)

target_link_libraries(Google_Tests_run gtest_main tokenstream)

set_target_properties(tokenstream PROPERTIES COMPILE_PDB_NAME tokenstream)

install(TARGETS tokenstream
        ARCHIVE DESTINATION lib
        )

install(DIRECTORY include
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        )

if (MSVC)
    install(FILES ${CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY}/tokenstream.pdb
            CONFIGURATIONS Debug RelWithDebInfo
            DESTINATION lib
            )
endif ()
