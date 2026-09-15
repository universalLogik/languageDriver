#include "mainwindow.h"
#include "translationengine.h"
#include "dictionaryworker.h"
#include "audiotranscriptionworker.h"
#include "recentfilesmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QLabel>
#include <QThread>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QShortcut>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QKeyEvent>
#include <QPushButton>
#include <QUrl>
#include <QClipboard>
#include <QPalette>
#include <QColor>
#include <QStyleHints>
#include <QStyleFactory>
#include <QGuiApplication>
#include <QApplication>
#include <QMessageBox>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
    ,m_modelPath("/home/Verya/projects/Prussiadriver/models/Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf")
    ,m_piperProcess(nullptr)
    ,m_showOriginalOrder(false)
    ,m_directInputVoiceMode(false)
    ,m_translationEngineInitialized(false)
    ,btnWhisperGerman(nullptr)
    ,btnWhisperEnglish(nullptr)
    ,btnWhisperSpanish(nullptr)
    ,m_activeTranscriptionLanguage("de")
    ,m_whisperModelPath("")
    ,m_cleanedText("")
    ,m_recordedAudioPath("")
    ,m_currentFilePath("")
    ,wordOutputEditorSecondary(nullptr)
    ,btnSplitWordEditor(nullptr)
    ,btnCopyTranslation(nullptr)
    ,btnEditWordEditor(nullptr)
    ,btnToggleTheme(nullptr)
    ,m_isEditingWordMode(false)
    ,m_isDarkMode(false)
    ,btnLookupOnly(nullptr)
    ,m_recentFilesManager(nullptr)
    ,statusLabel(nullptr)
    ,m_fileMenu(nullptr)
    ,translationEngine(nullptr)
    ,translationThread(nullptr)
    ,dictionaryWorker(nullptr)
    ,dictionaryThread(nullptr)
    ,m_audioWorker(nullptr)
    ,m_audioThread(nullptr)
    ,wordStackedWidget(nullptr)
    ,lookupOutputEditor(nullptr)
    ,btnTabSentenceWords(nullptr)
    ,btnTabGlossary(nullptr)
    ,lblWordHeader(nullptr)
{
    setupSeamButtonMenu();
    initFileMenuButton();
    setupUiLayout();

    setupCollapseFeature();
    setupSortFeature();
    setupVoiceModeFeature();
    setupAudioPlaybackFeature();
    initLookupOnlyButton();

    initHistoryMenu();
    setupFileMenu();

    initTranscribeButton();
    initThemeToggleButton();
    initCopyButton();
    initEditWordButton();

    setupTranslationFeature();
    setupWordExplanationsFeature();
    setupBottomControlFeature();

    setupShortcutsAndEvents();
    setupSpeechRecognitionFeature();
    setupAudioRecorder();

    initialPiperProcess();

    applyTheme(m_isDarkMode);
    updateWindowTitle();
}

MainWindow::~MainWindow() {
    if (translationThread) {
        translationThread->quit();
        translationThread->wait();
        translationEngine = nullptr;
    }

    if (dictionaryThread) {
        dictionaryThread->quit();
        dictionaryThread->wait();
        dictionaryWorker = nullptr;
    }

    if (m_audioThread) {
        m_audioThread->quit();
        m_audioThread->wait();
        m_audioWorker = nullptr;
    }

    if (m_piperProcess && m_piperProcess->state() != QProcess::NotRunning) {
        m_piperProcess->terminate();
        if (!m_piperProcess->waitForFinished(1000)) {
            m_piperProcess->kill();
            m_piperProcess->waitForFinished();
        }
    }

    if (m_recordProcess && m_recordProcess->state() != QProcess::NotRunning) {
        m_recordProcess->kill();
        m_recordProcess->waitForFinished();
    }
}

void MainWindow::initHistoryMenu() {
    m_recentFilesManager = new RecentFilesManager(this);
    connect(m_recentFilesManager, &RecentFilesManager::fileSelected, this, &MainWindow::loadFile);
}

void MainWindow::handleBatchFinished() {
    chunkLabel->setStyleSheet("color: #00FF00;");
    chunkLabel->setText(QString("No.: %1 || %2")
                            .arg(m_currentChunkIndex + 1)
                            .arg(m_germanChunks.size()));

    if (statusLabel) {
        statusLabel->setStyleSheet("color: #00FF00;");
        statusLabel->setText("Ready");
    }
    m_translationEngineInitialized = false;
}

void MainWindow::processText() {
    stripComments();
    QString text = m_cleanedText;
    if (text.isEmpty()) {
        clearAll();
        return;
    }

    if (!m_translationEngineInitialized) {
        emit initTranslation(m_modelPath);
        m_translationEngineInitialized = true;
    }

    m_germanChunks.clear();
    QRegularExpression sentenceRegex("[^.!?:]+[.!?:]*");
    QRegularExpressionMatchIterator it = sentenceRegex.globalMatch(text);
    while (it.hasNext()) {
        QString chunk = it.next().captured(0).trimmed();
        if (!chunk.isEmpty()) {
            m_germanChunks.append(chunk);
        }
    }

    int totalChunks = m_germanChunks.size();
    if (totalChunks == 0) {
        clearAll();
        return;
    }

    m_sentenceChunks.clear();
    m_englishTranslations.clear();
    m_chunkedOriginalExplanations.clear();
    m_chunkedExerciseExplanations.clear();

    m_showOriginalOrder = false;
    if (btnToggleSort) {
        btnToggleSort->setChecked(false);
    }

    for (int i = 0; i < totalChunks; ++i) {
        m_sentenceChunks.append("Translating...");
        m_englishTranslations.append(QString());
        m_chunkedOriginalExplanations.append(QList<QPair<QString, QString>>());
        m_chunkedExerciseExplanations.append(QList<QPair<QString, QString>>());
    }

    m_currentChunkIndex = 0;
    updateChunkDisplay();

    prevButton->setEnabled(false);
    nextButton->setEnabled(totalChunks > 1);

    if (statusLabel) {
        statusLabel->setStyleSheet("color: #d4af37;");
        statusLabel->setText("Translating...");
    }

    emit operateTranslation(m_germanChunks);
    emit operateLookup(m_germanChunks);
}

void MainWindow::handleLookupChunkFinished(int index,
                                           const QList<QPair<QString, QString>> &originalList,
                                           const QList<QPair<QString, QString>> &exerciseList) {
    if (index >= 0 && index < m_chunkedOriginalExplanations.size()) {
        m_chunkedOriginalExplanations[index] = originalList;
        m_chunkedExerciseExplanations[index] = exerciseList;

        if (index == m_currentChunkIndex) {
            updateChunkDisplay();
        }
    }
}

void MainWindow::handleChunkTranslationFinished(int index, const QString &translation) {
    if (index >= 0 && index < m_sentenceChunks.size()) {
        m_sentenceChunks[index] = translation;

        if (index < m_englishTranslations.size()) {
            m_englishTranslations[index] = translation;
        }

        if (translation.contains("[Translation Error")) {
            chunkLabel->setStyleSheet("color: red;");
        }

        if (index == m_currentChunkIndex) {
            updateChunkDisplay();
        }
    }
}

void MainWindow::showNextChunk() {
    if (m_germanChunks.isEmpty()) return;

    if (m_isEditingWordMode) {
        m_isEditingWordMode = false;
        wordOutputEditor->setReadOnly(true);
        if (btnEditWordEditor) btnEditWordEditor->setText("✏️");
    }

    resetSecondaryWordEditor();

    m_currentChunkIndex = (m_currentChunkIndex + 1) % m_germanChunks.size();
    m_showOriginalOrder = false;
    if (btnToggleSort) {
        btnToggleSort->setChecked(false);
    }
    updateChunkDisplay();
}

void MainWindow::showPreviousChunk() {
    if (m_germanChunks.isEmpty()) return;

    if (m_isEditingWordMode) {
        m_isEditingWordMode = false;
        wordOutputEditor->setReadOnly(true);
        if (btnEditWordEditor) {
            btnEditWordEditor->setText("✏️");
            btnEditWordEditor->setToolTip("Edit Word Explanations and Save to Database");
        }
    }

    resetSecondaryWordEditor();

    m_currentChunkIndex = (m_currentChunkIndex - 1 + m_germanChunks.size()) % m_germanChunks.size();
    m_showOriginalOrder = false;

    if (btnToggleSort) {
        btnToggleSort->setChecked(false);
    }
    updateChunkDisplay();
}

void MainWindow::resetSecondaryWordEditor() {
    if (wordOutputEditorSecondary && wordOutputEditorSecondary->isVisible()) {
        wordOutputEditorSecondary->hide();
        if (btnSplitWordEditor) {
            btnSplitWordEditor->setText("+");
        }
    }
}

