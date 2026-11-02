#include "WordTab.h"

#include "Style.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QTextBrowser>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>

// PONS returns entries as roms (one per part of speech), each holding arabs (one per meaning), each
// holding source/target translation pairs. The text fields already contain HTML with PONS CSS classes.

namespace {

QString directionId(const PonsDirection& direction)
{
    return direction.dictionary + QLatin1Char('/') + direction.sourceLang;
}

QString renderTranslations(const QJsonArray& translations)
{
    QString html = QStringLiteral("<table width=\"100%\" cellspacing=\"0\" cellpadding=\"2\">");
    for (const QJsonValue& value : translations) {
        const QJsonObject translation = value.toObject();
        html += QStringLiteral("<tr><td width=\"50%\" class=\"src\">%1</td><td width=\"50%\">%2</td></tr>")
                    .arg(translation.value(QLatin1String("source")).toString(),
                         translation.value(QLatin1String("target")).toString());
    }
    return html + QStringLiteral("</table>");
}

QString renderEntry(const QJsonObject& entry)
{
    QString html;
    for (const QJsonValue& romValue : entry.value(QLatin1String("roms")).toArray()) {
        const QJsonObject rom = romValue.toObject();
        QString headword = rom.value(QLatin1String("headword_full")).toString();
        if (headword.isEmpty())
            headword = rom.value(QLatin1String("headword")).toString();
        html += QStringLiteral("<p class=\"rom\"><span class=\"hw\">%1</span>").arg(headword);
        const QString wordclass = rom.value(QLatin1String("wordclass")).toString();
        if (!wordclass.isEmpty())
            html += QStringLiteral(" &nbsp;<span class=\"wc\">%1</span>").arg(wordclass.toHtmlEscaped());
        html += QStringLiteral("</p>");

        for (const QJsonValue& arabValue : rom.value(QLatin1String("arabs")).toArray()) {
            const QJsonObject arab = arabValue.toObject();
            const QString header = arab.value(QLatin1String("header")).toString();
            if (!header.isEmpty())
                html += QStringLiteral("<p class=\"arab\">%1</p>").arg(header);
            html += renderTranslations(arab.value(QLatin1String("translations")).toArray());
        }
    }
    return html;
}

QString renderHits(const QJsonArray& hits)
{
    QString html;
    QJsonArray looseTranslations; // hits found when PONS has no dictionary entry for the term
    for (const QJsonValue& value : hits) {
        const QJsonObject hit = value.toObject();
        const QString type = hit.value(QLatin1String("type")).toString();
        if (type == QLatin1String("entry")) {
            html += renderEntry(hit);
        } else if (type == QLatin1String("translation")) {
            looseTranslations.append(hit);
        } else if (type == QLatin1String("entry_with_secondary_entries")) {
            // e.g. "went": the primary entry, then the entries it refers to ("go").
            html += renderEntry(hit.value(QLatin1String("primary_entry")).toObject());
            for (const QJsonValue& secondary : hit.value(QLatin1String("secondary_entries")).toArray())
                html += renderEntry(secondary.toObject());
        }
    }
    if (!looseTranslations.isEmpty())
        html += renderTranslations(looseTranslations);
    return html;
}

QString resultStyleSheet(const QPalette& palette)
{
    return QStringLiteral(".hw { font-size: large; font-weight: bold; }"
                          ".rom { margin-top: 12px; margin-bottom: 2px; }"
                          ".arab { margin-top: 6px; margin-bottom: 0px; }"
                          ".wc, .example { font-style: italic; }"
                          ".wc, .arab, .src, .dir, .credit, .genus, .wordclass, .phonetics, .info, .rhetoric,"
                          " .style, .topic, .region { color: %1; }"
                          ".dir { font-weight: bold; }"
                          ".credit { font-size: small; margin-top: 14px; }")
        .arg(mutedColor(palette));
}

} // namespace

