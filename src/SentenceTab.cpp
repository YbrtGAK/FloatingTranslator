#include "SentenceTab.h"

#include "DeepLClient.h"
#include "Style.h"

#include <QClipboard>
#include <QComboBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

struct Language
{
    const char* name;
    const char* code;
};

constexpr Language kSourceLanguages[] = {
    {"Chinese", "ZH"},   {"Dutch", "NL"},      {"English", "EN"}, {"French", "FR"},  {"German", "DE"},
    {"Italian", "IT"},   {"Japanese", "JA"},   {"Korean", "KO"},  {"Polish", "PL"},  {"Portuguese", "PT"},
    {"Russian", "RU"},   {"Spanish", "ES"},    {"Swedish", "SV"}, {"Turkish", "TR"}, {"Ukrainian", "UK"},
};

// Some DeepL target codes name a variant; their first two letters are always the source code.
constexpr Language kTargetLanguages[] = {
    {"Chinese (simplified)", "ZH"},
    {"Dutch", "NL"},
    {"English (UK)", "EN-GB"},
    {"English (US)", "EN-US"},
    {"French", "FR"},
    {"German", "DE"},
    {"Italian", "IT"},
    {"Japanese", "JA"},
    {"Korean", "KO"},
    {"Polish", "PL"},
    {"Portuguese (Brazil)", "PT-BR"},
    {"Portuguese (Portugal)", "PT-PT"},
    {"Russian", "RU"},
    {"Spanish", "ES"},
    {"Swedish", "SV"},
    {"Turkish", "TR"},
    {"Ukrainian", "UK"},
};

QString languageName(const QString& code)
{
    for (const Language& language : kSourceLanguages) {
        if (code == QLatin1String(language.code))
            return QString::fromUtf8(language.name);
    }
    return code;
}

} // namespace

SentenceTab::SentenceTab(DeepLClient* client, QWidget* parent)
    : QWidget(parent)
    , m_client(client)
    , m_source(new QComboBox(this))
    , m_target(new QComboBox(this))
    , m_input(new QPlainTextEdit(this))
    , m_output(new QPlainTextEdit(this))
    , m_translateButton(new QPushButton(tr("Translate"), this))
{
    m_source->addItem(tr("Detect language"), QString());
    for (const Language& language : kSourceLanguages)
        m_source->addItem(QString::fromUtf8(language.name), QString::fromLatin1(language.code));
    for (const Language& language : kTargetLanguages)
        m_target->addItem(QString::fromUtf8(language.name), QString::fromLatin1(language.code));

    auto* swapButton = makeGlyphButton(Glyph::Swap, tr("Swap languages"), this);

    m_input->setPlaceholderText(tr("Type or paste text to translate"));
    m_input->setTabChangesFocus(true);
    m_output->setReadOnly(true);
    m_output->setPlaceholderText(tr("Translation"));
    m_output->setTabChangesFocus(true);
    m_translateButton->setToolTip(tr("Ctrl+Enter"));
    auto* copyButton = new QPushButton(tr("Copy"), this);

    auto* languageRow = new QHBoxLayout;
    languageRow->addWidget(m_source, 1);
    languageRow->addWidget(swapButton);
    languageRow->addWidget(m_target, 1);

    auto* translateRow = new QHBoxLayout;
    translateRow->addStretch();
    translateRow->addWidget(m_translateButton);

    auto* copyRow = new QHBoxLayout;
    copyRow->addStretch();
    copyRow->addWidget(copyButton);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(languageRow);
    layout->addWidget(m_input, 1);
    layout->addLayout(translateRow);
    layout->addWidget(m_output, 1);
    layout->addLayout(copyRow);

    const QSettings settings;
    m_source->setCurrentIndex(qMax(0, m_source->findData(settings.value(QStringLiteral("deepl/source")).toString())));
    m_target->setCurrentIndex(
        qMax(0, m_target->findData(settings.value(QStringLiteral("deepl/target"), QStringLiteral("EN-GB")).toString())));

    for (const QKeySequence& keys : {QKeySequence(Qt::CTRL | Qt::Key_Return), QKeySequence(Qt::CTRL | Qt::Key_Enter)}) {
        auto* shortcut = new QShortcut(keys, this);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(shortcut, &QShortcut::activated, this, &SentenceTab::translate);
    }
    connect(m_translateButton, &QPushButton::clicked, this, &SentenceTab::translate);
    connect(swapButton, &QToolButton::clicked, this, &SentenceTab::swapLanguages);
    connect(copyButton, &QPushButton::clicked, this, [this] {
        if (m_output->toPlainText().isEmpty())
            return;
        QGuiApplication::clipboard()->setText(m_output->toPlainText());
        emit statusChanged(tr("Translation copied."), false);
    });

    connect(m_client, &DeepLClient::translated, this, [this](const QString& text, const QString& detected) {
        m_translateButton->setEnabled(true);
        m_output->setPlainText(text);
        m_detectedSource = detected;
        const bool autoDetect = m_source->currentIndex() == 0;
        emit statusChanged(autoDetect && !detected.isEmpty() ? tr("Detected: %1").arg(languageName(detected))
                                                             : QString(),
                           false);
    });
    connect(m_client, &DeepLClient::failed, this, [this](const QString& message) {
        m_translateButton->setEnabled(true);
        emit statusChanged(message, true);
    });
}

void SentenceTab::focusInput()
{
    m_input->setFocus();
    m_input->selectAll();
}

void SentenceTab::saveSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("deepl/source"), m_source->currentData());
    settings.setValue(QStringLiteral("deepl/target"), m_target->currentData());
}

void SentenceTab::translate()
{
    const QString text = m_input->toPlainText();
    if (text.trimmed().isEmpty())
        return;
    if (!m_client->hasApiKey()) {
        emit settingsRequested(); // modal: returns once the dialog is closed
        if (!m_client->hasApiKey()) {
            emit statusChanged(tr("A DeepL API key is needed to translate."), true);
            return;
        }
    }

    m_translateButton->setEnabled(false);
    emit statusChanged(tr("Translating…"), false);
    m_client->translate(text, m_source->currentData().toString(), m_target->currentData().toString());
}

void SentenceTab::swapLanguages()
{
    const QString source = m_source->currentIndex() > 0 ? m_source->currentData().toString() : m_detectedSource;
    const int newTarget = targetIndexFor(source);
    if (newTarget < 0) {
        emit statusChanged(tr("Choose the source language before swapping."), true);
        return;
    }

    m_previousTarget = m_target->currentData().toString();
    m_source->setCurrentIndex(m_source->findData(m_previousTarget.left(2)));
    m_target->setCurrentIndex(newTarget);
    m_detectedSource.clear();

    // The current translation is a valid pair in the other direction too.
    const QString translation = m_output->toPlainText();
    if (!translation.isEmpty()) {
        m_output->setPlainText(m_input->toPlainText());
        m_input->setPlainText(translation);
    }
}

int SentenceTab::targetIndexFor(const QString& sourceCode) const
{
    if (sourceCode.isEmpty())
        return -1;
    // Prefer the variant used last time, e.g. keep EN-US rather than switching to EN-GB.
    if (m_previousTarget.left(2) == sourceCode)
        return m_target->findData(m_previousTarget);
    for (int i = 0; i < m_target->count(); ++i) {
        if (m_target->itemData(i).toString().left(2) == sourceCode)
            return i;
    }
    return -1;
}
