#pragma once

#include <QWidget>

class DeepLClient;
class PonsClient;
class QLabel;
class QSlider;
class QTabWidget;
class QToolButton;
class SentenceTab;
class WordTab;

// Frameless panel with its own title bar (pin, opacity, settings) and two tabs:
// sentence translation with DeepL and word lookup in the PONS dictionary.
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QWidget* createTitleBar();
    void loadSettings();
    void setPinned(bool pinned);
    void setOpacityPercent(int percent);
    void openSettings();
    void applyApiKeys();
    void showStatus(QWidget* tab, const QString& message, bool isError);
    void focusCurrentTab();

    DeepLClient* m_deepl;
    PonsClient* m_pons;
    QTabWidget* m_tabs;
    SentenceTab* m_sentenceTab;
    WordTab* m_wordTab;
    QLabel* m_status;
    QWidget* m_titleBar = nullptr;
    QToolButton* m_pinButton = nullptr;
    QToolButton* m_opacityButton = nullptr;
    QSlider* m_opacitySlider = nullptr;
};
