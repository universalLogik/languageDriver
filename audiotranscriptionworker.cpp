#include "audiotranscriptionworker.h"
#include <QDebug>
#include <QFile>
//#include <QDataStream> // ADD THIS
#include <vector>      // ADD THIS
#include <QRegularExpression>
#include "whisper.h"


AudioTranscriptionWorker::AudioTranscriptionWorker(QObject *parent)
    : QObject(parent), m_ctx(nullptr), m_currentLoadedModel("")
{

}

AudioTranscriptionWorker::~AudioTranscriptionWorker() {
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}





void AudioTranscriptionWorker::processTranscription(const QString &audioFilePath, const QString &languageCode) {
    if (audioFilePath.isEmpty()) {
      //  qDebug() << "[Whisper Debug] Audio File Path is empty.";
        emit transcriptionFinished("");
        return;
    }

   // qDebug() << "[Whisper Debug] Starting Transcription process for Language:" << languageCode;

    QString modelPath = (languageCode == "en")
                            ? "/home/Verya/whisper.cpp/models/ggml-medium.en.bin"
                            : "/home/Verya/whisper.cpp/models/ggml-medium.bin";

    if (!m_ctx || m_currentLoadedModel != modelPath) {
        if (m_ctx) {
          //  qDebug() << "[Whisper Debug] Freeing previously cached Model Context:" << m_currentLoadedModel;
            whisper_free(m_ctx);
            m_ctx = nullptr;
        }

       // qDebug() << "[Whisper Debug] Loading Model File from Path:" << modelPath;
        struct whisper_context_params cparams = whisper_context_default_params();
        m_ctx = whisper_init_from_file_with_params(modelPath.toUtf8().constData(), cparams);

        if (!m_ctx) {
          //  qDebug() << "[Whisper Debug Error] Failed to initialize Whisper Context from:" << modelPath;
            emit transcriptionFinished("Error: Failed to load Whisper Model.");
            return;
        }

        m_currentLoadedModel = modelPath;
       // qDebug() << "[Whisper Debug] Model successfully loaded and cached in Memory.";
    } else {
      //  qDebug() << "[Whisper Debug] Reusing existing cached Model Context for:" << m_currentLoadedModel;
    }

    std::string lang = languageCode.toStdString();
    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_realtime   = false;
    params.print_progress   = false;
    params.print_timestamps = false;
    params.print_special    = false;
    params.translate        = false;
    params.language         = lang.c_str();
    params.n_threads        = 4;

    params.no_speech_thold  = 0.8f;
    params.entropy_thold    = 2.8f;

    std::vector<float> pcmf32;
   // qDebug() << "[Whisper Debug] Reading WAV File at Path:" << audioFilePath;
    if (!readWavFile(audioFilePath, pcmf32)) {
        //qDebug() << "[Whisper Debug Error] Audio Reader failed to parse File:" << audioFilePath;
        emit transcriptionFinished("Error: Could not read Audio File.");
        return;
    }

    qDebug() << "[Whisper Debug] Running Whisper Inference on" << pcmf32.size() << "Audio Samples...";
    if (whisper_full(m_ctx, params, pcmf32.data(), pcmf32.size()) != 0) {
     //   qDebug() << "[Whisper Debug Error] Inference Execution failed in whisper_full().";
        emit transcriptionFinished("Error: Transcription failed.");
        return;
    }

    QString resultText;
    int n_segments = whisper_full_n_segments(m_ctx);
   // qDebug() << "[Whisper Debug] Inference complete. Extracted Segment Count:" << n_segments;

    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(m_ctx, i);
        QString segStr = QString::fromUtf8(text);
       // qDebug() << QString("[Whisper Debug] Segment %1: %2").arg(i).arg(segStr);
        resultText += segStr;
    }

    static const QRegularExpression tagRegex("\\[.*?\\]");
    resultText.remove(tagRegex);
    resultText = resultText.trimmed();

  //  qDebug() << "[Whisper Debug] Final Output Text:" << resultText;
    emit transcriptionFinished(resultText);
}



bool AudioTranscriptionWorker::readWavFile(const QString &filePath, std::vector<float> &pcmf32) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
      //  qDebug() << "[WAV Debug Error] Cannot open File at Path:" << filePath;
        return false;
    }

    QByteArray rawData = file.readAll();
    file.close();

    if (rawData.size() < 44) {
       // qDebug() << "[WAV Debug Error] File Size is smaller than standard 44-byte Header:" << rawData.size() << "Bytes.";
        return false;
    }

    if (!rawData.startsWith("RIFF") || rawData.mid(8, 4) != "WAVE") {
      //  qDebug() << "[WAV Debug Error] File is missing valid RIFF/WAVE Magic Bytes.";
        return false;
    }

    int offset = 12;
    uint32_t dataSize = 0;
    int dataOffset = -1;

    while (offset + 8 <= rawData.size()) {
        QByteArray chunkId = rawData.mid(offset, 4);
        //uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(rawData.constData() + offset + 4);
        uint32_t chunkSize = 0;
        std::memcpy(&chunkSize, rawData.constData() + offset + 4, sizeof(uint32_t));

        if (chunkId == "data") {
            dataOffset = offset + 8;
            dataSize = chunkSize;
            break;
        }

        uint32_t paddedSize = chunkSize;
        if (paddedSize % 2 != 0) {
            paddedSize += 1;
        }

        // Advance using padded size to maintain 2-byte alignment
        offset += 8 + paddedSize;
    }

    if (dataOffset == -1) {
      //  qDebug() << "[WAV Debug Error] Could not find 'data' Chunk Marker in RIFF Stream.";
        return false;
    }

    if (dataOffset + dataSize > static_cast<uint32_t>(rawData.size())) {
        dataSize = rawData.size() - dataOffset;
    }

    const int16_t *pcm16 = reinterpret_cast<const int16_t*>(rawData.constData() + dataOffset);
    size_t numSamples = dataSize / sizeof(int16_t);

    if (numSamples == 0) {
        qDebug() << "[WAV Debug Error] 0 Audio Samples found in Data Chunk.";
        return false;
    }

    pcmf32.resize(numSamples);
    float maxAmp = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        pcmf32[i] = static_cast<float>(pcm16[i]) / 32768.0f;
        float absVal = std::abs(pcmf32[i]);
        if (absVal > maxAmp) {
            maxAmp = absVal;
        }
    }

    //qDebug() << "[WAV Debug] Successfully extracted" << numSamples << "Samples. Peak Amplitude:" << maxAmp;

    if (maxAmp > 0.001f && maxAmp < 0.8f) {
        float scale = 0.95f / maxAmp;
        for (size_t i = 0; i < numSamples; ++i) {
            pcmf32[i] *= scale;
        }
       // qDebug() << "[WAV Debug] Applied Volume Boost with Factor:" << scale;
    }

    return true;
}
