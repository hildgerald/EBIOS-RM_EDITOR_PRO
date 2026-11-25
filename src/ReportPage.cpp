// SPDX-License-Identifier: MIT
#include "ReportPage.h"

#include "Calc.h"
#include "DocxExporter.h"
#include "PdfExporter.h"
#include "ReportGenerator.h"
#include "ui_ReportPage.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QUrl>

ReportPage::ReportPage(Project *p, QWidget *parent) : QWidget(parent), ui(new Ui::ReportPage), project_(p)
{
    ui->setupUi(this);
    connect(ui->refreshButton, &QPushButton::clicked, this, &ReportPage::refresh);
    connect(ui->exportMdButton, &QPushButton::clicked, this, [this] { exportMarkdown(this); });
    connect(ui->exportPdfButton, &QPushButton::clicked, this, [this] { exportPdf(this); });
    connect(ui->exportDocxButton, &QPushButton::clicked, this, [this] { exportDocx(this); });
    connect(project_, &Project::changed, this, [this] { if (isVisible()) refresh(); });
}

ReportPage::~ReportPage()
{
    delete ui;
}

void ReportPage::showEvent(QShowEvent *)
{
    refresh();
}

void ReportPage::refresh()
{
    // Images de l'aperçu : dossier temporaire à nom unique pour éviter tout cache d'images.
    QString assets;
    if (tmp_.isValid()) {
        const QString dir = QDir(tmp_.path()).filePath(QString::number(++counter_));
        if (ReportGenerator::writeAssets(*project_, dir, QSize(1000, 750)))
            assets = ReportGenerator::dirUrl(dir);
    }
    const int vpos = ui->reportBrowser->verticalScrollBar()->value();
    ui->reportBrowser->setMarkdown(ReportGenerator::generate(*project_, assets, ReportGenerator::Mode::Preview));
    ui->reportBrowser->verticalScrollBar()->setValue(vpos);
}

QString ReportPage::suggestedFileName(const QString &extension) const
{
    QString base = Calc::s(project_->section("sommaire"), "titre");
    if (base.isEmpty()) base = QStringLiteral("rapport_ebios_rm");
    base.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]+")), QStringLiteral("_"));
    return base + extension;
}

QString ReportPage::askSavePath(QWidget *parent, const QString &caption, const QString &extension, const QString &filter) const
{
    QString path = QFileDialog::getSaveFileName(parent, caption, suggestedFileName(extension), filter);
    if (path.isEmpty()) return QString();
    if (!path.endsWith(extension, Qt::CaseInsensitive)) path += extension;
    return path;
}

void ReportPage::finished(QWidget *parent, bool ok, const QString &path, const QString &error)
{
    if (!ok) {
        QMessageBox::warning(parent, tr("Export"), tr("Échec de l'export : %1").arg(error));
        return;
    }
    QMessageBox box(QMessageBox::Information, tr("Export"), tr("Rapport exporté :\n%1").arg(QDir::toNativeSeparators(path)),
                    QMessageBox::Ok, parent);
    QPushButton *open = box.addButton(tr("Ouvrir"), QMessageBox::ActionRole);
    box.exec();
    if (box.clickedButton() == open) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool ReportPage::exportMarkdown(QWidget *parent)
{
    const QString path = askSavePath(parent, tr("Exporter le rapport Markdown"), QStringLiteral(".md"), tr("Markdown (*.md)"));
    if (path.isEmpty()) return false;
    QString err;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = ReportGenerator::exportToFile(*project_, path, &err);
    QApplication::restoreOverrideCursor();
    finished(parent, ok, path, err);
    return ok;
}

bool ReportPage::exportPdf(QWidget *parent)
{
    const QString path = askSavePath(parent, tr("Exporter le rapport PDF"), QStringLiteral(".pdf"), tr("Document PDF (*.pdf)"));
    if (path.isEmpty()) return false;
    QString err;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = PdfExporter::exportReport(*project_, path, &err);
    QApplication::restoreOverrideCursor();
    finished(parent, ok, path, err);
    return ok;
}

bool ReportPage::exportDocx(QWidget *parent)
{
    const QString path = askSavePath(parent, tr("Exporter le rapport Word"), QStringLiteral(".docx"), tr("Document Word (*.docx)"));
    if (path.isEmpty()) return false;
    QString err;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool ok = DocxExporter::exportReport(*project_, path, &err);
    QApplication::restoreOverrideCursor();
    finished(parent, ok, path, err);
    return ok;
}