void MainWindow::updateChunkDisplay() {
    if (m_currentChunkIndex < 0 || m_currentChunkIndex >= m_germanChunks.size()) return;

    if (m_currentChunkIndex < m_sentenceChunks.size()) {
        sentenceOutputEditor->setPlainText(m_sentenceChunks.at(m_currentChunkIndex));
    }

    QString explanationText;
    if (m_showOriginalOrder) {
        explanationText = m_germanChunks.at(m_currentChunkIndex);
    } else {
        if (m_currentChunkIndex < m_chunkedExerciseExplanations.size()) {
            const QList<QPair<QString, QString>> &list = m_chunkedExerciseExplanations.at(m_currentChunkIndex);
            for (const auto &pair : list) {
                explanationText += QString("%1 : %2\n\n").arg(pair.first, pair.second);
            }
        }
    }

    wordOutputEditor->setPlainText(explanationText);

    int total = m_germanChunks.size();
    chunkLabel->setText(QString("No.: %1 || %2").arg(m_currentChunkIndex + 1).arg(total));

    prevButton->setEnabled(total > 1);
    nextButton->setEnabled(total > 1);
}

void MainWindow::initialPiperProcess() {
    if (!m_piperProcess) {
        m_piperProcess = new QProcess(this);
    }

    if (m_piperProcess->state() == QProcess::NotRunning) {
        qDebug() << "[Piper Daemon] Relaunching background Speech Pipeline...";
        m_piperProcess->start("/home/Verya/piper/piper-start.sh");
        m_piperProcess->waitForStarted(500);
    }
}

bool MainWindow::writeToAudioPipe(const QString &pipePath, const QString &text) {
    initialPiperProcess();

    QFile pipeFile(pipePath);
    if (pipeFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QTextStream out(&pipeFile);
        out << text << "\n";
        out.flush();
        pipeFile.close();
        return true;
    }

    qDebug() << "[Audio Error] Cannot open Pipe:" << pipePath << pipeFile.errorString();
    return false;
}

void MainWindow::playCurrentAudio() {
    QString textToPlay;

    if (m_directInputVoiceMode) {
        stripComments();
        textToPlay = m_cleanedText;
    } else {
        if (m_currentChunkIndex >= 0 && m_currentChunkIndex < m_germanChunks.size()) {
            textToPlay = m_germanChunks.at(m_currentChunkIndex);
        }
    }

    if (textToPlay.trimmed().isEmpty()) return;

    QString targetLang = m_activeTranscriptionLanguage;
    if (targetLang != "es" && targetLang != "en") {
        targetLang = "de";
    }

    QString pipePath = "/tmp/piper_pipe_de";
    if (targetLang == "es") {
        pipePath = "/tmp/piper_pipe_es";
    } else if (targetLang == "en") {
        pipePath = "/tmp/piper_pipe_en";
    }

    writeToAudioPipe(pipePath, textToPlay);
}

