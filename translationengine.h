#ifndef TRANSLATIONENGINE_H
#define TRANSLATIONENGINE_H
#include <QString>
#include <QObject>
#include <QProcess> // Added for background lifecycle management
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <memory>

// Class Declaration for the local Runner
class TranslationEngine : public QObject {
    Q_OBJECT

public:
    explicit TranslationEngine(QObject *parent = nullptr);
    ~TranslationEngine();

    bool initializeEngine(const QString &modelPath);
    void translateChunksAsync(const QStringList &germanChunks);


signals:
    // Emitted immediately when an individual Chunk finishes translating
    void chunkTranslationFinished(int index, const QString &translation);
    void batchTranslationFinished();

private slots:
    void handleReplyFinished();



private:
    void sendNextRequest();

private:

    bool isReady;
    QString pathToModel;
    std::unique_ptr<QProcess> serverProcess;

    QNetworkAccessManager *m_manager;
    QString m_binPath;
    QStringList m_chunks;
    int m_currentIndex;
    int m_retryCount;
    const int m_maxRetries;

};

#endif // TRANSLATIONENGINE_H
