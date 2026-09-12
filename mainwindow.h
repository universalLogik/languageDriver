#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QStringList>
//#include <memory>
#include <QToolButton>
#include <QFrame>
#include <QVBoxLayout>
#include <QProcess>
#include <QFile>
#include <QSplitter>
#include <QSaveFile>
#include <QFileInfo>
#include <QStackedWidget>


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
    //void initWhisper(const QString &modelPath);
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

    //void handleTranscriptionFinished(const QString &text);
    //void handleTranscriptionError(const QString &errorMessage);

    void onTranscriptionFinished(const QString &text);

    // Inside private slots:
    void processLookupOnly();

    // Inside private methods:
    void initLookupOnlyButton();
    void handlePureLookupFinished(const QList<QPair<QString, QString>> &results);
    void switchWordPane(int index);
    void handlePureWordUpdateFinished(bool success);



















protected:
    bool eventFilter(QObject *watched, QEvent *event) override;



private:
    void updateChunkDisplay();
    QString getEditorPanelStyle(const QString &accentColor) const;
   // QString getActionButtonStyle(bool isDark = true) const;
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
    // void initRecentFilesButton();


     void initFileMenuButton();
     void setupFileMenu();
     void handleNewFile();
     void handleOpenFile();
     bool handleSaveFile();
     bool handleSaveFileAs();

     bool ensurePipeOpen(const QString &lang);




















  QString getSeamButtonStyle(bool isDark = true) const;










 private:
    // Symmetrical Data Containers for progressive Slot Overwriting
    QStringList m_germanChunks;
    QString m_modelPath;
    //const QString m_seamButtonStyle;

    QStringList m_sentenceChunks;
    QStringList m_englishTranslations; // Added to cache the incoming stream
    // QList<QList<QPair<QString, QString>>> m_chunkedExplanations;
    int m_currentChunkIndex;
    Ui::MainWindow *ui;

    // Graphical Components for the User Interface Layout
    QTextEdit *inputEditor{nullptr}; // Added missing Component
    QPlainTextEdit *sentenceOutputEditor;
    QPlainTextEdit *wordOutputEditor;
    QPushButton *processButton; // Added missing Component
    QPushButton *nextButton;
    QPushButton *prevButton;
    QLabel *chunkLabel;
    QPushButton *clearButton;
    QSplitter *rightOutputSplitter;

    QFrame *seamContainer;
    QToolButton *btnCollapseLeft;
    QToolButton *btnCollapseRight;
 //   QWidget *rightOutputWidget;
    QVBoxLayout *rightOutputLayout;
    QPlainTextEdit *wordOutputEditorSecondary;
    QToolButton *btnSplitWordEditor;


    QToolButton *btnToggleSort;
    bool m_showOriginalOrder;
    QToolButton *btnToggleVoiceMode;
    bool m_directInputVoiceMode;
    bool m_translationEngineInitialized;


    QList<QList<QPair<QString, QString>>> m_chunkedOriginalExplanations;
    QList<QList<QPair<QString, QString>>> m_chunkedExerciseExplanations;
    QToolButton *btnPlayAudio;
    QProcess *m_piperProcess;
    QFile m_pipeFile;

    QToolButton *btnWhisperGerman{nullptr};
    QToolButton *btnWhisperEnglish{nullptr};
    QToolButton *btnWhisperSpanish{nullptr};


    QTimer *m_listeningAnimationTimer{nullptr};
    int m_listeningDotCount;
    QString m_recordedAudioPath;
    QString m_activeTranscriptionLanguage;
    QToolButton *btnCopyTranslation;
    QToolButton *btnToggleTheme{nullptr};
    bool m_isDarkMode{true};
    QProcess *m_recordProcess;
    QToolButton *btnLookupOnly;
    // Threading and Processing Infrastructure
    TranslationEngine*  translationEngine;
    QThread *translationThread;

    QThread *dictionaryThread;
    DictionaryWorker *dictionaryWorker;

    QThread *m_audioThread;
    AudioTranscriptionWorker *m_audioWorker;
    QString m_whisperModelPath;
    QString m_cleanedText;
    QLabel *statusLabel;
    QString m_currentFilePath;
    QToolButton *btnEditWordEditor;
    bool m_isEditingWordMode;
    QLabel *lblWordHeader;

    //QToolButton *btnRecentFiles;
    QToolButton *btnFileMenu;
    QMenu *m_fileMenu;
    RecentFilesManager *m_recentFilesManager;

    QStackedWidget *wordStackedWidget;
    QPlainTextEdit *lookupOutputEditor;
    QToolButton *btnTabSentenceWords;
    QToolButton *btnTabGlossary;

    QFile m_pipeFileDe;
    QFile m_pipeFileEn;
    QFile m_pipeFileEs;






















};

#endif // MAINWINDOW_H