void MainWindow::clearAll() {
    m_isEditingWordMode = false;
    wordOutputEditor->setReadOnly(true);
    if (btnEditWordEditor) {
        btnEditWordEditor->setText("✏️");
        btnEditWordEditor->setToolTip("Edit Word Explanations and Save to Database");
    }

    inputEditor->clear();
    sentenceOutputEditor->clear();
    wordOutputEditor->clear();

    m_germanChunks.clear();
    m_sentenceChunks.clear();
    m_englishTranslations.clear();
    m_currentChunkIndex = -1;
    m_chunkedOriginalExplanations.clear();
    m_chunkedExerciseExplanations.clear();

    stopAudioLoop();

    if (chunkLabel) {
        chunkLabel->setStyleSheet("");
        chunkLabel->setText("No.: 0 || 0");
    }
    prevButton->setEnabled(false);
    nextButton->setEnabled(false);

    if (statusLabel) {
        statusLabel->setStyleSheet("color: #00FF00;");
        statusLabel->setText("      ");
    }

    m_currentFilePath.clear();
    inputEditor->document()->setModified(false);
    updateWindowTitle();

    inputEditor->setFocus();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->key() == Qt::Key_Escape) {
            if (keyEvent->isAutoRepeat()) return true;
            if (m_isLooping) {
                stopAudioLoop();
                return true;
            }
        }

        if (keyEvent->key() == Qt::Key_F2) {
            if (keyEvent->isAutoRepeat()) return true;

            if (m_isLooping) {
                stopAudioLoop();
                return true;
            }

            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                toggleAudioLoop();
            } else {
                playCurrentAudio();
            }
            return true;
        }

        if (keyEvent->key() == Qt::Key_F3) {
            if (btnLookupOnly && btnLookupOnly->isEnabled()) {
                btnLookupOnly->animateClick();
                return true;
            }
        }

        if (keyEvent->modifiers() & Qt::ControlModifier) {
            switch (keyEvent->key()) {
            case Qt::Key_Left:
                if (prevButton && prevButton->isEnabled()) {
                    prevButton->animateClick();
                    return true;
                }
                break;

            case Qt::Key_Right:
                if (nextButton && nextButton->isEnabled()) {
                    nextButton->animateClick();
                    return true;
                }
                break;
            }
        }

        if (keyEvent->modifiers() & Qt::AltModifier) {
            switch (keyEvent->key()) {
            case Qt::Key_T:
                if (processButton && processButton->isEnabled()) {
                    processButton->animateClick();
                    return true;
                }
                break;

            case Qt::Key_C:
                if (clearButton && clearButton->isEnabled()) {
                    clearButton->animateClick();
                    return true;
                }
                break;

            case Qt::Key_S:
                if (btnToggleSort && btnToggleSort->isEnabled()) {
                    btnToggleSort->animateClick();
                    return true;
                }
                break;

            default:
                break;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupUiLayout() {
    qApp->installEventFilter(this);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *editorLayout = new QHBoxLayout();
    editorLayout->setSpacing(0);

    inputEditor = new QTextEdit(centralWidget);
    inputEditor->setObjectName("inputEditor");
    inputEditor->setPlaceholderText("Paste or type your German Sentences here...");

    rightOutputSplitter = new QSplitter(Qt::Vertical, centralWidget);

    QWidget *sentenceContainer = new QWidget(rightOutputSplitter);
    QVBoxLayout *sentenceLayout = new QVBoxLayout(sentenceContainer);
    sentenceLayout->setContentsMargins(0, 0, 0, 0);

    sentenceOutputEditor = new QPlainTextEdit(sentenceContainer);
    sentenceOutputEditor->setObjectName("sentenceOutputEditor");
    sentenceOutputEditor->setReadOnly(true);
    sentenceOutputEditor->setPlaceholderText("The fluid English Sentence Translation will appear here...");

    sentenceLayout->addWidget(new QLabel("Full Sentence Translation:", sentenceContainer));
    sentenceLayout->addWidget(sentenceOutputEditor);

    QWidget *wordContainer = new QWidget(rightOutputSplitter);
    QVBoxLayout *wordLayout = new QVBoxLayout(wordContainer);
    wordLayout->setContentsMargins(0, 0, 0, 0);
    wordLayout->setSpacing(4);

    QHBoxLayout *wordHeaderLayout = new QHBoxLayout();
    wordHeaderLayout->setContentsMargins(0, 0, 0, 0);

    btnTabSentenceWords = new QToolButton(wordContainer);
    btnTabSentenceWords->setObjectName("btnTabSentenceWords");
    btnTabSentenceWords->setText("Sentence Words");
    btnTabSentenceWords->setCheckable(true);
    btnTabSentenceWords->setChecked(true);
    btnTabSentenceWords->setCursor(Qt::PointingHandCursor);

    btnTabGlossary = new QToolButton(wordContainer);
    btnTabGlossary->setObjectName("btnTabGlossary");
    btnTabGlossary->setText("Full Glossary");
    btnTabGlossary->setCheckable(true);
    btnTabGlossary->setChecked(false);
    btnTabGlossary->setCursor(Qt::PointingHandCursor);

    connect(btnTabSentenceWords, &QToolButton::clicked, this, [this]() { switchWordPane(0); });
    connect(btnTabGlossary, &QToolButton::clicked, this, [this]() { switchWordPane(1); });

    wordHeaderLayout->addWidget(btnTabSentenceWords);
    wordHeaderLayout->addWidget(btnTabGlossary);
    wordHeaderLayout->addStretch();

    wordStackedWidget = new QStackedWidget(wordContainer);

    QSplitter *wordEditorSplitter = new QSplitter(Qt::Horizontal, wordStackedWidget);

    wordOutputEditor = new QPlainTextEdit(wordEditorSplitter);
    wordOutputEditor->setObjectName("wordOutputEditor");
    wordOutputEditor->setReadOnly(true);
    wordOutputEditor->setPlaceholderText("Word-by-word Dictionary Explanations will appear here...");

    QFont explanationFont = wordOutputEditor->font();
    explanationFont.setPointSize(explanationFont.pointSize() + 3);
    wordOutputEditor->setFont(explanationFont);

    wordOutputEditorSecondary = new QPlainTextEdit(wordEditorSplitter);
    wordOutputEditorSecondary->setObjectName("wordOutputEditorSecondary");
    wordOutputEditorSecondary->setReadOnly(true);
    wordOutputEditorSecondary->setFont(explanationFont);
    wordOutputEditorSecondary->hide();

    wordEditorSplitter->addWidget(wordOutputEditor);
    wordEditorSplitter->addWidget(wordOutputEditorSecondary);

    lookupOutputEditor = new QPlainTextEdit(wordStackedWidget);
    lookupOutputEditor->setObjectName("lookupOutputEditor");
    lookupOutputEditor->setReadOnly(true);
    lookupOutputEditor->setFont(explanationFont);
    lookupOutputEditor->setPlaceholderText("Full Document Vocabulary will appear here...");

    wordStackedWidget->addWidget(wordEditorSplitter);
    wordStackedWidget->addWidget(lookupOutputEditor);

    wordLayout->addLayout(wordHeaderLayout);
    wordLayout->addWidget(wordStackedWidget);

    initSplitWordEditorButton(wordHeaderLayout, wordEditorSplitter);

    rightOutputSplitter->addWidget(sentenceContainer);
    rightOutputSplitter->addWidget(wordContainer);
    rightOutputSplitter->setStretchFactor(0, 2);
    rightOutputSplitter->setStretchFactor(1, 3);

    editorLayout->addWidget(inputEditor, 4);

    QVBoxLayout *middleColumnLayout = new QVBoxLayout();
    middleColumnLayout->setContentsMargins(0, 0, 0, 0);
    middleColumnLayout->setSpacing(0);

    middleColumnLayout->addStretch(1);

    if (btnFileMenu) {
        middleColumnLayout->addWidget(btnFileMenu, 0, Qt::AlignHCenter);
    }

    middleColumnLayout->addSpacing(4);

    if (seamContainer) {
        middleColumnLayout->addWidget(seamContainer, 0, Qt::AlignHCenter);
    }

    middleColumnLayout->addStretch(1);

    editorLayout->addSpacing(8);
    editorLayout->addLayout(middleColumnLayout, 0);
    editorLayout->addSpacing(8);
    editorLayout->addWidget(rightOutputSplitter, 5);

    mainLayout->addLayout(editorLayout, 1);
    mainLayout->addSpacing(12);

    resize(900, 600);
}

void MainWindow::setupSeamButtonMenu() {
    seamContainer = new QFrame(centralWidget());
    seamContainer->setFrameShape(QFrame::NoFrame);
    seamContainer->setObjectName("seamContainer");
    seamContainer->setStyleSheet("QFrame#seamContainer { background-color: transparent; border: none; }");
    seamContainer->setAttribute(Qt::WA_TranslucentBackground, true);

    seamContainer->setFixedSize(17, 375);
    seamContainer->setMinimumSize(17, 375);
    seamContainer->setMaximumSize(17, 375);
    seamContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QVBoxLayout *seamLayout = new QVBoxLayout(seamContainer);
    seamLayout->setContentsMargins(0, 2, 0, 2);
    seamLayout->setSpacing(2);
}

void MainWindow::setupCollapseFeature() {
    btnCollapseLeft = new QToolButton(seamContainer);
    btnCollapseLeft->setText("◄");
    btnCollapseLeft->setToolTip("Restore Split View or Hide Left Panel");
    btnCollapseLeft->setCursor(Qt::PointingHandCursor);
    btnCollapseLeft->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    btnCollapseRight = new QToolButton(seamContainer);
    btnCollapseRight->setText("►");
    btnCollapseRight->setToolTip("Hide Right Panel and Navigation");
    btnCollapseRight->setCursor(Qt::PointingHandCursor);
    btnCollapseRight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (seamContainer && seamContainer->layout()) {
        QVBoxLayout *seamLayout = qobject_cast<QVBoxLayout*>(seamContainer->layout());
        if (seamLayout) {
            seamLayout->addWidget(btnCollapseLeft);
            seamLayout->addWidget(btnCollapseRight);
        }
    }

    connect(btnCollapseLeft, &QToolButton::clicked, this, [this]() {
        if (!rightOutputSplitter->isVisible()) {
            rightOutputSplitter->show();
            if (prevButton) prevButton->show();
            if (nextButton) nextButton->show();
            if (chunkLabel) chunkLabel->show();
            btnCollapseRight->setEnabled(true);
        } else if (inputEditor->isVisible()) {
            inputEditor->hide();
            btnCollapseLeft->setEnabled(false);
            btnCollapseRight->setEnabled(true);
        }
    });

    connect(btnCollapseRight, &QToolButton::clicked, this, [this]() {
        if (!inputEditor->isVisible()) {
            inputEditor->show();
            btnCollapseLeft->setEnabled(true);
        } else if (rightOutputSplitter->isVisible()) {
            rightOutputSplitter->hide();
            if (prevButton) prevButton->hide();
            if (nextButton) nextButton->hide();
            if (chunkLabel) chunkLabel->hide();
            btnCollapseRight->setEnabled(false);
            btnCollapseLeft->setEnabled(true);
        }
    });
}

void MainWindow::setupSortFeature() {
    btnToggleSort = new QToolButton(seamContainer);
    btnToggleSort->setText("⇅");
    btnToggleSort->setCheckable(true);
    btnToggleSort->setToolTip("Toggle Original / Exercise Word Order");
    btnToggleSort->setCursor(Qt::PointingHandCursor);
    btnToggleSort->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (seamContainer && seamContainer->layout()) {
        QVBoxLayout *seamLayout = qobject_cast<QVBoxLayout*>(seamContainer->layout());
        if (seamLayout) {
            seamLayout->addWidget(btnToggleSort);
        }
    }

    connect(btnToggleSort, &QToolButton::clicked, this, [this](bool checked) {
        m_showOriginalOrder = checked;
        updateChunkDisplay();
    });
}

void MainWindow::setupVoiceModeFeature() {
    btnToggleVoiceMode = new QToolButton(seamContainer);
    btnToggleVoiceMode->setObjectName("btnToggleVoiceMode");
    btnToggleVoiceMode->setText("🎤");
    btnToggleVoiceMode->setCheckable(true);
    btnToggleVoiceMode->setToolTip("Toggle Audio Mode: Chunked Translation vs Direct Input Scan");
    btnToggleVoiceMode->setCursor(Qt::PointingHandCursor);
    btnToggleVoiceMode->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (seamContainer && seamContainer->layout()) {
        QVBoxLayout *seamLayout = qobject_cast<QVBoxLayout*>(seamContainer->layout());
        if (seamLayout) {
            seamLayout->addWidget(btnToggleVoiceMode);
        }
    }

    connect(btnToggleVoiceMode, &QToolButton::toggled, this, [this](bool checked) {
        m_directInputVoiceMode = checked;
    });
}

void MainWindow::setupAudioPlaybackFeature() {
    btnPlayAudio = new QToolButton(seamContainer);
    btnPlayAudio->setText("🔊");
    btnPlayAudio->setToolTip("Play Sentence (Click) / Continuous Loop (Shift+Click)");
    btnPlayAudio->setCursor(Qt::PointingHandCursor);
    btnPlayAudio->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (seamContainer && seamContainer->layout()) {
        QVBoxLayout *seamLayout = qobject_cast<QVBoxLayout*>(seamContainer->layout());
        if (seamLayout) {
            seamLayout->addWidget(btnPlayAudio);
        }
    }

    m_loopAudioTimer = new QTimer(this);
    m_loopAudioTimer->setSingleShot(true);
    connect(m_loopAudioTimer, &QTimer::timeout, this, &MainWindow::playNextLoopToken);

    connect(btnPlayAudio, &QToolButton::clicked, this, [this]() {
        if (m_isLooping) {
            stopAudioLoop();
            return;
        }

        if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
            toggleAudioLoop();
        } else {
            playCurrentAudio();
        }
    });
}

void MainWindow::toggleAudioLoop() {
    if (m_isLooping) {
        stopAudioLoop();
        return;
    }

    stripComments();
    QStringList rawLines = m_cleanedText.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
    m_loopTokens.clear();

    for (const QString &line : rawLines) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            m_loopTokens.append(trimmed);
        }
    }

    if (m_loopTokens.isEmpty()) return;

    m_isLooping = true;
    m_currentLoopIndex = 0;

    if (btnPlayAudio) {
        btnPlayAudio->setText("🔁");
        btnPlayAudio->setStyleSheet("color: #00FF00;");
    }

    playNextLoopToken();
}

