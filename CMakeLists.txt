cmake_minimum_required(VERSION 3.19)
project(tracker-horas VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Test)
qt_standard_project_setup()

qt_add_executable(tracker-horas
    src/main.cpp
    src/model.h src/model.cpp
    src/storage.h src/storage.cpp
    src/appsettings.h src/appsettings.cpp
    src/hourbank.h src/hourbank.cpp
    src/theme.h src/theme.cpp
    src/card.h src/card.cpp
    src/icons.h
    src/widgets.h
    src/csvexport.h src/csvexport.cpp
    src/punchedit.h src/punchedit.cpp
    src/todaypage.h src/todaypage.cpp
    src/timesheetmodel.h src/timesheetmodel.cpp
    src/timesheetpage.h src/timesheetpage.cpp
    src/weekchart.h src/weekchart.cpp
    src/bankpage.h src/bankpage.cpp
    src/pages.h src/pages.cpp
    src/mainwindow.h src/mainwindow.cpp
)

qt_add_resources(tracker-horas "assets"
    PREFIX "/"
    BASE assets
    FILES assets/style.qss assets/check.svg assets/arrow-down.svg
          assets/tracker-horas.svg
)

target_link_libraries(tracker-horas PRIVATE Qt6::Widgets)

# --------------------------------------------- Instalação e pacote .deb

include(GNUInstallDirs)

install(TARGETS tracker-horas RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/tracker-horas.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/tracker-horas.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

set(CPACK_PACKAGE_NAME "tracker-horas")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Controle de ponto e tarefas diárias (Qt 6, Wayland nativo)")
set(CPACK_PACKAGE_CONTACT "Ricardo Fachini <ricardofachinisouza@gmail.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ricardofachini/tracker-de-horas")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT) # tracker-horas_<versão>_<arch>.deb
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
# dpkg-shlibdeps declara as dependências (libqt6widgets6 etc.) sozinho
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_STRIP_FILES ON)
include(CPack)

# ------------------------------------------------------- Testes (ctest)

enable_testing()

function(add_unit_test name)
    qt_add_executable(${name} tests/${name}.cpp ${ARGN})
    target_include_directories(${name} PRIVATE src)
    target_link_libraries(${name} PRIVATE Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    # offscreen: os testes rodam sem compositor (CI, terminal puro)
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

add_unit_test(tst_model src/model.cpp)
add_unit_test(tst_storage src/storage.cpp src/model.cpp)
add_unit_test(tst_appsettings src/appsettings.cpp)
add_unit_test(tst_hourbank src/hourbank.cpp src/model.cpp)
add_unit_test(tst_csvexport
    src/csvexport.cpp src/storage.cpp src/model.cpp
    src/card.cpp src/theme.cpp src/appsettings.cpp src/hourbank.cpp)
target_link_libraries(tst_csvexport PRIVATE Qt6::Widgets)
