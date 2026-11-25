// SPDX-License-Identifier: MIT
#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("EBIOS RM Studio");
    app.setOrganizationName("EbiosRM");

    MainWindow w;
    if (argc > 1) w.openFile(QString::fromLocal8Bit(argv[1]));
    w.show();
    return app.exec();
}
