#ifndef AUDIOTRANSCRIPTIONWORKER_H
#define AUDIOTRANSCRIPTIONWORKER_H

#include <QObject>
#include <QString>
#include <whisper.h>

class AudioTranscriptionWorker : public QObject {
    Q_OBJECT

public:
    explicit AudioTranscriptionWorker(QObject *parent = nullptr);
    ~AudioTranscriptionWorker();


private:
    bool readWavFile(const QString &filePath, std::vector<float> &pcmf32);


public slots:

    void processTranscription(const QString &audioFilePath, const QString &languageCode);

signals:
    void whisperInitialized(bool success);
    void transcriptionFinished(const QString &transcribedText);
    void transcriptionError(const QString &errorMessage);

private:
    whisper_context *m_ctx;
    QString m_currentLoadedModel;
};

#endif // AUDIOTRANSCRIPTIONWORKER_H