void MainWindow::playNextLoopToken() {
    if (!m_isLooping || m_loopTokens.isEmpty() || !m_loopAudioTimer) return;

    QString currentToken = m_loopTokens.at(m_currentLoopIndex);

    if (!currentToken.endsWith('.') && !currentToken.endsWith('!') && !currentToken.endsWith('?')) {
        currentToken += ".";
    }

    QString targetLang = m_activeTranscriptionLanguage;
    if (targetLang != "es" && targetLang != "en") {
        targetLang = "de";
    }

    QString pipePath = "/tmp/piper_pipe_de";
    if (targetLang == "es") {
        pipePath = "/tmp/piper_pipe_es";
    } else if (targetLang == "en") {
        pipePath = "/tmp/piper_pipe_en";
    }

    writeToAudioPipe(pipePath, currentToken);

    m_currentLoopIndex = (m_currentLoopIndex + 1) % m_loopTokens.size();

    int dynamicDelay = qMax(1300, currentToken.length() * 85 + 750);
    m_loopAudioTimer->start(dynamicDelay);
}

void MainWindow::stopAudioLoop() {
    m_isLooping = false;

    if (m_loopAudioTimer) {
        m_loopAudioTimer->stop();
    }
    m_loopTokens.clear();
    m_currentLoopIndex = 0;

    if (btnPlayAudio) {
        btnPlayAudio->setText("🔊");
        btnPlayAudio->setStyleSheet("");
    }
}

void MainWindow::setupTranslationFeature() {
    translationThread = new QThread(this);
    translationEngine = new TranslationEngine();
    translationEngine->moveToThread(translationThread);

    connect(this, &MainWindow::initTranslation, translationEngine, &TranslationEngine::initializeEngine);
    connect(this, &MainWindow::operateTranslation, translationEngine, &TranslationEngine::translateChunksAsync);
    connect(translationEngine, &TranslationEngine::chunkTranslationFinished, this, &MainWindow::handleChunkTranslationFinished);
    connect(translationEngine, &TranslationEngine::batchTranslationFinished, this, &MainWindow::handleBatchFinished);
    connect(translationThread, &QThread::finished, translationEngine, &QObject::deleteLater);

    translationThread->start();
}

void MainWindow::setupWordExplanationsFeature() {
    qRegisterMetaType<QList<QPair<QString, QString>>>("QList<QPair<QString,QString>>");

    dictionaryThread = new QThread(this);
    dictionaryWorker = new DictionaryWorker();
    dictionaryWorker->moveToThread(dictionaryThread);

    connect(this, &MainWindow::initDictionary, dictionaryWorker, &DictionaryWorker::initializeDatabase);
    connect(this, &MainWindow::operateLookup, dictionaryWorker, &DictionaryWorker::processChunks);
    connect(this, &MainWindow::operateWordUpdate, dictionaryWorker, &DictionaryWorker::updateWordEntries);

    connect(this, &MainWindow::operatePureLookup, dictionaryWorker, &DictionaryWorker::processPureLookup);
    connect(dictionaryWorker, &DictionaryWorker::pureLookupFinished, this, &MainWindow::handlePureLookupFinished);

    connect(this, &MainWindow::operatePureWordUpdate, dictionaryWorker, &DictionaryWorker::updatePureWordEntries);
    connect(dictionaryWorker, &DictionaryWorker::pureWordUpdateFinished, this, &MainWindow::handlePureWordUpdateFinished);

    connect(dictionaryWorker, &DictionaryWorker::lookupChunkFinished, this, &MainWindow::handleLookupChunkFinished);
    connect(dictionaryWorker, &DictionaryWorker::wordUpdateFinished, this, &MainWindow::handleWordUpdateFinished);
    connect(dictionaryThread, &QThread::finished, dictionaryWorker, &QObject::deleteLater);

    dictionaryThread->start();

    QTimer::singleShot(100, this, [this]() {
        emit initDictionary();
    });
}

QHBoxLayout* MainWindow::setupActionControlFeature() {
    QHBoxLayout *leftBottomLayout = new QHBoxLayout();

    clearButton = new QPushButton("C", centralWidget());
    clearButton->setMinimumHeight(28);
    clearButton->setMinimumWidth(50);
    clearButton->setToolTip("Clear Input and Output Editors (Alt+C)");

    statusLabel = new QLabel("         ", centralWidget());
    statusLabel->setStyleSheet("color: #00FF00;");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setMargin(10);

    processButton = new QPushButton("T", centralWidget());
    processButton->setMinimumHeight(28);
    processButton->setMinimumWidth(50);
    processButton->setToolTip("Translate German Sentences (Alt+T)");

    leftBottomLayout->addStretch();
    leftBottomLayout->addWidget(clearButton);
    leftBottomLayout->addWidget(statusLabel);
    leftBottomLayout->addWidget(processButton);
    leftBottomLayout->addStretch();

    connect(processButton, &QPushButton::clicked, this, &MainWindow::processText);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearAll);

    return leftBottomLayout;
}

QHBoxLayout* MainWindow::setupNavigationControlFeature() {
    QHBoxLayout *rightBottomLayout = new QHBoxLayout();

    prevButton = new QPushButton("⇤", centralWidget());
    prevButton->setMinimumHeight(28);
    prevButton->setMinimumWidth(50);

    nextButton = new QPushButton("⇥", centralWidget());
    nextButton->setMinimumHeight(28);
    nextButton->setMinimumWidth(50);

    chunkLabel = new QLabel("No.: 0 || 0", centralWidget());
    chunkLabel->setAlignment(Qt::AlignCenter);
    chunkLabel->setMargin(15);

    rightBottomLayout->addStretch();
    rightBottomLayout->addWidget(prevButton);
    rightBottomLayout->addWidget(chunkLabel);
    rightBottomLayout->addWidget(nextButton);
    rightBottomLayout->addStretch();

    prevButton->setEnabled(false);
    nextButton->setEnabled(false);

    connect(prevButton, &QPushButton::clicked, this, &MainWindow::showPreviousChunk);
    connect(nextButton, &QPushButton::clicked, this, &MainWindow::showNextChunk);

    return rightBottomLayout;
}

void MainWindow::setupShortcutsAndEvents() {
    QShortcut *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, &MainWindow::saveFile);

    QShortcut *saveAsShortcut = new QShortcut(QKeySequence::SaveAs, this);
    connect(saveAsShortcut, &QShortcut::activated, this, &MainWindow::saveFileAs);

    QShortcut *openShortcut = new QShortcut(QKeySequence::Open, this);
    connect(openShortcut, &QShortcut::activated, this, &MainWindow::openFile);

    connect(inputEditor->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateWindowTitle);
}

void MainWindow::saveFile() {
    if (m_currentFilePath.isEmpty()) {
        saveFileAs();
    } else {
        writeFile(m_currentFilePath);
    }
}

void MainWindow::saveFileAs() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Save German Text", m_currentFilePath, "Text Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    m_currentFilePath = fileName;
    writeFile(m_currentFilePath);
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(
        this, "Open German Text", "", "Text Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    loadFile(fileName);
}

void MainWindow::loadFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (statusLabel) {
            statusLabel->setStyleSheet("color: red;");
            statusLabel->setText("Cannot read File.");
        }
        return;
    }

    QTextStream in(&file);
    inputEditor->setPlainText(in.readAll());
    file.close();

    m_currentFilePath = filePath;
    inputEditor->document()->setModified(false);
    updateWindowTitle();

    if (m_recentFilesManager) {
        m_recentFilesManager->addFile(filePath);
    }

    if (statusLabel) {
        statusLabel->setStyleSheet("color: #00FF00;");
        statusLabel->setText("Loaded");
    }
}

bool MainWindow::writeFile(const QString &filePath) {
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (statusLabel) {
            statusLabel->setStyleSheet("color: red;");
            statusLabel->setText("Cannot write File.");
        }
        return false;
    }

    QTextStream out(&file);
    out << inputEditor->toPlainText();

    if (!file.commit()) {
        if (statusLabel) {
            statusLabel->setStyleSheet("color: red;");
            statusLabel->setText("Commit failed.");
        }
        return false;
    }

    inputEditor->document()->setModified(false);
    updateWindowTitle();

    if (m_recentFilesManager) {
        m_recentFilesManager->addFile(filePath);
    }

    if (statusLabel) {
        statusLabel->setStyleSheet("color: #00FF00;");
        statusLabel->setText("Saved: " + QFileInfo(filePath).fileName());
    }
    return true;
}

