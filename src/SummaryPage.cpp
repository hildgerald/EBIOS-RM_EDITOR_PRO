// SPDX-License-Identifier: MIT
#include "SummaryPage.h"

#include "Calc.h"
#include "UiUtil.h"
#include "ui_SummaryPage.h"

SummaryPage::SummaryPage(Project *p, QWidget *parent) : QWidget(parent), ui(new Ui::SummaryPage), project_(p)
{
    ui->setupUi(this);
    for (QLineEdit *e : {ui->titleEdit, ui->versionEdit, ui->dateEdit, ui->authorEdit, ui->ownerEdit})
        connect(e, &QLineEdit::textChanged, this, &SummaryPage::commit);
    for (QPlainTextEdit *e : {ui->objectiveEdit, ui->docsEdit, ui->historyEdit})
        connect(e, &QPlainTextEdit::textChanged, this, &SummaryPage::commit);
    connect(project_, &Project::changed, this, &SummaryPage::load);
    load();
}

SummaryPage::~SummaryPage()
{
    delete ui;
}

void SummaryPage::load()
{
    if (inEdit_) return;
    loading_ = true;
    const QJsonObject o = project_->section("sommaire");
    setIfDifferent(ui->titleEdit, Calc::s(o, "titre"));
    setIfDifferent(ui->versionEdit, Calc::s(o, "version"));
    setIfDifferent(ui->dateEdit, Calc::s(o, "date"));
    setIfDifferent(ui->authorEdit, Calc::s(o, "auteur"));
    setIfDifferent(ui->ownerEdit, Calc::s(o, "responsable"));
    setIfDifferent(ui->objectiveEdit, Calc::s(o, "objectif"));
    setIfDifferent(ui->docsEdit, Calc::s(o, "documents"));
    setIfDifferent(ui->historyEdit, Calc::s(o, "historique"));
    loading_ = false;
}

void SummaryPage::commit()
{
    if (loading_) return;
    QJsonObject o = project_->section("sommaire");
    o["titre"] = ui->titleEdit->text();
    o["version"] = ui->versionEdit->text();
    o["date"] = ui->dateEdit->text();
    o["auteur"] = ui->authorEdit->text();
    o["responsable"] = ui->ownerEdit->text();
    o["objectif"] = ui->objectiveEdit->toPlainText();
    o["documents"] = ui->docsEdit->toPlainText();
    o["historique"] = ui->historyEdit->toPlainText();
    inEdit_ = true;
    project_->setSection("sommaire", o);
    inEdit_ = false;
}