WordTab::WordTab(PonsClient* client, QWidget* parent)
    : QWidget(parent)
    , m_client(client)
    , m_direction(new QComboBox(this))
    , m_search(new QLineEdit(this))
    , m_lookupButton(new QPushButton(tr("Look up"), this))
    , m_results(new QTextBrowser(this))
{
    auto* swapButton = makeGlyphButton(Glyph::Swap, tr("Reverse direction"), this);
    m_search->setPlaceholderText(tr("Word or expression"));
    m_search->setClearButtonEnabled(true);
    m_results->setOpenLinks(false);
    m_results->setPlaceholderText(tr("Dictionary entries appear here."));

    auto* directionRow = new QHBoxLayout;
    directionRow->addWidget(m_direction, 1);
    directionRow->addWidget(swapButton);

    auto* searchRow = new QHBoxLayout;
    searchRow->addWidget(m_search, 1);
    searchRow->addWidget(m_lookupButton);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(directionRow);
    layout->addLayout(searchRow);
    layout->addWidget(m_results, 1);

    m_selectedDirection = QSettings().value(QStringLiteral("pons/direction"), QStringLiteral("enfr/fr")).toString();
    setDirections(PonsClient::defaultDirections());

    connect(m_direction, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            m_selectedDirection = directionId(m_directions.at(index));
    });
    connect(swapButton, &QToolButton::clicked, this, &WordTab::swapDirection);
    connect(m_search, &QLineEdit::returnPressed, this, &WordTab::lookup);
    connect(m_lookupButton, &QPushButton::clicked, this, &WordTab::lookup);

    connect(m_client, &PonsClient::directionsReady, this, &WordTab::setDirections);
    connect(m_client, &PonsClient::found, this, [this](const QJsonArray& result) {
        m_lookupButton->setEnabled(true);
        showResult(result);
    });
    connect(m_client, &PonsClient::notFound, this, [this] {
        m_lookupButton->setEnabled(true);
        showNotFound();
    });
    connect(m_client, &PonsClient::failed, this, [this](const QString& message) {
        m_lookupButton->setEnabled(true);
        emit statusChanged(message, true);
    });

    m_client->fetchDirections();
}

void WordTab::focusInput()
{
    m_search->setFocus();
    m_search->selectAll();
}

void WordTab::saveSettings() const
{
    QSettings().setValue(QStringLiteral("pons/direction"), m_selectedDirection);
}

void WordTab::setDirections(const QList<PonsDirection>& directions)
{
    const QSignalBlocker blocker(m_direction);
    m_directions = directions;
    m_direction->clear();

    int selected = 0;
    for (int i = 0; i < m_directions.size(); ++i) {
        m_direction->addItem(m_directions.at(i).label);
        if (directionId(m_directions.at(i)) == m_selectedDirection)
            selected = i;
    }
    m_direction->setCurrentIndex(selected);
}

void WordTab::swapDirection()
{
    const PonsDirection current = m_directions.value(m_direction->currentIndex());
    for (int i = 0; i < m_directions.size(); ++i) {
        const PonsDirection& candidate = m_directions.at(i);
        if (candidate.dictionary == current.dictionary && candidate.sourceLang != current.sourceLang) {
            m_direction->setCurrentIndex(i);
            break;
        }
    }
    focusInput();
}

void WordTab::lookup()
{
    const QString term = m_search->text().trimmed();
    const int index = m_direction->currentIndex();
    if (term.isEmpty() || index < 0)
        return;
    if (!m_client->hasApiKey()) {
        emit settingsRequested(); // modal: returns once the dialog is closed
        if (!m_client->hasApiKey()) {
            emit statusChanged(tr("A PONS API key is needed for lookups."), true);
            return;
        }
    }

    m_lastTerm = term;
    m_lookupButton->setEnabled(false);
    m_search->selectAll(); // typing the next word replaces this one
    emit statusChanged(tr("Looking up “%1”…").arg(term), false);
    m_client->lookup(term, m_directions.at(index));
}

void WordTab::showResult(const QJsonArray& result)
{
    QString html;
    for (const QJsonValue& value : result) {
        const QJsonObject direction = value.toObject();
        if (result.size() > 1) {
            html += QStringLiteral("<p class=\"dir\">%1 »</p>")
                        .arg(direction.value(QLatin1String("lang")).toString().toUpper());
        }
        html += renderHits(direction.value(QLatin1String("hits")).toArray());
    }
    if (html.isEmpty()) {
        showNotFound();
        return;
    }

    html += QStringLiteral("<p class=\"credit\">Source: PONS.com</p>");
    m_results->document()->setDefaultStyleSheet(resultStyleSheet(m_results->palette()));
    m_results->setHtml(html);
    emit statusChanged(QString(), false);
}

void WordTab::showNotFound()
{
    m_results->clear();
    emit statusChanged(tr("No entry found for “%1”.").arg(m_lastTerm), false);
}
