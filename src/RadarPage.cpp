// SPDX-License-Identifier: MIT
#include "RadarPage.h"

#include "Calc.h"
#include "Widgets.h"
#include "ui_RadarPage.h"

#include <QFileDialog>
#include <QMessageBox>

RadarPage::RadarPage(Project *p, QWidget *parent) : QWidget(parent), ui(new Ui::RadarPage), project_(p)
{
    ui->setupUi(this);
    connect(ui->modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RadarPage::refresh);
    connect(ui->exportButton, &QPushButton::clicked, this, [this] { exportPng(this); });
    connect(project_, &Project::changed, this, &RadarPage::refresh);
    refresh();
}

RadarPage::~RadarPage()
{
    delete ui;
}

void RadarPage::refresh()
{
    const bool proj = ui->modeCombo->currentIndex() == 1;
    ui->radarWidget->setData(Calc::radarItems(*project_, proj), thresholdsFrom(*project_),
                             proj ? tr("Radar de la menace – après mesures complémentaires (projection)")
                                  : tr("Radar de la menace – situation actuelle"));
}

bool RadarPage::exportPng(QWidget *dialogParent)
{
    const QString path = QFileDialog::getSaveFileName(dialogParent, tr("Exporter le radar"), QStringLiteral("radar_menace.png"),
                                                      tr("Image PNG (*.png)"));
    if (path.isEmpty()) return false;
    if (!ui->radarWidget->toImage().save(path)) {
        QMessageBox::warning(dialogParent, tr("Export"), tr("Échec de l'écriture du fichier."));
        return false;
    }
    return true;
}
