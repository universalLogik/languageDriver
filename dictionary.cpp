#include "dictionary.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

Dictionary::Dictionary()
{

}
Dictionary::~Dictionary()
{

}

bool Dictionary::updateMultipleWords(const QList<QPair<QString, QString>> &updatedList) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        return false;
    }

    db.transaction();

    bool allSuccess = true;
    for (const auto &[word, translation] : updatedList) {
        if (!updateWordTranslation(word, translation)) {
            allSuccess = false;
        }
    }

    if (allSuccess) {
        db.commit();
    } else {
        db.rollback();
    }

    return allSuccess;
}




bool Dictionary::updateWordTranslation(const QString &word, const QString &translation) {
    // 1. Check if the Word already exists in the Database
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM de_eg WHERE german_word = :word");
    checkQuery.bindValue(":word", word);

    if (!checkQuery.exec() || !checkQuery.next()) {
        qDebug() << "Database Check Error:" << checkQuery.lastError().text();
        return false;
    }

    int count = checkQuery.value(0).toInt();

    if (count > 0) {
        // 2. Update existing Records
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE de_eg SET english_translation = :translation WHERE german_word = :word");
        updateQuery.bindValue(":translation", translation);
        updateQuery.bindValue(":word", word);

        if (!updateQuery.exec()) {
            qDebug() << "Database Update Error:" << updateQuery.lastError().text();
            return false;
        }
    } else {
        // 3. Insert as a new Entry if it does not exist
        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT INTO de_eg (german_word, english_translation) VALUES (:word, :translation)");
        insertQuery.bindValue(":word", word);
        insertQuery.bindValue(":translation", translation);

        if (!insertQuery.exec()) {
            qDebug() << "Database Insert Error:" << insertQuery.lastError().text();
            return false;
        }
    }

    return true;
}


bool Dictionary::connectToDatabase() {
    QSqlDatabase db;
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    } else {
        db = QSqlDatabase::addDatabase("QMYSQL");
    }

    db.setHostName("127.0.0.1");
    db.setDatabaseName("dictionary");
    db.setUserName("root");
    db.setPassword("Fyff");

    if (!db.open()) {
        qDebug() << "Database Connection Error:" << db.lastError().text();
        return false;
    }
    return true;
}

QMap<QString, QString> Dictionary::translateWords(const QStringList &words) {
    QMap<QString, QString> sortedTranslations;
    QSqlQuery query;
    query.prepare("SELECT english_translation FROM de_eg WHERE german_word = :word LIMIT 1");

    for (const QString &word : words) {
        if (sortedTranslations.contains(word)) {
            continue;
        }

        query.bindValue(":word", word);
        if (query.exec() && query.next()) {
            QString translation = query.value(0).toString();
            sortedTranslations.insert(word, translation);
        } else {
            sortedTranslations.insert(word, "[Translation not found]");
        }
    }

    return sortedTranslations;
}
