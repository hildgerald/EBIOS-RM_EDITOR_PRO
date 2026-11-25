// SPDX-License-Identifier: MIT
#pragma once
#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;

#include "Project.h"

namespace Ui { class MetricsPage; }

class MetricsPage : public QWidget
{
    Q_OBJECT
public:
    explicit MetricsPage(Project *p, QWidget *parent = nullptr);
    ~MetricsPage() override;

private slots:
    void load();
    void commit();

private:
    bool loading_ = false, inEdit_ = false;
    QLineEdit *g_[4] = {};
    QLineEdit *v_[4] = {};
    QDoubleSpinBox *z_[4] = {};
    QComboBox *mat_[16] = {};
    Ui::MetricsPage *ui = nullptr;      // interface : ui/MetricsPage.ui (Qt Designer)
    Project *project_ = nullptr;
};
