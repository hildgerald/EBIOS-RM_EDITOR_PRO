// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>

#include "Project.h"

namespace Ui { class ArchitecturePage; }

class ArchitecturePage : public QWidget
{
    Q_OBJECT
public:
    explicit ArchitecturePage(Project *p, QWidget *parent = nullptr);
    ~ArchitecturePage() override;

private slots:
    void load();
    void commitText();
    void chooseImage();
    void clearImage();

private:
    void showImage();
    bool loading_ = false, inEdit_ = false;
    Ui::ArchitecturePage *ui = nullptr;      // interface : ui/ArchitecturePage.ui (Qt Designer)
    Project *project_ = nullptr;
};
