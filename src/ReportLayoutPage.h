// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>

#include "Project.h"

namespace Ui { class ReportLayoutPage; }

// Réglages de mise en page des exports PDF / Word : page de garde (logo), sommaire, figures.
class ReportLayoutPage : public QWidget
{
    Q_OBJECT
public:
    explicit ReportLayoutPage(Project *p, QWidget *parent = nullptr);
    ~ReportLayoutPage() override;

private slots:
    void load();
    void commit();
    void chooseLogo();
    void clearLogo();
    void updatePreview();

private:
    Ui::ReportLayoutPage *ui = nullptr;      // interface : ui/ReportLayoutPage.ui (Qt Designer)
    Project *project_ = nullptr;
    bool loading_ = false, inEdit_ = false;
};
