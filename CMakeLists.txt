cmake_minimum_required(VERSION 3.21)
project(phantompad)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/CPM.cmake)

if (WIN32)
    set(PLATFORM_SOURCES src/socket/WinSocket.cpp src/utils/Win.cpp src/driver/ViGEm.cpp res/phantompad.rc)

    CPMAddPackage("gh:nefarius/ViGEmClient@1.21.222.0")
    if (NOT MSVC)
        target_compile_options(ViGEmClient PRIVATE -Wno-pragma-pack)
    endif()
elseif (UNIX)
    set(PLATFORM_SOURCES src/socket/UnixSocket.cpp src/utils/Unix.cpp src/driver/UInput.cpp)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

add_executable(${PROJECT_NAME} src/Main.cpp ${PLATFORM_SOURCES})

CPMAddPackage("gh:fmtlib/fmt#12.1.0")
CPMAddPackage("gh:geode-sdk/result@1.4.1")

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
            -Wall -Wextra -Wpedantic -Werror
            -Wno-gnu-statement-expression-from-macro-expansion
            -Wno-missing-field-initializers
            -Wformat=2
    )
    target_compile_options(fmt PRIVATE -Wno-unused-result) # fmtlib 12.1.0 discards a nodiscard value
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt GeodeResult)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ViGEmClient ws2_32 setupapi)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
elseif (UNIX)
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/installer/phantompad.service
        DESTINATION /etc/systemd/system
    )
endif()
