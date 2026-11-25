// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>

#include "Project.h"

namespace Ui { class TreatmentPage; }

class TreatmentPage : public QWidget
{
    Q_OBJECT
public:
    explicit TreatmentPage(Project *p, QWidget *parent = nullptr);
    ~TreatmentPage() override;

private slots:
    void refresh();

private:
    Ui::TreatmentPage *ui = nullptr;      // interface : ui/TreatmentPage.ui (Qt Designer)
    Project *project_ = nullptr;
};
