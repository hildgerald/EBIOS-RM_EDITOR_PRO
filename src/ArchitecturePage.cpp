// SPDX-License-Identifier: MIT
#include "ArchitecturePage.h"

#include "Calc.h"
#include "UiUtil.h"
#include "ui_ArchitecturePage.h"

#include <QBuffer>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QPixmap>

ArchitecturePage::ArchitecturePage(Project *p, QWidget *parent) : QWidget(parent), ui(new Ui::ArchitecturePage), project_(p)
{
    ui->setupUi(this);
    connect(ui->descriptionEdit, &QPlainTextEdit::textChanged, this, &ArchitecturePage::commitText);
    connect(ui->loadButton, &QPushButton::clicked, this, &ArchitecturePage::chooseImage);
    connect(ui->clearButton, &QPushButton::clicked, this, &ArchitecturePage::clearImage);
    connect(project_, &Project::changed, this, &ArchitecturePage::load);
    load();
}

ArchitecturePage::~ArchitecturePage()
{
    delete ui;
}

void ArchitecturePage::load()
{
    if (inEdit_) return;
    loading_ = true;
    setIfDifferent(ui->descriptionEdit, Calc::s(project_->section("architecture"), "description"));
    loading_ = false;
    showImage();
}

void ArchitecturePage::showImage()
{
    const QByteArray b64 = Calc::s(project_->section("architecture"), "image").toLatin1();
    QPixmap pm;
    if (!b64.isEmpty()) pm.loadFromData(QByteArray::fromBase64(b64));
    if (pm.isNull()) {
        ui->imageLabel->setPixmap(QPixmap());
        ui->imageLabel->setText(tr("Aucune image"));
    } else {
        ui->imageLabel->setPixmap(pm);
    }
}

void ArchitecturePage::commitText()
{
    if (loading_) return;
    QJsonObject o = project_->section("architecture");
    o["description"] = ui->descriptionEdit->toPlainText();
    inEdit_ = true;
    project_->setSection("architecture", o);
    inEdit_ = false;
}

void ArchitecturePage::chooseImage()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Choisir un schéma d'architecture"), QString(),
                                                      tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (path.isEmpty()) return;
    QImage img(path);
    if (img.isNull()) {
        QMessageBox::warning(this, tr("Image"), tr("Impossible de lire cette image."));
        return;
    }
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    QJsonObject o = project_->section("architecture");
    o["image"] = QString::fromLatin1(bytes.toBase64());
    project_->setSection("architecture", o);   // déclenche load() -> showImage()
}

void ArchitecturePage::clearImage()
{
    QJsonObject o = project_->section("architecture");
    o["image"] = QString();
    project_->setSection("architecture", o);
}
