#include "dictionaryworker.h"
#include "dictionary.h"
#include <algorithm>
#include <QTextBoundaryFinder>
#include <QRegularExpression>
#include <QSet>
#include  <QSqlQuery>
#include <QDebug>
#include <QSqlError>


DictionaryWorker::DictionaryWorker(QObject *parent) : QObject(parent)
    , m_dictionary(std::make_unique<Dictionary>())
{
}

DictionaryWorker::~DictionaryWorker() = default;

void DictionaryWorker::initializeDatabase()
{
    if (m_dictionary) {
        m_dictionary->connectToDatabase();
    }
}

void DictionaryWorker::processChunks(const QStringList &germanChunks)
{
    if (!m_dictionary) return;

    for (int i = 0; i < germanChunks.size(); ++i) {
        QString sentence = germanChunks.at(i);
        QStringList originalWords;

        QTextBoundaryFinder finder(QTextBoundaryFinder::Word, sentence);
        int lastPos = 0;

        while (finder.toNextBoundary() != -1) {
            int currentPos = finder.position();
            QString word = sentence.mid(lastPos, currentPos - lastPos).trimmed();

            if (!word.isEmpty() && word.at(0).isLetterOrNumber()) {
                if (!originalWords.contains(word)) {
                    originalWords.append(word);
                }
            }
            lastPos = currentPos;
        }

        QStringList sortedWords = originalWords;
        std::sort(sortedWords.begin(), sortedWords.end(), [](const QString &a, const QString &b) {
            return QString::compare(a, b, Qt::CaseInsensitive) < 0;
        });

        QMap<QString, QString> wordMap = m_dictionary->translateWords(originalWords);

        QList<QPair<QString, QString>> originalList;
        for (const QString &word : originalWords) {
            originalList.append(qMakePair(word, wordMap.value(word, "[Translation not found]")));
        }

        QList<QPair<QString, QString>> exerciseList;
        for (const QString &word : sortedWords) {
            exerciseList.append(qMakePair(word, wordMap.value(word, "[Translation not found]")));
        }

        emit lookupChunkFinished(i, originalList, exerciseList);
    }
}

void DictionaryWorker::updateWordEntries(int chunkIndex, const QList<QPair<QString, QString>> &updatedList)
{
    Q_UNUSED(chunkIndex);
    if (!m_dictionary) {
        emit wordUpdateFinished(false);
        return;
    }

    // Delegate transaction execution directly to the Dictionary class
    bool success = m_dictionary->updateMultipleWords(updatedList);
    emit wordUpdateFinished(success);
}


void DictionaryWorker::processPureLookup(const QString &text) {
    if (text.trimmed().isEmpty()) {
        emit pureLookupFinished(QList<QPair<QString, QString>>());
        return;
    }

    static const QRegularExpression wordRegex(QString::fromUtf8("[A-Za-zÄÖÜäöüß]+"));
    QRegularExpressionMatchIterator it = wordRegex.globalMatch(text);

    QSet<QString> seenWords;
    QStringList uniqueWords;

    while (it.hasNext()) {
        QString word = it.next().captured(0).trimmed();
        if (word.length() > 1 && !seenWords.contains(word)) {
            seenWords.insert(word);
            uniqueWords.append(word);
        }
    }

    QList<QPair<QString, QString>> results;

    QSqlQuery query;
    query.prepare("SELECT english_translation FROM de_eg WHERE german_word = :word LIMIT 1");

    for (const QString &word : uniqueWords) {
        query.bindValue(":word", word);

        if (query.exec() && query.next()) {
            QString definition = query.value(0).toString().trimmed();
            if (!definition.isEmpty()) {
                results.append(qMakePair(word, definition));
            }
        }
    }


    emit pureLookupFinished(results);
}



void DictionaryWorker::updatePureWordEntries(const QList<QPair<QString, QString>> &updatedList) {
    if (!m_dictionary) {
        emit pureWordUpdateFinished(false);
        return;
    }

    bool success = m_dictionary->updateMultipleWords(updatedList);
    emit pureWordUpdateFinished(success);
}
