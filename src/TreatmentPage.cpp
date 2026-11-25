// SPDX-License-Identifier: MIT
#include "TreatmentPage.h"

#include "Calc.h"
#include "Widgets.h"
#include "ui_TreatmentPage.h"

TreatmentPage::TreatmentPage(Project *p, QWidget *parent) : QWidget(parent), ui(new Ui::TreatmentPage), project_(p)
{
    ui->setupUi(this);
    connect(project_, &Project::changed, this, &TreatmentPage::refresh);
    refresh();
}

TreatmentPage::~TreatmentPage()
{
    delete ui;
}

void TreatmentPage::refresh()
{
    ui->initialMatrix->setData(tr("Risques initiaux"), Calc::matrixLevels(*project_), Calc::matrixTexts(*project_, false));
    ui->residualMatrix->setData(tr("Risques résiduels (projection)"), Calc::matrixLevels(*project_), Calc::matrixTexts(*project_, true));
}
