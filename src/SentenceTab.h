#pragma once

#include <QWidget>

class DeepLClient;
class QComboBox;
class QPlainTextEdit;
class QPushButton;

// "Sentence" tab: translates free text with DeepL.
class SentenceTab : public QWidget
{
    Q_OBJECT

public:
    explicit SentenceTab(DeepLClient* client, QWidget* parent = nullptr);

    void focusInput();
    void saveSettings() const;

signals:
    void statusChanged(const QString& message, bool isError);
    void settingsRequested();

private:
    void translate();
    void swapLanguages();
    int targetIndexFor(const QString& sourceCode) const;

    DeepLClient* m_client;
    QComboBox* m_source;
    QComboBox* m_target;
    QPlainTextEdit* m_input;
    QPlainTextEdit* m_output;
    QPushButton* m_translateButton;
    QString m_detectedSource;
    QString m_previousTarget;
};
