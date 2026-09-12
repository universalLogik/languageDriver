#include "translationengine.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
//#include <QDebug>
#include <QThread> // Add this line here
#include <QTimer>
// #include "llama.h"


TranslationEngine::TranslationEngine(QObject *parent)
    : QObject(parent)
    ,isReady(false)
    ,m_currentIndex(0)
    ,m_retryCount(0)
    ,m_maxRetries(30)
    ,serverProcess(nullptr)
    ,m_manager(nullptr)
    ,m_binPath("/home/Verya/projects/Prussiadriver/llama.cpp/build/bin")
{

}

TranslationEngine::~TranslationEngine() {
    // Clean up the background server process when the application exits
    if (serverProcess) {
        if (serverProcess->state() != QProcess::NotRunning) {
            serverProcess->terminate();
            if (!serverProcess->waitForFinished(3000)) {
                serverProcess->kill();
            }
        }

    }
    delete m_manager;
    m_manager=nullptr;
}

void TranslationEngine::translateChunksAsync(const QStringList &germanChunks) {
  //  if (!isReady || germanChunks.isEmpty()) {
      if (germanChunks.isEmpty()) {
        emit batchTranslationFinished();
        return;
    }

    m_chunks = germanChunks;
    m_currentIndex = 0;
    m_retryCount = 0;

    // UPDATE: Boot the Model Server back into Memory if it was released previously
    if (!isReady) {
        if (!initializeEngine(pathToModel)) {
            emit batchTranslationFinished();
            return;
        }
    }

    // Launch the very first Request to start the Chain
    sendNextRequest();
}

bool TranslationEngine::initializeEngine(const QString &modelPath) {
    if (modelPath.isEmpty()) {
        return false;
    }

    pathToModel = modelPath;

    if (!serverProcess) {
       serverProcess = std::make_unique<QProcess>();
    }


    if (!m_manager) {
        m_manager = new QNetworkAccessManager();
    }

    // Define the absolute Path to the binary Directory

    QString program = m_binPath + "/llama-server";

    // Set the working Directory so the Server finds its local Files
    serverProcess->setWorkingDirectory(m_binPath);
    // Inject the Path Environment Variable to force the Loading of the Vulkan Plugin
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("GGML_BACKEND_PATH", m_binPath);
    serverProcess->setProcessEnvironment(env);
    serverProcess->setStandardOutputFile(QProcess::nullDevice());
    serverProcess->setStandardErrorFile(QProcess::nullDevice());

    //serverProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    QStringList arguments;
    // arguments << "-m" << modelPath
    //           << "-c" << "4096"
    //           << "--port" << "8085"
    //           << "-ngl" << "38"            //
    //           << "-t" << "6"                   // Match physical Cores
    //           << "-b" << "1024"            // Reduced Batch Size for safer Memory allocation
    //           << "-lv" << "5"
    //           << "-ub" << "512";

    arguments << "-m" << modelPath
              << "-c" << "4096"
              << "--port" << "8085"
              << "-ngl" << "33"     // Llama 3.1 8B has 33 transformer layers
              << "-t" << "6"        // Ryzen 5 4500U: 6 physical cores
              << "-b" << "1024"
              << "-ub" << "512";
    //        << "-lv" << "5";



    serverProcess->start(program, arguments);

    if (!serverProcess->waitForStarted(5000)) {
    //    qDebug() << "Failed to launch the background llama-server process.";
        return false;
    }

    isReady = true;
    return true;
}


void TranslationEngine::sendNextRequest() {
    if (m_currentIndex >= m_chunks.size()) {
        emit batchTranslationFinished();

        if (serverProcess) {
            if (serverProcess->state() != QProcess::NotRunning) {
                serverProcess->terminate();
                if (!serverProcess->waitForFinished(2000)) {
                    serverProcess->kill();
                    serverProcess->waitForFinished();
                }
            }
            serverProcess.reset();
        }
        isReady = false;

        return;
    }

    if (!m_manager) {
        m_manager = new QNetworkAccessManager(this);
    }

    QString germanText = m_chunks.at(m_currentIndex);

    if (germanText.trimmed().isEmpty()) {
        emit chunkTranslationFinished(m_currentIndex, "");
        m_currentIndex++;
        m_retryCount = 0;
   //     sendNextRequest();
        QTimer::singleShot(0, this, &TranslationEngine::sendNextRequest);
        return;
    }
    QUrl url("http://127.0.0.1:8085/completion");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // QString promptText =
    //     QString("<start_of_turn>user\n"
    //             "You are a professional translator. Translate the following German text into clear, natural English. Provide only the translation without any commentary.\n\n"
    //             "%1<end_of_turn>\n"
    //             "<start_of_turn>model\n").arg(germanText);

    QString promptText =
        QString("<|begin_of_text|><|start_header_id|>system<|end_header_id|>\n"
                "You are a professional translator. Translate German into clear, natural English. "
                "Output only the translation and nothing else.<|eot_id|>"
                "<|start_header_id|>user<|end_header_id|>\n"
                "%1<|eot_id|>"
                "<|start_header_id|>assistant<|end_header_id|>\n")
            .arg(germanText);


    QJsonObject json;
    json["prompt"] = promptText;
    json["n_predict"] = 128;
    json["temperature"] = 0.2;

    // Add Gemma stop tokens to stop generation cleanly
    QJsonArray stopTokens;
    // stopTokens.append("<end_of_turn>");
    // stopTokens.append("<eos>");
    stopTokens.append("<|eot_id|>");
    stopTokens.append("<|end_of_text|>");
    json["stop"] = stopTokens;

    auto reply = m_manager->post(request, QJsonDocument(json).toJson());

    // Connect the finish Signal directly to our State Controller
    connect(reply, &QNetworkReply::finished, this, &TranslationEngine::handleReplyFinished);

}

void TranslationEngine::handleReplyFinished() {
  auto reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool isServerLoading = (statusCode.isValid() && statusCode.toInt() == 503);
    bool isPortNotReady = (reply->error() == QNetworkReply::ConnectionRefusedError);

    // Non-blocking asynchronous Retry handling using a native Qt Timer
    if ((isServerLoading || isPortNotReady) && m_retryCount < m_maxRetries) {
       // qDebug() << "Server is loading the Model. Retrying asynchronously... Attempt" << (m_retryCount + 1);
        m_retryCount++;
        reply->deleteLater();

        QTimer::singleShot(2000, this, &TranslationEngine::sendNextRequest);
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        //qDebug() << "Network Error on Chunk" << m_currentIndex << ":" << reply->errorString();
        emit chunkTranslationFinished(m_currentIndex, "[Translation Error: Connection terminated.]");
    } else {
        QByteArray responseData = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        QString translation = responseDoc.object().value("content").toString().trimmed();

        // Emit the String immediately to trigger the UI Slot update
        emit chunkTranslationFinished(m_currentIndex, translation);
    }

    reply->deleteLater();

    // Advance the Index Counter and trigger the next Network Action
    m_currentIndex++;
    m_retryCount = 0;
    sendNextRequest();
}






















