// SPDX-License-Identifier: MIT
#pragma once
#include <QDialog>
#include <QUrl>
#include <QVector>

namespace Ui { class HelpDialog; }

// Aide intégrée : rubriques en Markdown (resources/help/*.md, compilées dans l'exécutable), recherche plein texte.
class HelpDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HelpDialog(QWidget *parent = nullptr);
    ~HelpDialog() override;

    // key = nom de fichier de la rubrique sans extension (ex. « 07_atelier3 »)
    void showChapter(const QString &key);

private slots:
    void onChapterChanged(int row);
    void onSearchChanged(const QString &text);
    void findNext();
    void findPrevious();
    void onAnchorClicked(const QUrl &url);
    void previousChapter();
    void nextChapter();

private:
    struct Chapter {
        QString key, title, markdown;
    };
    void loadChapters();
    int neighbour(int from, int step) const;     // rubrique visible suivante / précédente, -1 si aucune

    Ui::HelpDialog *ui = nullptr;                // interface : ui/HelpDialog.ui (Qt Designer)
    QVector<Chapter> chapters_;
};
