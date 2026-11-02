#include "multieditorwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QStyle>
#include <QRegularExpression>



EditorPane::EditorPane(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Miniature Toolbar with Object Name for themeQss
    m_toolbarWidget = new QFrame(this);
    m_toolbarWidget->setObjectName("paneToolbar");
    m_toolbarWidget->setFixedHeight(24);
    m_toolbarWidget->setFrameShape(QFrame::NoFrame);

    QHBoxLayout *toolbarLayout = new QHBoxLayout(m_toolbarWidget);
    toolbarLayout->setContentsMargins(8, 0, 4, 0);
    toolbarLayout->setSpacing(4);

    m_titleLabel = new QLabel("Untitled", m_toolbarWidget);
    m_titleLabel->setObjectName("paneTitleLabel");

    m_btnSplit = new QToolButton(m_toolbarWidget);
    m_btnSplit->setObjectName("btnPaneAction");
    m_btnSplit->setText("+");
    m_btnSplit->setToolTip("Split View Horizontally (Qt Creator Style)");
    m_btnSplit->setCursor(Qt::PointingHandCursor);

    m_btnClose = new QToolButton(m_toolbarWidget);
    m_btnClose->setObjectName("btnPaneAction");
    m_btnClose->setText("<");
    m_btnClose->setToolTip("Close this View");
    m_btnClose->setCursor(Qt::PointingHandCursor);

    toolbarLayout->addWidget(m_titleLabel);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_btnSplit);
    toolbarLayout->addWidget(m_btnClose);

    // Active Focus Line
    m_focusLine = new QFrame(this);
    m_focusLine->setObjectName("paneFocusLine");
    m_focusLine->setFixedHeight(2);
    m_focusLine->setFrameShape(QFrame::NoFrame);

    // Document Editor Viewport
    //m_editor = new QTextEdit(this);
    m_editor = new FormulaTextEdit(this);
    m_editor->setObjectName("inputEditor");
    m_editor->setPlaceholderText("Paste or type your Sentences here...");
    m_editor->setLineWrapMode(QTextEdit::FixedPixelWidth);
    m_editor->setLineWrapColumnOrWidth(800);
    m_editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editor->installEventFilter(this);

    mainLayout->addWidget(m_toolbarWidget);
    mainLayout->addWidget(m_focusLine);
    mainLayout->addWidget(m_editor);

    connect(m_btnSplit, &QToolButton::clicked, this, [this]() {
        emit splitRequested(this);
    });

    connect(m_btnClose, &QToolButton::clicked, this, [this]() {
        emit closeRequested(this);
    });

    setActive(false);
}

void EditorPane::setActive(bool active) {
    m_toolbarWidget->setProperty("active", active);
    m_toolbarWidget->style()->unpolish(m_toolbarWidget);
    m_toolbarWidget->style()->polish(m_toolbarWidget);

    m_focusLine->setStyleSheet(active ? "background-color: #00e676;" : "background-color: transparent;");

    if (m_titleLabel) {
        m_titleLabel->setProperty("active", active);
        m_titleLabel->style()->unpolish(m_titleLabel);
        m_titleLabel->style()->polish(m_titleLabel);
    }
}



void EditorPane::setTitle(const QString &title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

QString EditorPane::title() const {
    return m_titleLabel ? m_titleLabel->text() : QString();
}

void EditorPane::setCloseEnabled(bool enabled) {
    if (m_btnClose) {
        m_btnClose->setEnabled(enabled);
        m_btnClose->setVisible(enabled);
    }
}

void EditorPane::setCloseArrow(const QString &arrow) {
    if (m_btnClose) {
        m_btnClose->setText(arrow);
        if (arrow == "<") {
            m_btnClose->setToolTip("Close View (Collapse Left)");
        } else {
            m_btnClose->setToolTip("Close View (Collapse Right)");
        }
    }
}

bool EditorPane::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_editor && event->type() == QEvent::FocusIn) {
        emit focused(this);
    }
    return QWidget::eventFilter(watched, event);
}

MultiEditorWidget::MultiEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setObjectName("leftInputSplitter");
    mainLayout->addWidget(m_splitter);

    // Construct primary initial Pane
    EditorPane *initialPane = createPaneInstance();
    m_panes.append(initialPane);
    m_currentActivePane = initialPane;
    m_splitter->addWidget(initialPane);

    initialPane->setActive(true);
    updatePanesState();
}

QTextEdit* MultiEditorWidget::activeEditor() const {
    return (m_currentActivePane && m_panes.contains(m_currentActivePane))
           ? m_currentActivePane->editor()
           : nullptr;
}

EditorPane* MultiEditorWidget::activePane() const {
    return (m_currentActivePane && m_panes.contains(m_currentActivePane))
           ? m_currentActivePane
           : nullptr;
}

QString MultiEditorWidget::activeText() const {
    QTextEdit *editor = activeEditor();
    return editor ? editor->toPlainText() : QString();
}

void MultiEditorWidget::setActiveText(const QString &text) {
    QTextEdit *editor = activeEditor();
    if (editor) {
        editor->setPlainText(text);
        editor->document()->setModified(false);
    }
}

void MultiEditorWidget::setActiveTitle(const QString &title) {
    if (m_currentActivePane && m_panes.contains(m_currentActivePane)) {
        m_currentActivePane->setTitle(title);
    }
}

QList<QTextEdit*> MultiEditorWidget::allEditors() const {
    QList<QTextEdit*> list;
    for (EditorPane *pane : m_panes) {
        if (pane && m_panes.contains(pane) && pane->editor()) {
            list.append(pane->editor());
        }
    }
    return list;
}

