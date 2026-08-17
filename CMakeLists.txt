cmake_minimum_required(VERSION 3.16)
project(kobiQ VERSION 0.1.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network DBus)

add_executable(kobiQ
  src/main.cpp
  src/ClipboardMonitor.cpp
  src/HistoryStore.cpp
  src/MainWindow.cpp
  src/TrayController.cpp
  src/PasteHelper.cpp
  src/GlobalHotkey.cpp
  src/SingleInstance.cpp
  src/InstancePaths.cpp
  src/ApplicationDBus.cpp
  resources.qrc
)

target_include_directories(kobiQ PRIVATE src)
target_compile_definitions(kobiQ PRIVATE KOBIQ_VERSION="${PROJECT_VERSION}")
target_link_libraries(kobiQ PRIVATE Qt6::Widgets Qt6::Network Qt6::DBus)

install(TARGETS kobiQ RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS packaging/kobiq-show DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS packaging/kobiq-show-debug DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS packaging/kobiq-activate DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/kobiQ.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES packaging/org.kobiq.Application.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES icons/kobiQ.svg
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps)
install(FILES packaging/kobiQ.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo)
