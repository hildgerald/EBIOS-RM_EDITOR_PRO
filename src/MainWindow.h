// SPDX-License-Identifier: MIT
#pragma once
#include <QMainWindow>

#include "Project.h"

class HelpDialog;
class QTreeWidgetItem;
class RadarPage;
class ReportPage;
namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *e) override;

private slots:
    void newProject();
    void openProject();
    bool saveProject();
    bool saveProjectAs();
    void updateTitle();
    void onNavChanged(QTreeWidgetItem *cur);
    void about();
    void showHelp(const QString &key);
    void showContextHelp();

private:
    bool maybeSave();
    void addPage(QTreeWidgetItem *parent, const QString &title, QWidget *page, const QString &helpKey = QString());

    Ui::MainWindow *ui = nullptr;      // interface : ui/MainWindow.ui (Qt Designer)
    Project *project_ = nullptr;
    RadarPage *radar_ = nullptr;
    ReportPage *report_ = nullptr;
    HelpDialog *helpDialog_ = nullptr;
};