void MainWindow::updateWindowTitle() {
    QString title = "Prussiadriver";
    if (!m_currentFilePath.isEmpty()) {
        title += " - " + QFileInfo(m_currentFilePath).fileName();
    } else {
        title += " - Untitled";
    }

    if (inputEditor->document()->isModified()) {
        title += " [*]";
    }

    setWindowTitle(title);
    setWindowModified(inputEditor->document()->isModified());
}

void MainWindow::setupBottomControlFeature() {
    QWidget *central = centralWidget();
    if (!central || !central->layout()) return;

    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(central->layout());
    if (!mainLayout) return;

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addLayout(setupActionControlFeature(), 4);
    bottomLayout->addLayout(setupNavigationControlFeature(), 5);

    mainLayout->addSpacing(15);
    mainLayout->addLayout(bottomLayout, 0);
}

void MainWindow::setupSpeechRecognitionFeature() {
    m_audioThread = new QThread(this);
    m_audioWorker = new AudioTranscriptionWorker();
    m_audioWorker->moveToThread(m_audioThread);

    connect(this, &MainWindow::operateTranscription,
            m_audioWorker, &AudioTranscriptionWorker::processTranscription);

    connect(m_audioWorker, &AudioTranscriptionWorker::transcriptionFinished,
            this, &MainWindow::onTranscriptionFinished);

    connect(m_audioThread, &QThread::finished,
            m_audioWorker, &QObject::deleteLater);

    m_audioThread->start();
}

void MainWindow::initTranscribeButton() {
    btnWhisperGerman = new QToolButton(seamContainer);
    btnWhisperGerman->setText("🎙️\n🇩🇪");
    btnWhisperGerman->setCheckable(true);
    btnWhisperGerman->setToolTip("German Voice Input: Click to Start / Stop");
    btnWhisperGerman->setCursor(Qt::PointingHandCursor);
    btnWhisperGerman->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    btnWhisperGerman->setObjectName("btnWhisperGerman");

    btnWhisperEnglish = new QToolButton(seamContainer);
    btnWhisperEnglish->setText("🎙️\n🇬🇧");
    btnWhisperEnglish->setCheckable(true);
    btnWhisperEnglish->setToolTip("English Voice Input: Click to Start / Stop");
    btnWhisperEnglish->setCursor(Qt::PointingHandCursor);
    btnWhisperEnglish->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    btnWhisperEnglish->setObjectName("btnWhisperEnglish");

    btnWhisperSpanish = new QToolButton(seamContainer);
    btnWhisperSpanish->setText("🎙️\n🇪🇸");
    btnWhisperSpanish->setCheckable(true);
    btnWhisperSpanish->setToolTip("Spanish Voice Input: Click to Start / Stop");
    btnWhisperSpanish->setCursor(Qt::PointingHandCursor);
    btnWhisperSpanish->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    btnWhisperSpanish->setObjectName("btnWhisperSpanish");

    m_listeningAnimationTimer = new QTimer(this);
    m_listeningDotCount = 1;

    if (m_listeningAnimationTimer) {
        connect(m_listeningAnimationTimer, &QTimer::timeout, this, [this]() {
            if (!inputEditor) return;
            QString dots = QString(".").repeated(m_listeningDotCount);
            inputEditor->setPlaceholderText("Listening" + dots);
            m_listeningDotCount = (m_listeningDotCount % 3) + 1;
        });
    }

    if (seamContainer && seamContainer->layout()) {
        QVBoxLayout *seamLayout = qobject_cast<QVBoxLayout*>(seamContainer->layout());
        if (seamLayout) {
            seamLayout->addWidget(btnWhisperGerman);
            seamLayout->addWidget(btnWhisperEnglish);
            seamLayout->addWidget(btnWhisperSpanish);
        }
    }

    auto handleToggleState = [this](QToolButton *activeBtn, const QString &languageCode) {
        QList<QToolButton*> allWhisperButtons = {btnWhisperGerman, btnWhisperEnglish, btnWhisperSpanish};

        if (activeBtn->isChecked()) {
            m_activeTranscriptionLanguage = languageCode;

            for (auto *btn : allWhisperButtons) {
                if (btn && btn != activeBtn) {
                    btn->setChecked(false);
                }
            }

            m_listeningDotCount = 1;
            if (inputEditor) {
                inputEditor->setPlaceholderText("Listening (" + languageCode + ").");
            }
            if (m_listeningAnimationTimer) {
                m_listeningAnimationTimer->start(500);
            }

            startRecordingAudio();
        } else {
            if (m_listeningAnimationTimer) {
                m_listeningAnimationTimer->stop();
            }
            if (inputEditor) {
                inputEditor->setPlaceholderText("Transcribing audio, please wait...");
            }

            stopRecordingAudio();
        }
    };

    connect(btnWhisperGerman, &QToolButton::clicked, this, [handleToggleState, this]() {
        handleToggleState(btnWhisperGerman, "de");
    });

    connect(btnWhisperEnglish, &QToolButton::clicked, this, [handleToggleState, this]() {
        handleToggleState(btnWhisperEnglish, "en");
    });

    connect(btnWhisperSpanish, &QToolButton::clicked, this, [handleToggleState, this]() {
        handleToggleState(btnWhisperSpanish, "es");
    });
}

void MainWindow::setupAudioRecorder() {
    m_recordedAudioPath = "/tmp/whisper.wav";
    m_recordProcess = new QProcess(this);
    qDebug() << "[Recorder Debug] Audio Recorder initialized using arecord on hw:2,0.";
}

void MainWindow::startRecordingAudio() {
    QFile::remove("/tmp/raw.wav");
    QFile::remove(m_recordedAudioPath);

    if (m_recordProcess && m_recordProcess->state() != QProcess::NotRunning) {
        m_recordProcess->kill();
        m_recordProcess->waitForFinished(200);
    }

    QStringList args;
    args << "-D" << "hw:2,0"
         << "-f" << "S32_LE"
         << "-r" << "48000"
         << "-c" << "2"
         << "/tmp/raw.wav";

    qDebug() << "[Recorder] Starting Hardware Capture on hw:2,0...";
    m_recordProcess->start("arecord", args);
}

void MainWindow::stopRecordingAudio() {
    if (!m_recordProcess || m_recordProcess->state() == QProcess::NotRunning) {
        return;
    }

    qDebug() << "[Recorder] Stopping arecord Process...";
    m_recordProcess->terminate();
    if (!m_recordProcess->waitForFinished(1000)) {
        m_recordProcess->kill();
        m_recordProcess->waitForFinished(500);
    }

    QProcess *ffmpegProcess = new QProcess(this);
    QStringList ffmpegArgs;
    ffmpegArgs << "-y"
               << "-i" << "/tmp/raw.wav"
               << "-af" << "volume=12dB"
               << "-ar" << "16000"
               << "-ac" << "1"
               << "-sample_fmt" << "s16"
               << m_recordedAudioPath;

    connect(ffmpegProcess, &QProcess::finished, this, [this, ffmpegProcess](int exitCode, QProcess::ExitStatus) {
        ffmpegProcess->deleteLater();

        if (exitCode == 0 && QFile::exists(m_recordedAudioPath)) {
            QFileInfo fileInfo(m_recordedAudioPath);
            qDebug() << "[Recorder] Audio prepared successfully. Size:" << fileInfo.size() << "Bytes.";
            emit operateTranscription(m_recordedAudioPath, m_activeTranscriptionLanguage);
        } else {
            qDebug() << "[Recorder Error] Audio Conversion failed with Exit Code:" << exitCode;
        }
    });

    qDebug() << "[Recorder] Converting Audio via FFmpeg...";
    ffmpegProcess->start("ffmpeg", ffmpegArgs);
}

void MainWindow::onTranscriptionFinished(const QString &text) {
    inputEditor->setPlaceholderText("Paste or type your German Sentences here...");

    if (text.isEmpty()) {
        chunkLabel->setStyleSheet("color: red;");
        chunkLabel->setText("Whisper: No Audio detected or empty Transcription.");
        return;
    }

    inputEditor->append(text);
}

QString MainWindow::getEditorPanelStyle(const QString &accentColor) const {
    return QString(
               "QTextEdit, QPlainTextEdit {"
               "  background-color: #1e1e1e;"
               "  color: #e0e0e0;"
               "  border: 1px solid #333333;"
               "  border-left: 1px solid %1;"
               "  border-radius: 6px;"
               "  padding: 8px;"
               "  selection-background-color: #3e4451;"
               "}"
               "QTextEdit:focus, QPlainTextEdit:focus {"
               "  border-top: 1px solid %1;"
               "  border-right: 1px solid %1;"
               "  border-bottom: 1px solid %1;"
               "}"
               ).arg(accentColor);
}

