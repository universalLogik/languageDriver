#ifndef DICTIONARYWORKER_H
#define DICTIONARYWORKER_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QPair>
#include <memory> // include for unique_ptr

class Dictionary;
class DictionaryWorker : public QObject
{
    Q_OBJECT
public:
    explicit DictionaryWorker(QObject *parent = nullptr);
    ~DictionaryWorker();

public slots:
    void initializeDatabase();
    void processChunks(const QStringList &germanChunks);
    void updateWordEntries(int chunkIndex, const QList<QPair<QString, QString>> &updatedList);
    void processPureLookup(const QString &text);
    void updatePureWordEntries(const QList<QPair<QString, QString>> &updatedList);


signals:
    // CHANGED: Emits a sequential list to protect insertion order
    void lookupChunkFinished(int index,
                             const QList<QPair<QString, QString>> &originalList,
                             const QList<QPair<QString, QString>> &exerciseList);
    void wordUpdateFinished(bool success);
    void pureLookupFinished(const QList<QPair<QString, QString>> &results);
void pureWordUpdateFinished(bool success);





private:

    std::unique_ptr<Dictionary> m_dictionary;
};

#endif // DICTIONARYWORKER_H
