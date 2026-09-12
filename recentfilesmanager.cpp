#include "recentfilesmanager.h"
#include <QSettings>
#include <QFileInfo>
#include <QAction>

RecentFilesManager::RecentFilesManager(QWidget *parent)
    : QObject(parent)
    , m_menu(new QMenu(parent))
{
    m_menu->setTitle("🕒  Recent Files");
    loadSettings();
    rebuildMenu();
}

void RecentFilesManager::addFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    m_recentFiles.removeAll(filePath);
    m_recentFiles.prepend(filePath);

    while (m_recentFiles.size() > MaxRecentFiles) {
        m_recentFiles.removeLast();
    }

    saveSettings();
    rebuildMenu();
}

void RecentFilesManager::showMenuAt(const QPoint &globalPos)
{
    if (m_menu) {
        m_menu->exec(globalPos);
    }
}

void RecentFilesManager::clearHistory()
{
    m_recentFiles.clear();
    saveSettings();
    rebuildMenu();
}

void RecentFilesManager::loadSettings()
{
    QSettings settings("Prussiadriver", "Prussiadriver");
    m_recentFiles = settings.value("recentFiles").toStringList();
}

void RecentFilesManager::saveSettings()
{
    QSettings settings("Prussiadriver", "Prussiadriver");
    settings.setValue("recentFiles", m_recentFiles);
}

void RecentFilesManager::rebuildMenu()
{
    if (!m_menu) {
        return;
    }

    m_menu->clear();

    if (m_recentFiles.isEmpty()) {
        QAction *emptyAction = m_menu->addAction("No Recent Files");
        emptyAction->setEnabled(false);
        return;
    }

    for (int i = 0; i < m_recentFiles.size(); ++i) {
        const QString &path = m_recentFiles.at(i);
        QString fileName = QFileInfo(path).fileName();

        QAction *fileAction = m_menu->addAction(QString("%1.  %2").arg(i + 1).arg(fileName));
        fileAction->setToolTip(path);

        connect(fileAction, &QAction::triggered, this, [this, path]() {
            emit fileSelected(path);
        });
    }

    m_menu->addSeparator();
    QAction *clearAction = m_menu->addAction("🗑️  Clear History");
    connect(clearAction, &QAction::triggered, this, &RecentFilesManager::clearHistory);
}