void MainWindow::initCopyButton() {
    if (!btnSplitWordEditor) return;

    QWidget *wordContainer = btnSplitWordEditor->parentWidget();
    if (!wordContainer) return;

    QVBoxLayout *wordLayout = qobject_cast<QVBoxLayout*>(wordContainer->layout());
    if (!wordLayout || wordLayout->count() == 0) return;

    QHBoxLayout *wordHeaderLayout = qobject_cast<QHBoxLayout*>(wordLayout->itemAt(0)->layout());
    if (!wordHeaderLayout) return;

    btnCopyTranslation = new QToolButton(wordContainer);
    btnCopyTranslation->setText("📋");
    btnCopyTranslation->setToolTip("Copy Current Sentence Translation");
    btnCopyTranslation->setCursor(Qt::PointingHandCursor);

    int splitBtnIndex = wordHeaderLayout->indexOf(btnSplitWordEditor);
    if (splitBtnIndex != -1) {
        wordHeaderLayout->insertWidget(splitBtnIndex, btnCopyTranslation);
        wordHeaderLayout->insertSpacing(splitBtnIndex + 1, 4);
    } else {
        wordHeaderLayout->addWidget(btnCopyTranslation);
    }

    connect(btnCopyTranslation, &QToolButton::clicked, this, [this]() {
        if (m_currentChunkIndex >= 0 && m_currentChunkIndex < m_sentenceChunks.size()) {
            QString textToCopy = m_sentenceChunks.at(m_currentChunkIndex);
            if (!textToCopy.isEmpty()) {
                QGuiApplication::clipboard()->setText(textToCopy);

                btnCopyTranslation->setText("✓");
                QTimer::singleShot(1500, this, [this]() {
                    if (btnCopyTranslation) {
                        btnCopyTranslation->setText("📋");
                    }
                });
            }
        }
    });
}

void MainWindow::initSplitWordEditorButton(QHBoxLayout *wordHeaderLayout, QSplitter *wordEditorSplitter) {
    if (!wordHeaderLayout || !wordEditorSplitter) return;

    QWidget *wordContainer = wordHeaderLayout->parentWidget();

    btnSplitWordEditor = new QToolButton(wordContainer);
    btnSplitWordEditor->setText("+");
    btnSplitWordEditor->setToolTip("Split Editor Side-by-Side (Qt Creator Style)");
    btnSplitWordEditor->setCursor(Qt::PointingHandCursor);

    wordHeaderLayout->addWidget(btnSplitWordEditor);

    connect(btnSplitWordEditor, &QToolButton::clicked, this, [this, wordEditorSplitter]() {
        if (!wordOutputEditorSecondary || !btnSplitWordEditor) return;

        if (wordOutputEditorSecondary->isVisible()) {
            wordOutputEditorSecondary->hide();
            btnSplitWordEditor->setText("+");
        } else {
            wordOutputEditorSecondary->setPlainText(wordOutputEditor->toPlainText());
            wordOutputEditorSecondary->show();

            int totalWidth = wordEditorSplitter->width();
            if (totalWidth > 0) {
                wordEditorSplitter->setSizes({ totalWidth / 2, totalWidth / 2 });
            } else {
                wordEditorSplitter->setSizes({ 1, 1 });
            }

            btnSplitWordEditor->setText("−");
        }
    });
}

void MainWindow::initThemeToggleButton() {
    if (!seamContainer || !seamContainer->layout()) return;

    QVBoxLayout *seamLayout = qobject_cast<QVBoxLayout*>(seamContainer->layout());
    if (!seamLayout) return;

    btnToggleTheme = new QToolButton(seamContainer);
    btnToggleTheme->setText("🌙");
    btnToggleTheme->setToolTip("Toggle Dark / Light Theme");
    btnToggleTheme->setCursor(Qt::PointingHandCursor);
    btnToggleTheme->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    seamLayout->addWidget(btnToggleTheme);

    connect(btnToggleTheme, &QToolButton::clicked, this, [this]() {
        m_isDarkMode = !m_isDarkMode;
        applyTheme(m_isDarkMode);
    });
}

QString MainWindow::getSeamButtonStyle(bool isDark) const {
    if (isDark) {
        return "QToolButton {"
               "  background: transparent;"
               "  color: #888888;"
               "  border: none;"
               "  font-size: 13px;"
               "  font-weight: bold;"
               "}"
               "QToolButton:hover {"
               "  color: #ffffff;"
               "  background-color: #444444;"
               "}";
    } else {
        return "QToolButton {"
               "  background: transparent;"
               "  color: #444444;"
               "  border: none;"
               "  font-size: 13px;"
               "  font-weight: bold;"
               "}"
               "QToolButton:hover {"
               "  color: #000000;"
               "  background-color: #d0d0d0;"
               "}";
    }
}

void MainWindow::applyTheme(bool isDark) {
    applySystemTheme(isDark);
}

