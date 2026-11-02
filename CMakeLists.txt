cmake_minimum_required(VERSION 3.16)

project(languageDriver VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Widgets
    Sql
    Network
    Multimedia
)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Widgets
    Sql
    Network
    Multimedia
)

# Central source list containing all project files
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    dictionary.h
    dictionary.cpp
    translationengine.h
    translationengine.cpp
    dictionaryworker.h
    dictionaryworker.cpp
    audiotranscriptionworker.h
    audiotranscriptionworker.cpp
    multieditorwidget.cpp
    multieditorwidget.h

    resource.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(languageDriver
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        recentfilesmanager.h recentfilesmanager.cpp
    )
else()
    if(ANDROID)
        add_library(languageDriver SHARED ${PROJECT_SOURCES})
    else()
        add_executable(languageDriver ${PROJECT_SOURCES})
    endif()
endif()

# Header include paths
target_include_directories(languageDriver PRIVATE
    $ENV{WHISPER_PATH}/include
    $ENV{WHISPER_PATH}/ggml/include
    ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/ggml/include
)

# Apply compile definitions to the target
target_compile_definitions(languageDriver PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

# External library resolution
find_library(WHISPER_LIB
    NAMES whisper
    PATHS $ENV{WHISPER_PATH}/build
          $ENV{WHISPER_PATH}/build/src
          $ENV{WHISPER_PATH}/build/Release
          $ENV{WHISPER_PATH}/build/Release/src
    NO_DEFAULT_PATH
)

find_library(LLAMA_LIB
    NAMES llama
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/build/src
          ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/build/bin
          ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/build
    NO_DEFAULT_PATH
)

# Linked targets
target_link_libraries(languageDriver PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Multimedia
    ${WHISPER_LIB}
    ${LLAMA_LIB}
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.languageDriver)
endif()

set_target_properties(languageDriver PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS languageDriver
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(languageDriver)
endif()

# Install Desktop Entry File
install(FILES languageDriver.desktop
    DESTINATION $ENV{HOME}/.local/share/applications
)

# Install Application Icon
install(FILES icon/languageDriver.png
    DESTINATION $ENV{HOME}/.local/share/icons/hicolor/256x256/apps
    RENAME languageDriver.png
)
