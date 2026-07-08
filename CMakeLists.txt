cmake_minimum_required(VERSION 3.19)
project(tracker-horas VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(tracker-horas
    src/main.cpp
    src/model.h src/model.cpp
    src/storage.h src/storage.cpp
    src/theme.h src/theme.cpp
    src/card.h src/card.cpp
    src/icons.h
    src/widgets.h
    src/todaypage.h src/todaypage.cpp
    src/timesheetmodel.h src/timesheetmodel.cpp
    src/timesheetpage.h src/timesheetpage.cpp
    src/pages.h src/pages.cpp
    src/mainwindow.h src/mainwindow.cpp
)

qt_add_resources(tracker-horas "assets"
    PREFIX "/"
    BASE assets
    FILES assets/style.qss assets/check.svg
)

target_link_libraries(tracker-horas PRIVATE Qt6::Widgets)
