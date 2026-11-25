// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>

#include "Project.h"

namespace Ui { class SummaryPage; }

class SummaryPage : public QWidget
{
    Q_OBJECT
public:
    explicit SummaryPage(Project *p, QWidget *parent = nullptr);
    ~SummaryPage() override;

private slots:
    void load();
    void commit();

private:
    bool loading_ = false, inEdit_ = false;
    Ui::SummaryPage *ui = nullptr;      // interface : ui/SummaryPage.ui (Qt Designer)
    Project *project_ = nullptr;
};