void MainWindow::applySystemTheme(bool isDark) {
    m_isDarkMode = isDark;

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QGuiApplication::styleHints()->setColorScheme(
        isDark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light
        );

    QPalette windowPalette;
    if (isDark) {
        windowPalette.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
        windowPalette.setColor(QPalette::WindowText, QColor(0xe0, 0xe0, 0xe0));
        windowPalette.setColor(QPalette::Base, QColor(0x25, 0x25, 0x26));
        windowPalette.setColor(QPalette::Text, QColor(0xe0, 0xe0, 0xe0));
        windowPalette.setColor(QPalette::Button, QColor(0x2d, 0x2d, 0x2d));
        windowPalette.setColor(QPalette::ButtonText, QColor(0xe0, 0xe0, 0xe0));
    } else {
        windowPalette.setColor(QPalette::Window, QColor(0xf5, 0xf5, 0xf5));
        windowPalette.setColor(QPalette::WindowText, QColor(0x22, 0x22, 0x22));
        windowPalette.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
        windowPalette.setColor(QPalette::Text, QColor(0x22, 0x22, 0x22));
        windowPalette.setColor(QPalette::Button, QColor(0xff, 0xff, 0xff));
        windowPalette.setColor(QPalette::ButtonText, QColor(0x22, 0x22, 0x22));
    }
    qApp->setPalette(windowPalette);

    if (btnToggleTheme) {
        btnToggleTheme->setText(isDark ? "🌙" : "☀️");
    }

    QString themeQss;
    if (isDark) {
        themeQss =
            "QMainWindow { background-color: #1e1e1e; color: #e0e0e0; }"
            "QLabel { color: #e0e0e0; }"
            "#btnFileMenu {"
            "  background-color: #2b2b2b;"
            "  color: #dcdcdc;"
            "  border: 1px solid #3c3c3c;"
            "  border-radius: 3px;"
            "  padding: 1px 6px;"
            "  min-height: 18px;"
            "  max-height: 18px;"
            "  font-size: 10px;"
            "  font-weight: normal;"
            "}"
            "#btnFileMenu:hover {"
            "  background-color: #383838;"
            "  color: #ffffff;"
            "  border-color: #555555;"
            "}"
            "#btnFileMenu:pressed {"
            "  background-color: #1f1f1f;"
            "}"
            "QMenu {"
            "  background-color: #252526;"
            "  color: #cccccc;"
            "  border: 1px solid #3f3f46;"
            "  padding: 4px;"
            "  border-radius: 5px;"
            "}"
            "QMenu::item {"
            "  padding: 5px 22px 5px 18px;"
            "  border-radius: 3px;"
            "}"
            "QMenu::item:selected {"
            "  background-color: #094771;"
            "  color: #ffffff;"
            "}"
            "QMenu::separator {"
            "  height: 1px;"
            "  background-color: #3f3f46;"
            "  margin: 4px 6px;"
            "}"
            "#btnTabSentenceWords, #btnTabGlossary {"
            "  background-color: transparent;"
            "  color: #888888;"
            "  border: none;"
            "  font-size: 11px;"
            "  font-weight: bold;"
            "  padding: 3px 8px;"
            "  border-radius: 4px;"
            "}"
            "#btnTabSentenceWords:hover, #btnTabGlossary:hover {"
            "  background-color: #2e2e2e;"
            "  color: #ffffff;"
            "}"
            "#btnTabSentenceWords:checked, #btnTabGlossary:checked {"
            "  background-color: #383838;"
            "  color: #ffffff;"
            "}"
            "QFrame#seamContainer { background-color: transparent; border: none; }"
            "QFrame#seamContainer QToolButton {"
            "  background-color: #2b2b2b;"
            "  color: #a0a0a0;"
            "  border: 1px solid #3c3c3c;"
            "  border-radius: 6px;"
            "  min-height: 28px;"
            "  font-size: 11px;"
            "  padding: 2px 0px;"
            "}"
            "QFrame#seamContainer QToolButton:hover { background-color: #383838; color: #ffffff; border-color: #555555; }"
            "QFrame#seamContainer QToolButton:pressed { background-color: #222222; }"
            "QTextEdit, QPlainTextEdit {"
            "  background-color: #1e1e1e;"
            "  color: #e0e0e0;"
            "  border: 1px solid #333333;"
            "  border-radius: 6px;"
            "  padding: 8px;"
            "  selection-background-color: #3e4451;"
            "}"
            "QPushButton { background-color: #2b2b2b; color: #e0e0e0; border: 1px solid #444444; border-radius: 8px; padding: 6px 12px; font-weight: bold; }"
            "QPushButton:hover { background-color: #383838; border-color: #555555; }"
            "QPushButton:pressed { background-color: #1f1f1f; }"
            "QPushButton:disabled { background-color: #1a1a1a; color: #555555; border-color: #2a2a2a; }"
            "QToolButton { background: transparent; color: #888888; border: none; font-size: 13px; font-weight: bold; }"
            "QToolButton:hover { color: #ffffff; background-color: #444444; }"
            "#btnToggleVoiceMode:checked { background-color: #2e7d32; color: #ffffff; border: none; }"
            "#btnToggleVoiceMode:checked:hover { background-color: #388e3c; }"
            "#btnWhisperGerman:checked, #btnWhisperEnglish:checked, #btnWhisperSpanish:checked { background-color: #7b1fa2; color: #ffffff; border: none; }"
            "#btnWhisperGerman:checked:hover, #btnWhisperEnglish:checked:hover, #btnWhisperSpanish:checked:hover { background-color: #8e24aa; }"
            "QSplitter::handle:vertical { background-color: #333333; height: 4px; margin: 2px 0px; }"
            "QSplitter::handle:vertical:hover { background-color: #4a90e2; }";
    } else {
        themeQss =
            "QMainWindow { background-color: #f5f5f5; color: #222222; }"
            "QLabel { color: #222222; }"
            "#btnFileMenu {"
            "  background-color: #ffffff;"
            "  color: #222222;"
            "  border: 1px solid #cccccc;"
            "  border-radius: 3px;"
            "  padding: 1px 6px;"
            "  min-height: 18px;"
            "  max-height: 18px;"
            "  font-size: 10px;"
            "  font-weight: normal;"
            "}"
            "#btnFileMenu:hover {"
            "  background-color: #eaeaea;"
            "  color: #000000;"
            "  border-color: #bbbbbb;"
            "}"
            "#btnFileMenu:pressed {"
            "  background-color: #d8d8d8;"
            "}"
            "QMenu {"
            "  background-color: #ffffff;"
            "  color: #222222;"
            "  border: 1px solid #cccccc;"
            "  padding: 4px;"
            "  border-radius: 5px;"
            "}"
            "QMenu::item {"
            "  padding: 5px 22px 5px 18px;"
            "  border-radius: 3px;"
            "}"
            "QMenu::item:selected {"
            "  background-color: #0078d7;"
            "  color: #ffffff;"
            "}"
            "QMenu::separator {"
            "  height: 1px;"
            "  background-color: #e0e0e0;"
            "  margin: 4px 6px;"
            "}"
            "#btnTabSentenceWords, #btnTabGlossary {"
            "  background-color: transparent;"
            "  color: #666666;"
            "  border: none;"
            "  font-size: 11px;"
            "  font-weight: bold;"
            "  padding: 3px 8px;"
            "  border-radius: 4px;"
            "}"
            "#btnTabSentenceWords:hover, #btnTabGlossary:hover {"
            "  background-color: #e2e2e2;"
            "  color: #000000;"
            "}"
            "#btnTabSentenceWords:checked, #btnTabGlossary:checked {"
            "  background-color: #d6d6d6;"
            "  color: #000000;"
            "}"
            "QFrame#seamContainer { background-color: transparent; border: none; }"
            "QFrame#seamContainer QToolButton {"
            "  background-color: #ffffff;"
            "  color: #555555;"
            "  border: 1px solid #dcdcdc;"
            "  border-radius: 6px;"
            "  min-height: 28px;"
            "  font-size: 11px;"
            "  padding: 2px 0px;"
            "}"
            "QFrame#seamContainer QToolButton:hover { background-color: #eaeaea; color: #000000; border-color: #bbbbbb; }"
            "QFrame#seamContainer QToolButton:pressed { background-color: #d8d8d8; }"
            "QTextEdit, QPlainTextEdit {"
            "  background-color: #ffffff;"
            "  color: #1e1e1e;"
            "  border: 1px solid #cccccc;"
            "  border-radius: 6px;"
            "  padding: 8px;"
            "  selection-background-color: #b5d5ff;"
            "}"
            "QPushButton { background-color: #ffffff; color: #1e1e1e; border: 1px solid #cccccc; border-radius: 8px; padding: 6px 12px; font-weight: bold; }"
            "QPushButton:hover { background-color: #e8e8e8; border-color: #bbbbbb; }"
            "QPushButton:pressed { background-color: #d0d0d0; }"
            "QPushButton:disabled { background-color: #f0f0f0; color: #aaaaaa; border-color: #dddddd; }"
            "QToolButton { background: transparent; color: #444444; border: none; font-size: 13px; font-weight: bold; }"
            "QToolButton:hover { color: #000000; background-color: #d0d0d0; }"
            "#btnToggleVoiceMode:checked { background-color: #2e7d32; color: #ffffff; border: none; }"
            "#btnToggleVoiceMode:checked:hover { background-color: #388e3c; }"
            "#btnWhisperGerman:checked, #btnWhisperEnglish:checked, #btnWhisperSpanish:checked { background-color: #7b1fa2; color: #ffffff; border: none; }"
            "#btnWhisperGerman:checked:hover, #btnWhisperEnglish:checked:hover, #btnWhisperSpanish:checked:hover { background-color: #8e24aa; }"
            "QSplitter::handle:vertical { background-color: #cccccc; height: 4px; margin: 2px 0px; }"
            "QSplitter::handle:vertical:hover { background-color: #1976d2; }";
    }

    this->setStyleSheet(themeQss);
}

void MainWindow::initLookupOnlyButton() {
    btnLookupOnly = new QToolButton(seamContainer);
    btnLookupOnly->setText("📖");
    btnLookupOnly->setToolTip("Lookup all Words in Database (Single Page, No Translation)");
    btnLookupOnly->setCursor(Qt::PointingHandCursor);
    btnLookupOnly->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (seamContainer && seamContainer->layout()) {
        QVBoxLayout *seamLayout = qobject_cast<QVBoxLayout*>(seamContainer->layout());
        if (seamLayout) {
            seamLayout->addWidget(btnLookupOnly);
        }
    }

    connect(btnLookupOnly, &QToolButton::clicked, this, &MainWindow::processLookupOnly);
}

void MainWindow::processLookupOnly() {
    stripComments();
    if (m_cleanedText.isEmpty()) return;

    switchWordPane(1);
    lookupOutputEditor->setPlainText("Searching Database for Document Vocabulary...");
    emit operatePureLookup(m_cleanedText);
}

void MainWindow::stripComments() {
    static const QRegularExpression multiLineCommentRegex("/\\*[\\s\\S]*?\\*/");
    static const QRegularExpression singleLineCommentRegex("//[^\\r\\n]*");

    m_cleanedText = inputEditor->toPlainText();
    m_cleanedText.replace(multiLineCommentRegex, " ");
    m_cleanedText.replace(singleLineCommentRegex, " ");
    m_cleanedText = m_cleanedText.trimmed();
}

