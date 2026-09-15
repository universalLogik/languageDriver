#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QToolButton>
#include <QFrame>
#include <QVBoxLayout>
#include <QProcess>
#include <QFile>
#include <QSplitter>
#include <QSaveFile>
#include <QFileInfo>
#include <QStackedWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class TranslationEngine;
class DictionaryWorker;
class QPlainTextEdit;
class QPushButton;
class QThread;
class QLabel;
class AudioTranscriptionWorker;
class RecentFilesManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void initTranslation(const QString &modelPath);
    void initDictionary();
    void operateTranslation(const QStringList &germanChunks);
    void operateLookup(const QStringList &germanChunks);
    void operateTranscription(const QString &audioFilePath, const QString &languageCode);
    void operateWordUpdate(int chunkIndex, const QList<QPair<QString, QString>> &updatedList);
    void operatePureLookup(const QString &text);
    void operatePureWordUpdate(const QList<QPair<QString, QString>> &updatedList);

private slots:
    void processText();
    void handleLookupChunkFinished(int index,
                                   const QList<QPair<QString, QString>> &originalList,
                                   const QList<QPair<QString, QString>> &exerciseList);
    void handleBatchFinished();
    void clearAll();
    void showNextChunk();
    void showPreviousChunk();
    void handleChunkTranslationFinished(int index, const QString &translation);
    void onTranscriptionFinished(const QString &text);
    void processLookupOnly();
    void initLookupOnlyButton();
    void handlePureLookupFinished(const QList<QPair<QString, QString>> &results);
    void switchWordPane(int index);
    void handlePureWordUpdateFinished(bool success);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateChunkDisplay();
    QString getEditorPanelStyle(const QString &accentColor) const;
    void resetSecondaryWordEditor();
    void initCopyButton();
    void initSplitWordEditorButton(QHBoxLayout *wordHeaderLayout, QSplitter *wordEditorSplitter);

    void playCurrentAudio();
    void setupUiLayout();
    void setupSeamButtonMenu();
    void setupCollapseFeature();
    void setupSortFeature();
    void setupVoiceModeFeature();
    void setupAudioPlaybackFeature();
    void setupTranslationFeature();
    void setupWordExplanationsFeature();
    QHBoxLayout* setupActionControlFeature();
    QHBoxLayout* setupNavigationControlFeature();
    void setupShortcutsAndEvents();
    void initialPiperProcess();
    void setupBottomControlFeature();
    void initTranscribeButton();
    void setupSpeechRecognitionFeature();

    void setupAudioRecorder();
    void startRecordingAudio();
    void stopRecordingAudio();
    void initThemeToggleButton();
    void applyTheme(bool isDark);
    void applySystemTheme(bool isDark);
    void stripComments();

    void saveFile();
    void saveFileAs();
    void openFile();
    void loadFile(const QString &filePath);
    bool writeFile(const QString &filePath);
    void updateWindowTitle();
    void initEditWordButton();
    void handleWordUpdateFinished(bool success);

    void initHistoryMenu();
    void initFileMenuButton();
    void setupFileMenu();
    void handleNewFile();
    void handleOpenFile();
    bool handleSaveFile();
    bool handleSaveFileAs();

    // Audio Pipeline and Looping
    void toggleAudioLoop();
    void stopAudioLoop();
    void playNextLoopToken();
    bool writeToAudioPipe(const QString &pipePath, const QString &text);

    QString getSeamButtonStyle(bool isDark = true) const;

private:
    QStringList m_germanChunks;
    QString m_modelPath;

    QStringList m_sentenceChunks;
    QStringList m_englishTranslations;
    int m_currentChunkIndex;
    Ui::MainWindow *ui;

    QTextEdit *inputEditor{nullptr};
    QPlainTextEdit *sentenceOutputEditor{nullptr};
    QPlainTextEdit *wordOutputEditor{nullptr};
    QPushButton *processButton{nullptr};
    QPushButton *nextButton{nullptr};
    QPushButton *prevButton{nullptr};
    QLabel *chunkLabel{nullptr};
    QPushButton *clearButton{nullptr};
    QSplitter *rightOutputSplitter{nullptr};

    QFrame *seamContainer{nullptr};
    QToolButton *btnCollapseLeft{nullptr};
    QToolButton *btnCollapseRight{nullptr};
    QVBoxLayout *rightOutputLayout{nullptr};
    QPlainTextEdit *wordOutputEditorSecondary{nullptr};
    QToolButton *btnSplitWordEditor{nullptr};

    QToolButton *btnToggleSort{nullptr};
    bool m_showOriginalOrder;
    QToolButton *btnToggleVoiceMode{nullptr};
    bool m_directInputVoiceMode;
    bool m_translationEngineInitialized;

    QList<QList<QPair<QString, QString>>> m_chunkedOriginalExplanations;
    QList<QList<QPair<QString, QString>>> m_chunkedExerciseExplanations;
    QToolButton *btnPlayAudio{nullptr};
    QProcess *m_piperProcess{nullptr};

    QToolButton *btnWhisperGerman{nullptr};
    QToolButton *btnWhisperEnglish{nullptr};
    QToolButton *btnWhisperSpanish{nullptr};

    QTimer *m_listeningAnimationTimer{nullptr};
    int m_listeningDotCount;
    QString m_recordedAudioPath;
    QString m_activeTranscriptionLanguage;
    QToolButton *btnCopyTranslation{nullptr};
    QToolButton *btnToggleTheme{nullptr};
    bool m_isDarkMode{true};
    QProcess *m_recordProcess{nullptr};
    QToolButton *btnLookupOnly{nullptr};

    TranslationEngine *translationEngine{nullptr};
    QThread *translationThread{nullptr};

    QThread *dictionaryThread{nullptr};
    DictionaryWorker *dictionaryWorker{nullptr};

    QThread *m_audioThread{nullptr};
    AudioTranscriptionWorker *m_audioWorker{nullptr};
    QString m_whisperModelPath;
    QString m_cleanedText;
    QLabel *statusLabel{nullptr};
    QString m_currentFilePath;
    QToolButton *btnEditWordEditor{nullptr};
    bool m_isEditingWordMode;
    QLabel *lblWordHeader{nullptr};

    QToolButton *btnFileMenu{nullptr};
    QMenu *m_fileMenu{nullptr};
    RecentFilesManager *m_recentFilesManager{nullptr};

    QStackedWidget *wordStackedWidget{nullptr};
    QPlainTextEdit *lookupOutputEditor{nullptr};
    QToolButton *btnTabSentenceWords{nullptr};
    QToolButton *btnTabGlossary{nullptr};

    // Dedicated State Management for Audio Looping
    QTimer *m_loopAudioTimer{nullptr};
    QStringList m_loopTokens;
    int m_currentLoopIndex{0};
    bool m_isLooping{false};
};

#endif // MAINWINDOW_H
