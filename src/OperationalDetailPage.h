// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>

#include "Project.h"

class TablePage;
namespace Ui { class OperationalDetailPage; }

class OperationalDetailPage : public QWidget
{
    Q_OBJECT
public:
    explicit OperationalDetailPage(Project *p, QWidget *parent = nullptr);
    ~OperationalDetailPage() override;

private slots:
    void refreshList();
    void onSelect();
    void refreshChain();

private:
    TablePage *actions_ = nullptr;
    QString current_, shown_;
    bool refreshing_ = false;
    Ui::OperationalDetailPage *ui = nullptr;      // interface : ui/OperationalDetailPage.ui (Qt Designer)
    Project *project_ = nullptr;
};
