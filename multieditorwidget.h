#ifndef MULTIEDITORWIDGET_H
#define MULTIEDITORWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <QTextEdit>
#include <QToolButton>
#include <QLabel>
#include <QFrame>
#include <QList>

#include <QTextEdit>
#include <QMimeData>


class FormulaTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit FormulaTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {}

protected:
    void insertFromMimeData(const QMimeData *source) override;
};



class EditorPane : public QWidget
{
    Q_OBJECT

public:
    explicit EditorPane(QWidget *parent = nullptr);
    ~EditorPane() override = default;

    //QTextEdit* editor() const { return m_editor; }
    FormulaTextEdit* editor() const { return m_editor; }
    void setActive(bool active);
    void setTitle(const QString &title);
    QString title() const;
    void setCloseEnabled(bool enabled);
    void setCloseArrow(const QString &arrow);

signals:
    void splitRequested(EditorPane *pane);
    void closeRequested(EditorPane *pane);
    void focused(EditorPane *pane);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLabel *m_titleLabel{nullptr};
    QToolButton *m_btnSplit{nullptr};
    QToolButton *m_btnClose{nullptr};
    QFrame *m_focusLine{nullptr};
    //QTextEdit *m_editor{nullptr};
    FormulaTextEdit *m_editor{nullptr};

    QFrame *m_toolbarWidget{nullptr};





};

class MultiEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MultiEditorWidget(QWidget *parent = nullptr);
    ~MultiEditorWidget() override = default;

    QTextEdit* activeEditor() const;
    EditorPane* activePane() const;
    QString activeText() const;
    void setActiveText(const QString &text);
    void setActiveTitle(const QString &title);
    QList<QTextEdit*> allEditors() const;

signals:
    void activeEditorChanged(QTextEdit *editor);
    void textModified(bool modified);

public slots:
    void splitCurrentPane();
    void closePane(EditorPane *pane);

private slots:
    void onPaneFocused(EditorPane *pane);

private:
    EditorPane* createPaneInstance();
    void updatePanesState();

    QSplitter *m_splitter{nullptr};
    QList<EditorPane*> m_panes;
    EditorPane *m_currentActivePane{nullptr};



};

#endif // MULTIEDITORWIDGET_H
