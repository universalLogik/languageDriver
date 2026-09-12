cmake_minimum_required(VERSION 3.16)

project(Prussiadriver VERSION 0.1 LANGUAGES CXX)

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
    resource.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Prussiadriver
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        recentfilesmanager.h recentfilesmanager.cpp
    )
else()
    if(ANDROID)
        add_library(Prussiadriver SHARED ${PROJECT_SOURCES})
    else()
        add_executable(Prussiadriver ${PROJECT_SOURCES})
    endif()
endif()

# Header include paths
target_include_directories(Prussiadriver PRIVATE
    /home/Verya/whisper.cpp/include
    /home/Verya/whisper.cpp/ggml/include
    ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/llama.cpp/ggml/include
)

# Apply compile definitions to the target
target_compile_definitions(Prussiadriver PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

# External library resolution
find_library(WHISPER_LIB
    NAMES whisper
    PATHS /home/Verya/whisper.cpp/build
          /home/Verya/whisper.cpp/build/src
          /home/Verya/whisper.cpp/build/Release
          /home/Verya/whisper.cpp/build/Release/src
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
target_link_libraries(Prussiadriver PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Multimedia
    ${WHISPER_LIB}
    ${LLAMA_LIB}
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Prussiadriver)
endif()

set_target_properties(Prussiadriver PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Prussiadriver
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Prussiadriver)



endif()


include(GNUInstallDirs)
install(TARGETS Prussiadriver
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Prussiadriver)
endif()

# Install Desktop Entry File
install(FILES Prussiadriver.desktop
    DESTINATION $ENV{HOME}/.local/share/applications
)

# Install Application Icon
install(FILES icon/Prussia.png
    DESTINATION $ENV{HOME}/.local/share/icons/hicolor/256x256/apps
    RENAME Prussiadriver.png
)
