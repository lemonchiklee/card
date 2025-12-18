cmake_minimum_required(VERSION 3.16)
project(CardCab VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Windows-specific settings
if(WIN32)
    # Enable static linking on Windows for easier distribution
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        add_compile_options(/utf-8)  # Enable UTF-8 source encoding
    endif()
    
    # Hide console window in release mode (GUI application)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Sql)

message(STATUS "Qt5: ${Qt5_VERSION}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")

set(SOURCES
    src/main.cpp
    src/database.cpp
    src/logindialog.cpp
    src/carddialog.cpp
    src/cardwidget.cpp
    src/mainwindow.cpp
)

set(HEADERS
    include/constants.h
    include/card.h
    include/user.h
    include/database.h
    include/logindialog.h
    include/carddialog.h
    include/cardwidget.h
    include/mainwindow.h
)

# Windows resource file for application icon (optional)
if(WIN32)
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources/cardcab.rc")
        list(APPEND SOURCES resources/cardcab.rc)
    endif()
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Sql
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3)
endif()

# Installation
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES sql/init.sql DESTINATION share/${PROJECT_NAME})

# Windows deployment - copy Qt DLLs
if(WIN32)
    # Get the Qt bin directory
    get_target_property(QT5_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT5_BINDIR "${QT5_QMAKE_EXECUTABLE}" DIRECTORY)
    
    # Post-build: copy required DLLs (for development)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Run windeployqt to deploy Qt DLLs for distribution"
    )
endif()

message(STATUS "")
message(STATUS "====================================")
message(STATUS "  ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Build: ${CMAKE_BUILD_TYPE}")
message(STATUS "====================================")
