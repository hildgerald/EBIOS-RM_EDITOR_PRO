# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16)
project(EbiosRMStudio VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# moc / uic / rcc automatiques : les fichiers .ui du dossier ui/ sont compilés en ui_<Nom>.h
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Type de build" FORCE)
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)   # QPdfWriter / QTextDocument font partie de Qt6::Gui (tiré par Widgets)

set(EBIOS_SOURCES
    src/main.cpp
    src/Project.cpp          src/Project.h
    src/Calc.cpp             src/Calc.h
    src/Widgets.cpp          src/Widgets.h
    src/UiUtil.h
    src/TablePage.cpp        src/TablePage.h
    src/PageFactory.cpp      src/PageFactory.h
    src/SummaryPage.cpp      src/SummaryPage.h
    src/MetricsPage.cpp      src/MetricsPage.h
    src/ArchitecturePage.cpp src/ArchitecturePage.h
    src/RadarPage.cpp        src/RadarPage.h
    src/OperationalDetailPage.cpp src/OperationalDetailPage.h
    src/TreatmentPage.cpp    src/TreatmentPage.h
    src/ConclusionPage.cpp   src/ConclusionPage.h
    src/ReportPage.cpp       src/ReportPage.h
    src/ReportLayoutPage.cpp src/ReportLayoutPage.h
    src/HelpDialog.cpp       src/HelpDialog.h
    src/MainWindow.cpp       src/MainWindow.h
    src/ReportGenerator.cpp  src/ReportGenerator.h
    src/MarkdownModel.cpp    src/MarkdownModel.h
    src/ReportAssembler.cpp  src/ReportAssembler.h
    src/ReportPainter.cpp    src/ReportPainter.h
    src/ZipWriter.cpp        src/ZipWriter.h
    src/DocxExporter.cpp     src/DocxExporter.h
    src/PdfExporter.cpp      src/PdfExporter.h
)

# Les .ui sont listés pour apparaître dans Qt Creator (double-clic = Qt Designer)
set(EBIOS_FORMS
    ui/MainWindow.ui
    ui/TablePage.ui
    ui/SummaryPage.ui
    ui/MetricsPage.ui
    ui/ArchitecturePage.ui
    ui/RadarPage.ui
    ui/OperationalDetailPage.ui
    ui/TreatmentPage.ui
    ui/ConclusionPage.ui
    ui/ReportPage.ui
    ui/ReportLayoutPage.ui
    ui/HelpDialog.ui
)

# Aide intégrée : rubriques Markdown compilées dans l'exécutable (resources/help/*.md)
set(EBIOS_RESOURCES resources/resources.qrc)

add_executable(EbiosRMStudio ${EBIOS_SOURCES} ${EBIOS_FORMS} ${EBIOS_RESOURCES})
target_include_directories(EbiosRMStudio PRIVATE src)
target_link_libraries(EbiosRMStudio PRIVATE Qt6::Widgets)
target_compile_definitions(EbiosRMStudio PRIVATE APP_VERSION="${PROJECT_VERSION}")
if(MSVC)
    target_compile_options(EbiosRMStudio PRIVATE /utf-8 /W3 /permissive-)
    target_compile_definitions(EbiosRMStudio PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(EbiosRMStudio PRIVATE -Wall -Wextra)
endif()

if(WIN32)
    # Application graphique (pas de console) + évite les macros min/max de <windows.h>
    set_target_properties(EbiosRMStudio PROPERTIES WIN32_EXECUTABLE ON)
    target_compile_definitions(EbiosRMStudio PRIVATE NOMINMAX UNICODE _UNICODE)

    # Les textes de licence accompagnent l'exécutable (obligation LGPL pour les DLL de Qt)
    add_custom_command(TARGET EbiosRMStudio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
                "$<TARGET_FILE_DIR:EbiosRMStudio>"
        COMMENT "Copie des fichiers de licence")

    # Copie les DLL Qt et plugins à côté de l'exécutable après la compilation (exécutable autonome pour les tests)
    option(EBIOS_WINDEPLOYQT "Exécuter windeployqt après la compilation" ON)
    if(EBIOS_WINDEPLOYQT)
        find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt6 windeployqt
            HINTS "${Qt6_DIR}/../../../bin" "${QT_DIR}/../../../bin" ENV QTDIR PATH_SUFFIXES bin)
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET EbiosRMStudio POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-system-d3d-compiler --no-opengl-sw
                        "$<TARGET_FILE:EbiosRMStudio>"
                COMMENT "windeployqt : copie des bibliothèques Qt")
        else()
            message(STATUS "windeployqt introuvable : copiez les DLL Qt à la main ou ajoutez le dossier bin de Qt au PATH.")
        endif()
    endif()
endif()

if(APPLE)
    set_target_properties(EbiosRMStudio PROPERTIES MACOSX_BUNDLE ON)
endif()

# ---- Installation (cmake --install build --prefix <dossier>) ----
include(GNUInstallDirs)
install(TARGETS EbiosRMStudio
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE  DESTINATION .)
install(DIRECTORY examples/ DESTINATION ${CMAKE_INSTALL_DATADIR}/ebiosrm-studio/examples)
install(FILES README.md LICENSE THIRD_PARTY_NOTICES.md DESTINATION ${CMAKE_INSTALL_DATADIR}/ebiosrm-studio)
if(UNIX AND NOT APPLE)
    install(FILES packaging/ebiosrm-studio.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()
if(WIN32)
    # Sous Windows, on installe aussi les DLL/plugins Qt (Qt >= 6.5)
    if(COMMAND qt_generate_deploy_app_script)
        qt_generate_deploy_app_script(TARGET EbiosRMStudio OUTPUT_SCRIPT EBIOS_DEPLOY_SCRIPT NO_UNSUPPORTED_PLATFORM_ERROR)
        install(SCRIPT ${EBIOS_DEPLOY_SCRIPT})
    endif()
endif()
