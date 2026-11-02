#include "MainWindow.h"

#include "DeepLClient.h"
#include "PonsClient.h"
#include "SentenceTab.h"
#include "SettingsDialog.h"
#include "Style.h"
#include "WordTab.h"

#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QSizeGrip>
#include <QSlider>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QWindow>

// WindowMinimizeButtonHint lets the frameless window be minimized and restored from the taskbar.
MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint)
    , m_deepl(new DeepLClient(this))
    , m_pons(new PonsClient(this))
    , m_tabs(new QTabWidget(this))
    , m_sentenceTab(new SentenceTab(m_deepl))
    , m_wordTab(new WordTab(m_pons))
    , m_status(new QLabel(this))
{
    setWindowTitle(tr("Translator"));
    setMinimumSize(320, 380);
    resize(400, 520);

    m_tabs->setDocumentMode(true);
    m_tabs->addTab(m_sentenceTab, tr("Sentence"));
    m_tabs->addTab(m_wordTab, tr("Word"));
    m_tabs->setTabToolTip(0, tr("Translate text with DeepL"));
    m_tabs->setTabToolTip(1, tr("Look up a word in the PONS dictionary"));

    // Long messages are clipped (full text in the tooltip) instead of widening the window.
    m_status->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    auto* statusRow = new QHBoxLayout;
    statusRow->setContentsMargins(10, 0, 0, 0);
    statusRow->addWidget(m_status, 1);
    statusRow->addWidget(new QSizeGrip(this), 0, Qt::AlignBottom | Qt::AlignRight);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1); // keeps the border drawn in paintEvent() visible
    layout->setSpacing(0);
    layout->addWidget(createTitleBar());
    layout->addWidget(m_tabs, 1);
    layout->addLayout(statusRow);

    connect(m_sentenceTab, &SentenceTab::statusChanged, this, [this](const QString& message, bool isError) {
        showStatus(m_sentenceTab, message, isError);
    });
    connect(m_wordTab, &WordTab::statusChanged, this, [this](const QString& message, bool isError) {
        showStatus(m_wordTab, message, isError);
    });
    connect(m_sentenceTab, &SentenceTab::settingsRequested, this, &MainWindow::openSettings);
    connect(m_wordTab, &WordTab::settingsRequested, this, &MainWindow::openSettings);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this] {
        m_status->clear();
        focusCurrentTab();
    });

    loadSettings();
    focusCurrentTab();

    // First launch: ask for the API keys right away.
    if (!m_deepl->hasApiKey() && !m_pons->hasApiKey())
        QTimer::singleShot(0, this, &MainWindow::openSettings);
}

QWidget* MainWindow::createTitleBar()
{
    m_titleBar = new QWidget(this);
    m_titleBar->installEventFilter(this); // dragging the bar moves the window

    auto* logo = new QLabel(m_titleBar);
    logo->setPixmap(QApplication::windowIcon().pixmap(QSize(16, 16), devicePixelRatioF()));

    auto* title = new QLabel(windowTitle(), m_titleBar);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);

    m_pinButton = makeGlyphButton(Glyph::Pin, tr("Keep on top of other windows"), m_titleBar);
    m_pinButton->setCheckable(true);
    connect(m_pinButton, &QToolButton::toggled, this, &MainWindow::setPinned);

    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(30, 100);
    m_opacitySlider->setValue(100);
    m_opacitySlider->setFixedWidth(160);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &MainWindow::setOpacityPercent);

    auto* opacityMenu = new QMenu(this);
    auto* sliderAction = new QWidgetAction(opacityMenu);
    sliderAction->setDefaultWidget(m_opacitySlider);
    opacityMenu->addAction(sliderAction);

    m_opacityButton = new QToolButton(m_titleBar);
    m_opacityButton->setAutoRaise(true);
    m_opacityButton->setText(QStringLiteral("100%"));
    m_opacityButton->setToolTip(tr("Window opacity"));
    m_opacityButton->setMenu(opacityMenu);
    m_opacityButton->setPopupMode(QToolButton::InstantPopup);

    auto* settingsButton = makeGlyphButton(Glyph::Settings, tr("API keys"), m_titleBar);
    connect(settingsButton, &QToolButton::clicked, this, &MainWindow::openSettings);

    auto* minimizeButton = makeGlyphButton(Glyph::Minimize, tr("Minimize"), m_titleBar);
    connect(minimizeButton, &QToolButton::clicked, this, &QWidget::showMinimized);

    auto* closeButton = makeGlyphButton(Glyph::Close, tr("Close"), m_titleBar);
    connect(closeButton, &QToolButton::clicked, this, &QWidget::close);

    auto* layout = new QHBoxLayout(m_titleBar);
    layout->setContentsMargins(10, 4, 4, 4);
    layout->setSpacing(2);
    layout->addWidget(logo);
    layout->addSpacing(6);
    layout->addWidget(title);
    layout->addStretch();
    layout->addWidget(m_pinButton);
    layout->addWidget(m_opacityButton);
    layout->addWidget(settingsButton);
    layout->addSpacing(6);
    layout->addWidget(minimizeButton);
    layout->addWidget(closeButton);
    return m_titleBar;
}

void MainWindow::loadSettings()
{
    const QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
    m_tabs->setCurrentIndex(settings.value(QStringLiteral("window/tab"), 0).toInt());
    m_opacitySlider->setValue(settings.value(QStringLiteral("window/opacity"), 100).toInt());
    m_pinButton->setChecked(settings.value(QStringLiteral("window/pinned"), true).toBool());
    applyApiKeys();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/tab"), m_tabs->currentIndex());
    settings.setValue(QStringLiteral("window/opacity"), m_opacitySlider->value());
    settings.setValue(QStringLiteral("window/pinned"), m_pinButton->isChecked());
    m_sentenceTab->saveSettings();
    m_wordTab->saveSettings();
    QWidget::closeEvent(event);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_titleBar && event->type() == QEvent::MouseButtonPress
        && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
        windowHandle()->startSystemMove();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setPen(palette().color(QPalette::Mid));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void MainWindow::setPinned(bool pinned)
{
    m_pinButton->setText(QString(QChar(pinned ? Glyph::Pinned : Glyph::Pin)));
    m_pinButton->setToolTip(pinned ? tr("Always on top (click to release)") : tr("Keep on top of other windows"));

    // Changing window flags hides the window, so show it again if it was visible.
    const bool wasVisible = isVisible();
    setWindowFlag(Qt::WindowStaysOnTopHint, pinned);
    if (wasVisible)
        show();
}

void MainWindow::setOpacityPercent(int percent)
{
    setWindowOpacity(percent / 100.0);
    m_opacityButton->setText(QStringLiteral("%1%").arg(percent));
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        applyApiKeys();
        m_status->clear();
    }
}

void MainWindow::applyApiKeys()
{
    m_deepl->setApiKey(SettingsDialog::deeplApiKey());
    m_pons->setApiKey(SettingsDialog::ponsApiKey());
}

void MainWindow::showStatus(QWidget* tab, const QString& message, bool isError)
{
    if (tab != m_tabs->currentWidget())
        return;
    const QString color = isError ? QString::fromLatin1(kErrorColor) : mutedColor(palette());
    m_status->setStyleSheet(QStringLiteral("color: %1;").arg(color));
    m_status->setText(message);
    m_status->setToolTip(message);
}

void MainWindow::focusCurrentTab()
{
    if (m_tabs->currentWidget() == m_sentenceTab)
        m_sentenceTab->focusInput();
    else
        m_wordTab->focusInput();
}
