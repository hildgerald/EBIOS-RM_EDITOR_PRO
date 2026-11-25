# Projet qmake (alternative à CMake) – ouvrir ce fichier dans Qt Creator pour éditer les .ui avec Qt Designer.
QT += widgets
CONFIG += c++17
TARGET = EbiosRMStudio
TEMPLATE = app

INCLUDEPATH += src
DEFINES += APP_VERSION=\\\"1.2.0\\\"

win32 {
    DEFINES += NOMINMAX UNICODE _UNICODE
    msvc: QMAKE_CXXFLAGS += /utf-8
}

SOURCES += \
    src/main.cpp src/Project.cpp src/Calc.cpp src/Widgets.cpp src/TablePage.cpp src/PageFactory.cpp \
    src/SummaryPage.cpp src/MetricsPage.cpp src/ArchitecturePage.cpp src/RadarPage.cpp \
    src/OperationalDetailPage.cpp src/TreatmentPage.cpp src/ConclusionPage.cpp \
    src/ReportPage.cpp src/ReportLayoutPage.cpp src/HelpDialog.cpp src/MainWindow.cpp \
    src/ReportGenerator.cpp src/MarkdownModel.cpp src/ReportAssembler.cpp src/ReportPainter.cpp \
    src/ZipWriter.cpp src/DocxExporter.cpp src/PdfExporter.cpp

HEADERS += \
    src/Project.h src/Calc.h src/Widgets.h src/UiUtil.h src/TablePage.h src/PageFactory.h \
    src/SummaryPage.h src/MetricsPage.h src/ArchitecturePage.h src/RadarPage.h \
    src/OperationalDetailPage.h src/TreatmentPage.h src/ConclusionPage.h \
    src/ReportPage.h src/ReportLayoutPage.h src/HelpDialog.h src/MainWindow.h \
    src/ReportGenerator.h src/MarkdownModel.h src/ReportAssembler.h src/ReportPainter.h \
    src/ZipWriter.h src/DocxExporter.h src/PdfExporter.h

FORMS += \
    ui/MainWindow.ui ui/TablePage.ui ui/SummaryPage.ui ui/MetricsPage.ui ui/ArchitecturePage.ui \
    ui/RadarPage.ui ui/OperationalDetailPage.ui ui/TreatmentPage.ui ui/ConclusionPage.ui \
    ui/ReportPage.ui ui/ReportLayoutPage.ui ui/HelpDialog.ui

# Aide intégrée (rubriques Markdown dans resources/help)
RESOURCES += resources/resources.qrc
