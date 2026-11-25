// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>

#include "Project.h"

namespace Ui { class RadarPage; }

class RadarPage : public QWidget
{
    Q_OBJECT
public:
    explicit RadarPage(Project *p, QWidget *parent = nullptr);
    ~RadarPage() override;
    bool exportPng(QWidget *dialogParent);

private slots:
    void refresh();

private:
    Ui::RadarPage *ui = nullptr;      // interface : ui/RadarPage.ui (Qt Designer)
    Project *project_ = nullptr;
};
