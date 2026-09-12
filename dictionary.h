#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <QString>
#include <QStringList>
#include <QMap>

class Dictionary
{
public:
    Dictionary();
    ~Dictionary();
    bool connectToDatabase();
    QMap<QString, QString> translateWords(const QStringList &words);

    bool updateWordTranslation(const QString &word, const QString &translation);

    bool updateMultipleWords(const QList<QPair<QString, QString>> &updatedList);

};

#endif // DICTIONARY_H