void MainWindow::initEditWordButton() {
    if (!btnSplitWordEditor) return;

    QWidget *wordContainer = btnSplitWordEditor->parentWidget();
    if (!wordContainer) return;

    QVBoxLayout *wordLayout = qobject_cast<QVBoxLayout*>(wordContainer->layout());
    if (!wordLayout || wordLayout->count() == 0) return;

    QHBoxLayout *wordHeaderLayout = qobject_cast<QHBoxLayout*>(wordLayout->itemAt(0)->layout());
    if (!wordHeaderLayout) return;

    btnEditWordEditor = new QToolButton(wordContainer);
    btnEditWordEditor->setText("✏️");
    btnEditWordEditor->setToolTip("Edit Word Explanations and Save to Database");
    btnEditWordEditor->setCursor(Qt::PointingHandCursor);

    int targetIndex = btnCopyTranslation ? wordHeaderLayout->indexOf(btnCopyTranslation)
                                         : wordHeaderLayout->indexOf(btnSplitWordEditor);
    if (targetIndex != -1) {
        wordHeaderLayout->insertWidget(targetIndex, btnEditWordEditor);
        wordHeaderLayout->insertSpacing(targetIndex + 1, 4);
    } else {
        wordHeaderLayout->addWidget(btnEditWordEditor);
    }

    connect(btnEditWordEditor, &QToolButton::clicked, this, [this]() {
        int activeIndex = wordStackedWidget ? wordStackedWidget->currentIndex() : 0;
        QPlainTextEdit *targetEditor = (activeIndex == 1) ? lookupOutputEditor : wordOutputEditor;
        if (!targetEditor) return;

        if (!m_isEditingWordMode) {
            if (activeIndex == 0 && m_showOriginalOrder) {
                m_showOriginalOrder = false;
                if (btnToggleSort) btnToggleSort->setChecked(false);
                updateChunkDisplay();
            }

            m_isEditingWordMode = true;
            targetEditor->setReadOnly(false);
            targetEditor->setFocus();
            btnEditWordEditor->setText("💾");
            btnEditWordEditor->setToolTip("Save Changes to Database");
        } else {
            m_isEditingWordMode = false;
            targetEditor->setReadOnly(true);

            QString content = targetEditor->toPlainText();
            QStringList rawLines = content.split('\n');
            QList<QPair<QString, QString>> updatedList;

            QString currentWord;
            QString currentDef;

            for (const QString &rawLine : rawLines) {
                QString trimmedLine = rawLine.trimmed();
                if (trimmedLine.isEmpty()) continue;

                int colonIndex = trimmedLine.indexOf(':');
                if (colonIndex != -1) {
                    if (!currentWord.isEmpty() && !currentDef.isEmpty()) {
                        updatedList.append(qMakePair(currentWord, currentDef));
                    }
                    currentWord = trimmedLine.left(colonIndex).trimmed();
                    currentDef = trimmedLine.mid(colonIndex + 1).trimmed();
                } else {
                    if (!currentWord.isEmpty()) {
                        if (!currentDef.isEmpty()) currentDef += " ";
                        currentDef += trimmedLine;
                    }
                }
            }

            if (!currentWord.isEmpty() && !currentDef.isEmpty()) {
                updatedList.append(qMakePair(currentWord, currentDef));
            }

            if (activeIndex == 0) {
                if (m_currentChunkIndex >= 0 && m_currentChunkIndex < m_chunkedExerciseExplanations.size()) {
                    m_chunkedExerciseExplanations[m_currentChunkIndex] = updatedList;
                    m_chunkedOriginalExplanations[m_currentChunkIndex] = updatedList;
                }
                emit operateWordUpdate(m_currentChunkIndex, updatedList);
            } else {
                emit operatePureWordUpdate(updatedList);
            }

            btnEditWordEditor->setText("✓");
            QTimer::singleShot(1200, this, [this]() {
                if (btnEditWordEditor) {
                    btnEditWordEditor->setText("✏️");
                    btnEditWordEditor->setToolTip("Edit Word Explanations and Save to Database");
                }
            });
        }
    });
}

void MainWindow::handleWordUpdateFinished(bool success) {
    if (statusLabel) {
        statusLabel->setStyleSheet(success ? "color: #00FF00;" : "color: red;");
        statusLabel->setText(success ? "Word updated" : "Update failed");
        QTimer::singleShot(2000, this, [this]() {
            if (statusLabel) {
                statusLabel->setStyleSheet("color: #00FF00;");
                statusLabel->setText("Ready");
            }
        });
    }
}

void MainWindow::initFileMenuButton() {
    btnFileMenu = new QToolButton(this);
    btnFileMenu->setObjectName("btnFileMenu");
    btnFileMenu->setText("&File");
    btnFileMenu->setToolTip("File Operations (Alt+F)");
    btnFileMenu->setCursor(Qt::PointingHandCursor);

    connect(btnFileMenu, &QToolButton::clicked, this, [this]() {
        if (!m_fileMenu || !btnFileMenu) return;
        QPoint popupPos = btnFileMenu->mapToGlobal(QPoint(0, btnFileMenu->height() + 3));
        m_fileMenu->exec(popupPos);
    });
}

void MainWindow::setupFileMenu() {
    m_fileMenu = new QMenu(this);

    QAction *actNew = m_fileMenu->addAction("📄  New File");
    actNew->setShortcut(QKeySequence::New);
    connect(actNew, &QAction::triggered, this, &MainWindow::handleNewFile);

    QAction *actOpen = m_fileMenu->addAction("📂  Open File...");
    actOpen->setShortcut(QKeySequence::Open);
    connect(actOpen, &QAction::triggered, this, &MainWindow::handleOpenFile);

    QAction *actSave = m_fileMenu->addAction("💾  Save");
    actSave->setShortcut(QKeySequence::Save);
    connect(actSave, &QAction::triggered, this, &MainWindow::handleSaveFile);

    QAction *actSaveAs = m_fileMenu->addAction("💾  Save As...");
    actSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(actSaveAs, &QAction::triggered, this, &MainWindow::handleSaveFileAs);

    m_fileMenu->addSeparator();

    if (m_recentFilesManager && m_recentFilesManager->menu()) {
        m_fileMenu->addMenu(m_recentFilesManager->menu());
    }

    m_fileMenu->addSeparator();

    QAction *actClear = m_fileMenu->addAction("❌  Close / Clear All");
    connect(actClear, &QAction::triggered, this, &MainWindow::clearAll);
}

void MainWindow::handleNewFile() {
    if (inputEditor && inputEditor->document()->isModified()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Unsaved Changes",
            "The Document has been modified.\nDo you want to save your Changes before creating a new File?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
            );

        if (reply == QMessageBox::Save) {
            if (!handleSaveFile()) return;
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    clearAll();
}

void MainWindow::handleOpenFile() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open German Text Document",
        QString(),
        "Text Files (*.txt *.md);;All Files (*)"
        );

    if (!filePath.isEmpty()) {
        loadFile(filePath);
    }
}

bool MainWindow::handleSaveFile() {
    if (m_currentFilePath.isEmpty()) {
        return handleSaveFileAs();
    }
    return writeFile(m_currentFilePath);
}

bool MainWindow::handleSaveFileAs() {
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save German Text Document",
        m_currentFilePath.isEmpty() ? "Unbenannt.txt" : m_currentFilePath,
        "Text Files (*.txt *.md);;All Files (*)"
        );

    if (filePath.isEmpty()) {
        return false;
    }

    m_currentFilePath = filePath;
    return writeFile(filePath);
}

void MainWindow::switchWordPane(int index) {
    if (!wordStackedWidget) return;

    if (m_isEditingWordMode) {
        m_isEditingWordMode = false;
        if (wordOutputEditor) wordOutputEditor->setReadOnly(true);
        if (lookupOutputEditor) lookupOutputEditor->setReadOnly(true);
        if (btnEditWordEditor) {
            btnEditWordEditor->setText("✏️");
            btnEditWordEditor->setToolTip("Edit Word Explanations and Save to Database");
        }
    }

    wordStackedWidget->setCurrentIndex(index);
    btnTabSentenceWords->setChecked(index == 0);
    btnTabGlossary->setChecked(index == 1);

    if (btnEditWordEditor) btnEditWordEditor->setVisible(true);

    bool isSentenceView = (index == 0);
    if (btnCopyTranslation) btnCopyTranslation->setVisible(isSentenceView);
    if (btnSplitWordEditor) btnSplitWordEditor->setVisible(isSentenceView);
}

void MainWindow::handlePureLookupFinished(const QList<QPair<QString, QString>> &results) {
    if (!lookupOutputEditor) return;

    if (results.isEmpty()) {
        lookupOutputEditor->setPlainText("No matching Vocabulary found in Database.");
        return;
    }

    QString text;
    text.reserve(results.size() * 64);

    for (const auto &pair : results) {
        text += QString("%1 : %2\n\n").arg(pair.first, pair.second);
    }

    lookupOutputEditor->setPlainText(text);
}

void MainWindow::handlePureWordUpdateFinished(bool success) {
    if (statusLabel) {
        statusLabel->setStyleSheet(success ? "color: #00FF00;" : "color: red;");
        statusLabel->setText(success ? "Glossary saved" : "Glossary save failed");
        QTimer::singleShot(2000, this, [this]() {
            if (statusLabel) {
                statusLabel->setStyleSheet("color: #00FF00;");
                statusLabel->setText("Ready");
            }
        });
    }
}
