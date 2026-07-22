cmake_minimum_required(VERSION 3.20)

project(sts_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(sts_common
    common/Serialization.cpp
)
target_include_directories(sts_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(sts_game
    game/Box.cpp
    game/Hero.cpp
    game/HeroFactory.cpp
    game/BoxBoard.cpp
    game/EliminationRule.cpp
    game/GameSession.cpp
)
target_include_directories(sts_game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sts_game PUBLIC sts_common)

add_executable(sts_demo demo/main.cpp)
target_link_libraries(sts_demo PRIVATE sts_game)

include(CTest)
if(BUILD_TESTING)
    add_executable(sts_tests
        tests/TestMain.cpp
        tests/EliminationRuleTests.cpp
        tests/BoardTests.cpp
        tests/HeroTests.cpp
        tests/ProtocolTests.cpp
        tests/ViewTransformTests.cpp
    )
    target_link_libraries(sts_tests PRIVATE sts_game)
    add_test(NAME sts_tests COMMAND sts_tests)
endif()

# Networking is optional, but enabling it always creates both TCP executables.
option(STS_BUILD_NETWORK "Build the standalone Asio TCP server and client" ON)
if(STS_BUILD_NETWORK)
    include(FetchContent)
    FetchContent_Declare(
        standalone_asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(standalone_asio)

    if(NOT EXISTS "${standalone_asio_SOURCE_DIR}/asio/include/asio.hpp")
        message(FATAL_ERROR "Standalone Asio was fetched but asio/include/asio.hpp is missing.")
    endif()

    add_library(sts_asio INTERFACE)
    target_include_directories(sts_asio INTERFACE "${standalone_asio_SOURCE_DIR}/asio/include")
    target_compile_definitions(sts_asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)

    find_package(Threads REQUIRED)

    add_executable(sts_server server/main.cpp server/GameServer.cpp)
    target_link_libraries(sts_server PRIVATE sts_game sts_common Threads::Threads sts_asio)

    add_executable(sts_client client/main.cpp client/GameClient.cpp)
    target_link_libraries(sts_client PRIVATE sts_common Threads::Threads sts_asio)

    if(BUILD_TESTING)
        add_executable(sts_network_tests
            tests/NetworkSessionTests.cpp
            server/GameServer.cpp
        )
        target_link_libraries(sts_network_tests PRIVATE sts_game sts_common Threads::Threads sts_asio)
        add_test(NAME sts_network_tests COMMAND sts_network_tests)
    endif()

    if(WIN32)
        target_link_libraries(sts_server PRIVATE ws2_32 mswsock)
        target_link_libraries(sts_client PRIVATE ws2_32 mswsock)
        if(BUILD_TESTING)
            target_link_libraries(sts_network_tests PRIVATE ws2_32 mswsock)
        endif()
    endif()
endif()
