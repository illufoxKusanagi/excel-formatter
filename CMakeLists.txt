cmake_minimum_required(VERSION 3.5)

project(trainLoggerFormatter)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_subdirectory(QXlsx/QXlsx)
include_directories(QXlsx)

set(PROJECT_SOURCES
    main.cpp
    pages/mainwindow.cpp
    pages/mainwindow.h
    controllers/file_handler.h
    controllers/file_handler.cpp
    mainwindow.ui
    icons/icons.qrc
)

if(WIN32)
  set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/icons/icons.rc")
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(trainLoggerFormatter
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${APP_ICON_RESOURCE}
    )
else()
    if(ANDROID)
        add_library(trainLoggerFormatter SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(trainLoggerFormatter
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Tautkan pustaka Qt dan QtXlsx
target_link_libraries(trainLoggerFormatter PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    QXlsx
)

set_target_properties(trainLoggerFormatter PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS trainLoggerFormatter
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(trainLoggerFormatter)
endif()
