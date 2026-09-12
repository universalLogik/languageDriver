#ifndef RECENTFILESMANAGER_H
#define RECENTFILESMANAGER_H

#include <QObject>
#include <QStringList>
#include <QMenu>
#include <QPoint>

class RecentFilesManager : public QObject
{
    Q_OBJECT
public:
    explicit RecentFilesManager(QWidget *parent = nullptr);
    ~RecentFilesManager() override = default;

    QMenu* menu() const { return m_menu; }
    void addFile(const QString &filePath);
    void showMenuAt(const QPoint &globalPos);
    void clearHistory();


















signals:
    void fileSelected(const QString &filePath);

private:
    void loadSettings();
    void saveSettings();
    void rebuildMenu();

    static constexpr int MaxRecentFiles = 8;
    QMenu *m_menu;
    QStringList m_recentFiles;
};

#endif // RECENTFILESMANAGER_H
