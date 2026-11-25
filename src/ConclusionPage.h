// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>

#include "Project.h"

namespace Ui { class ConclusionPage; }

class ConclusionPage : public QWidget
{
    Q_OBJECT
public:
    explicit ConclusionPage(Project *p, QWidget *parent = nullptr);
    ~ConclusionPage() override;

private slots:
    void load();
    void commit();
    void draft();

private:
    bool loading_ = false, inEdit_ = false;
    Ui::ConclusionPage *ui = nullptr;      // interface : ui/ConclusionPage.ui (Qt Designer)
    Project *project_ = nullptr;
};
