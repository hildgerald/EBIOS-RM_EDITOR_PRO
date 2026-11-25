// SPDX-License-Identifier: MIT
#include "ReportLayoutPage.h"

#include "Calc.h"
#include "ReportAssembler.h"
#include "ReportPainter.h"
#include "ui_ReportLayoutPage.h"

#include <QBuffer>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QPixmap>
#include <QSignalBlocker>

ReportLayoutPage::ReportLayoutPage(Project *p, QWidget *parent) : QWidget(parent), ui(new Ui::ReportLayoutPage), project_(p)
{
    ui->setupUi(this);

    connect(ui->groupCover, &QGroupBox::toggled, this, &ReportLayoutPage::commit);
    connect(ui->groupToc, &QGroupBox::toggled, this, &ReportLayoutPage::commit);
    connect(ui->groupFigures, &QGroupBox::toggled, this, &ReportLayoutPage::commit);
    connect(ui->scenarioFiguresCheck, &QCheckBox::toggled, this, &ReportLayoutPage::commit);
    connect(ui->logoWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ReportLayoutPage::commit);
    connect(ui->logoAlignCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ReportLayoutPage::commit);
    connect(ui->subtitleEdit, &QLineEdit::textChanged, this, &ReportLayoutPage::commit);
    connect(ui->organisationEdit, &QLineEdit::textChanged, this, &ReportLayoutPage::commit);
    connect(ui->classificationCombo, &QComboBox::editTextChanged, this, &ReportLayoutPage::commit);
    connect(ui->logoLoadButton, &QPushButton::clicked, this, &ReportLayoutPage::chooseLogo);
    connect(ui->logoClearButton, &QPushButton::clicked, this, &ReportLayoutPage::clearLogo);
    connect(project_, &Project::changed, this, &ReportLayoutPage::load);
    load();
}

ReportLayoutPage::~ReportLayoutPage()
{
    delete ui;
}

void ReportLayoutPage::load()
{
    if (!inEdit_) {
        loading_ = true;
        const QJsonObject o = project_->section("rapport");
        ui->groupCover->setChecked(o.value("garde").toBool(true));
        ui->groupToc->setChecked(o.value("sommaire").toBool(true));
        ui->groupFigures->setChecked(o.value("figures").toBool(true));
        ui->scenarioFiguresCheck->setChecked(o.value("figures_so").toBool(true));
        ui->logoWidthSpin->setValue(o.contains("logo_largeur") ? o.value("logo_largeur").toInt(45) : 45);
        const QString al = Calc::s(o, "logo_align");
        ui->logoAlignCombo->setCurrentIndex(al == "centre" ? 1 : (al == "droite" ? 2 : 0));
        if (ui->subtitleEdit->text() != Calc::s(o, "sous_titre")) ui->subtitleEdit->setText(Calc::s(o, "sous_titre"));
        if (ui->organisationEdit->text() != Calc::s(o, "organisation")) ui->organisationEdit->setText(Calc::s(o, "organisation"));
        if (ui->classificationCombo->currentText() != Calc::s(o, "classification"))
            ui->classificationCombo->setEditText(Calc::s(o, "classification"));
        ui->scenarioFiguresCheck->setEnabled(ui->groupFigures->isChecked());
        loading_ = false;
    }
    updatePreview();
}

void ReportLayoutPage::commit()
{
    if (loading_) return;
    QJsonObject o = project_->section("rapport");
    o["garde"] = ui->groupCover->isChecked();
    o["sommaire"] = ui->groupToc->isChecked();
    o["figures"] = ui->groupFigures->isChecked();
    o["figures_so"] = ui->scenarioFiguresCheck->isChecked();
    o["logo_largeur"] = ui->logoWidthSpin->value();
    const int al = ui->logoAlignCombo->currentIndex();
    o["logo_align"] = al == 1 ? QStringLiteral("centre") : (al == 2 ? QStringLiteral("droite") : QStringLiteral("gauche"));
    o["sous_titre"] = ui->subtitleEdit->text();
    o["organisation"] = ui->organisationEdit->text();
    o["classification"] = ui->classificationCombo->currentText();
    ui->scenarioFiguresCheck->setEnabled(ui->groupFigures->isChecked());
    inEdit_ = true;
    project_->setSection("rapport", o);
    inEdit_ = false;
    updatePreview();
}

void ReportLayoutPage::chooseLogo()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Choisir un logo"), QString(),
                                                      tr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp)"));
    if (path.isEmpty()) return;
    QImage img(path);
    if (img.isNull()) {
        QMessageBox::warning(this, tr("Logo"), tr("Impossible de lire cette image."));
        return;
    }
    if (img.width() > 1200) img = img.scaledToWidth(1200, Qt::SmoothTransformation);   // limite la taille du projet
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");                                                            // conserve la transparence
    QJsonObject o = project_->section("rapport");
    o["logo"] = QString::fromLatin1(bytes.toBase64());
    project_->setSection("rapport", o);                                               // -> load() -> aperçu
}

void ReportLayoutPage::clearLogo()
{
    QJsonObject o = project_->section("rapport");
    o["logo"] = QString();
    project_->setSection("rapport", o);
}

void ReportLayoutPage::updatePreview()
{
    const QImage img = ReportPainter::coverPreview(ReportAssembler::coverFrom(*project_), 560);
    ui->coverPreviewLabel->setPixmap(QPixmap::fromImage(img));
    ui->coverPreviewLabel->setEnabled(ui->groupCover->isChecked());
}