EditorPane* MultiEditorWidget::createPaneInstance() {
    EditorPane *pane = new EditorPane(m_splitter);

    connect(pane, &EditorPane::focused, this, &MultiEditorWidget::onPaneFocused);
    connect(pane, &EditorPane::splitRequested, this, &MultiEditorWidget::splitCurrentPane);
    connect(pane, &EditorPane::closeRequested, this, &MultiEditorWidget::closePane);

    connect(pane->editor()->document(), &QTextDocument::modificationChanged,
            this, &MultiEditorWidget::textModified);

    return pane;
}

void MultiEditorWidget::onPaneFocused(EditorPane *pane) {
    if (!pane || !m_panes.contains(pane) || pane == m_currentActivePane) {
        return;
    }

    m_currentActivePane = pane;
    for (EditorPane *p : m_panes) {
        p->setActive(p == m_currentActivePane);
    }

    emit activeEditorChanged(m_currentActivePane->editor());
}

void MultiEditorWidget::splitCurrentPane() {
    EditorPane *newPane = createPaneInstance();
    m_panes.append(newPane);
    m_splitter->addWidget(newPane);

    onPaneFocused(newPane);
    newPane->editor()->setFocus();

    // Distribute Width equally across all Panes
    int totalWidth = m_splitter->width();
    if (totalWidth > 0 && !m_panes.isEmpty()) {
        int singleWidth = totalWidth / m_panes.size();
        QList<int> sizes;
        for (int i = 0; i < m_panes.size(); ++i) {
            sizes.append(singleWidth);
        }
        m_splitter->setSizes(sizes);
    }

    updatePanesState();
}

void MultiEditorWidget::closePane(EditorPane *pane) {
    if (m_panes.size() <= 1 || !pane || !m_panes.contains(pane)) {
        return;
    }

    // Disconnect pane completely so deferred events cannot trigger methods on a dying pane
    disconnect(pane, nullptr, this, nullptr);
    if (pane->editor()) {
        pane->editor()->removeEventFilter(pane);
    }

    int index = m_panes.indexOf(pane);
    bool wasActive = (pane == m_currentActivePane);
    m_panes.removeOne(pane);
    pane->deleteLater();

    if (wasActive) {
        m_currentActivePane = nullptr;
        int nextIndex = qBound(0, index, m_panes.size() - 1);
        if (!m_panes.isEmpty()) {
            EditorPane *nextPane = m_panes.at(nextIndex);
            m_currentActivePane = nextPane;
            for (EditorPane *p : m_panes) {
                p->setActive(p == m_currentActivePane);
            }
            emit activeEditorChanged(m_currentActivePane->editor());
            if (m_currentActivePane->editor()) {
                m_currentActivePane->editor()->setFocus();
            }
        } else {
            emit activeEditorChanged(nullptr);
        }
    }

    updatePanesState();
}

void MultiEditorWidget::updatePanesState() {
    bool canClose = m_panes.size() > 1;
    for (int i = 0; i < m_panes.size(); ++i) {
        EditorPane *pane = m_panes.at(i);
        pane->setCloseEnabled(canClose);
        if (i == 0) {
            pane->setCloseArrow("<");
        } else {
            pane->setCloseArrow(">");
        }
    }
}


static QString sanitizeFormulaMarkup(QString text)
{
    // Replace \xrightarrow{...} with Unicode arrow and bracketed label
    static const QRegularExpression arrowRegex(R"(\\xrightarrow\{([^}]*)\})");
    text.replace(arrowRegex, " → [\\1] ");

    // Replace basic arrows
    static const QRegularExpression simpleArrowRegex(R"(\\rightarrow|\\to)");
    text.replace(simpleArrowRegex, " → ");

    // Replace \mathbf{...} with bold HTML tags
    static const QRegularExpression boldRegex(R"(\\mathbf\{([^}]*)\})");
    text.replace(boldRegex, "<b>\\1</b>");

    // Replace \text{...} with inner plain text
    static const QRegularExpression textRegex(R"(\\text\{([^}]*)\})");
    text.replace(textRegex, "\\1");

    // Clean up escaped spaces from LaTeX
    text.replace(QRegularExpression(R"(\\\s+)"), " ");

    // Remove LaTeX math delimiters ($$ and $)
    text.remove("$$");
    text.remove("$");

    return text;
}


void FormulaTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (!source) return;

    // 1. If the clipboard has rich HTML (e.g. from Browser or Chat)
    if (source->hasHtml()) {
        QString html = source->html();

        // Clean any residual LaTeX math symbols if present
        if (html.contains("$$") || html.contains("\\mathbf") || html.contains("\\xrightarrow")) {
            html = sanitizeFormulaMarkup(html);
        }

        // Ensure table borders are rendered crisply in dark mode
        if (html.contains("<table") && !html.contains("border=")) {
            html.replace("<table", "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\" style=\"border-collapse: collapse; border: 1px solid #444444;\"");
        }

        // Force Qt to render the rich HTML table layout
        this->insertHtml(html);
        return;
    }

    // 2. If the clipboard only has plain text
    if (source->hasText()) {
        QString text = source->text();

        // Sanitize LaTeX if present
        if (text.contains("$$") || text.contains("\\mathbf") || text.contains("\\xrightarrow")) {
            text = sanitizeFormulaMarkup(text);
        }

        // If it contains raw HTML tags, insert as HTML
        if (text.contains("<table") || text.contains("<tr>") || text.contains("<td>")) {
            this->insertHtml(text);
            return;
        }

        // If it's a Markdown table (|---|---|), convert to HTML or insert directly
        if (text.contains("|") && text.contains("---")) {
            this->insertPlainText(text); // or let setMarkdown handle it
            return;
        }

        this->insertPlainText(text);
        return;
    }

    QTextEdit::insertFromMimeData(source);
}



