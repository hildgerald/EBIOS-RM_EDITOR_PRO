// SPDX-License-Identifier: MIT
#pragma once
#include <QTemporaryDir>
#include <QWidget>

#include "Project.h"

namespace Ui { class ReportPage; }

class ReportPage : public QWidget
{
    Q_OBJECT
public:
    explicit ReportPage(Project *p, QWidget *parent = nullptr);
    ~ReportPage() override;

    // Exports : affichent leurs propres boîtes de dialogue ; retournent true en cas de succès.
    bool exportMarkdown(QWidget *dialogParent);
    bool exportPdf(QWidget *dialogParent);
    bool exportDocx(QWidget *dialogParent);

protected:
    void showEvent(QShowEvent *) override;

private slots:
    void refresh();

private:
    QString suggestedFileName(const QString &extension) const;
    QString askSavePath(QWidget *parent, const QString &caption, const QString &extension, const QString &filter) const;
    void finished(QWidget *parent, bool ok, const QString &path, const QString &error);

    Ui::ReportPage *ui = nullptr;      // interface : ui/ReportPage.ui (Qt Designer)
    Project *project_ = nullptr;
    QTemporaryDir tmp_;                // images de l'aperçu
    int counter_ = 0;
};
