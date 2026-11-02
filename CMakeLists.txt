cmake_minimum_required(VERSION 3.21)

project(FloatingTranslator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

qt_add_executable(FloatingTranslator
    src/main.cpp
    src/MainWindow.cpp src/MainWindow.h
    src/SentenceTab.cpp src/SentenceTab.h
    src/WordTab.cpp src/WordTab.h
    src/DeepLClient.cpp src/DeepLClient.h
    src/PonsClient.cpp src/PonsClient.h
    src/SettingsDialog.cpp src/SettingsDialog.h
    src/Style.h
)

qt_add_resources(FloatingTranslator "assets"
    PREFIX "/"
    BASE assets
    FILES assets/logo.svg
)

# Icon of the .exe in Explorer and the taskbar.
if(WIN32)
    target_sources(FloatingTranslator PRIVATE assets/app.rc)
endif()

target_link_libraries(FloatingTranslator PRIVATE Qt6::Widgets Qt6::Network)

set_target_properties(FloatingTranslator PROPERTIES WIN32_EXECUTABLE ON)

if(MSVC)
    target_compile_options(FloatingTranslator PRIVATE /utf-8)
endif()

# "cmake --install" produces a standalone folder with the Qt DLLs next to the exe.
install(TARGETS FloatingTranslator
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
qt_generate_deploy_app_script(
    TARGET FloatingTranslator
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
