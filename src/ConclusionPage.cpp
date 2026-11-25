// SPDX-License-Identifier: MIT
#include "ConclusionPage.h"

#include "Calc.h"
#include "ReportGenerator.h"
#include "UiUtil.h"
#include "ui_ConclusionPage.h"

#include <QMessageBox>

ConclusionPage::ConclusionPage(Project *p, QWidget *parent) : QWidget(parent), ui(new Ui::ConclusionPage), project_(p)
{
    ui->setupUi(this);
    connect(ui->conclusionEdit, &QPlainTextEdit::textChanged, this, &ConclusionPage::commit);
    connect(ui->draftButton, &QPushButton::clicked, this, &ConclusionPage::draft);
    connect(project_, &Project::changed, this, &ConclusionPage::load);
    load();
}

ConclusionPage::~ConclusionPage()
{
    delete ui;
}

void ConclusionPage::load()
{
    if (inEdit_) return;
    loading_ = true;
    setIfDifferent(ui->conclusionEdit, Calc::s(project_->section("conclusion"), "texte"));
    loading_ = false;
}

void ConclusionPage::commit()
{
    if (loading_) return;
    QJsonObject o = project_->section("conclusion");
    o["texte"] = ui->conclusionEdit->toPlainText();
    inEdit_ = true;
    project_->setSection("conclusion", o);
    inEdit_ = false;
}

void ConclusionPage::draft()
{
    if (!ui->conclusionEdit->toPlainText().trimmed().isEmpty() &&
        QMessageBox::question(this, tr("Brouillon"), tr("Le texte actuel sera remplacé par le brouillon. Continuer ?")) != QMessageBox::Yes)
        return;
    ui->conclusionEdit->setPlainText(ReportGenerator::draftConclusion(*project_));   // déclenche commit()
}
