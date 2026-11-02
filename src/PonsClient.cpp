#include "PonsClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <algorithm>

// Labels (directions, word classes) are requested in English (language=en) to match the UI.

namespace {

constexpr int kTimeoutMs = 15000;

} // namespace

PonsClient::PonsClient(QObject* parent)
    : QObject(parent)
{
}

void PonsClient::setApiKey(const QString& key)
{
    m_apiKey = key.trimmed();
}

QList<PonsDirection> PonsClient::defaultDirections()
{
    return {
        {QStringLiteral("English » French"), QStringLiteral("enfr"), QStringLiteral("en")},
        {QStringLiteral("French » English"), QStringLiteral("enfr"), QStringLiteral("fr")},
        {QStringLiteral("English » German"), QStringLiteral("deen"), QStringLiteral("en")},
        {QStringLiteral("German » English"), QStringLiteral("deen"), QStringLiteral("de")},
        {QStringLiteral("English » Spanish"), QStringLiteral("enes"), QStringLiteral("en")},
        {QStringLiteral("Spanish » English"), QStringLiteral("enes"), QStringLiteral("es")},
    };
}

void PonsClient::fetchDirections()
{
    QNetworkRequest request(QUrl(QStringLiteral("https://api.pons.com/v1/dictionaries?language=en")));
    request.setTransferTimeout(kTimeoutMs);

    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return; // keep the default list

        QList<PonsDirection> directions;
        const QJsonArray dictionaries = QJsonDocument::fromJson(reply->readAll()).array();
        for (const QJsonValue& value : dictionaries) {
            const QJsonObject dictionary = value.toObject();
            if (dictionary.value(QLatin1String("languages")).toArray().size() < 2)
                continue; // monolingual dictionary
            const QString key = dictionary.value(QLatin1String("key")).toString();
            // Each directed key starts with the source language: "fren" is French » English.
            const QJsonObject labels = dictionary.value(QLatin1String("directed_label")).toObject();
            for (auto it = labels.constBegin(); it != labels.constEnd(); ++it)
                directions.append(PonsDirection{it.value().toString(), key, it.key().left(2)});
        }
        if (directions.isEmpty())
            return;

        std::sort(directions.begin(), directions.end(), [](const PonsDirection& a, const PonsDirection& b) {
            return QString::localeAwareCompare(a.label, b.label) < 0;
        });
        emit directionsReady(directions);
    });
}

void PonsClient::lookup(const QString& term, const PonsDirection& direction)
{
    if (QNetworkReply* previous = m_pending) {
        m_pending.clear();
        previous->abort();
    }

    // Built by hand so that characters such as '+' or '&' in the term are percent-encoded.
    QUrl url(QStringLiteral("https://api.pons.com/v1/dictionary"));
    url.setQuery(QStringLiteral("q=%1&l=%2&in=%3&ref=true&language=en")
                     .arg(QString::fromLatin1(QUrl::toPercentEncoding(term)), direction.dictionary,
                          direction.sourceLang));

    QNetworkRequest request(url);
    request.setRawHeader("X-Secret", m_apiKey.toUtf8());
    request.setTransferTimeout(kTimeoutMs);

    QNetworkReply* reply = m_network.get(request);
    m_pending = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply != m_pending)
            return; // superseded by a newer lookup
        m_pending.clear();

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        switch (status) {
        case 200: {
            const QJsonArray result = QJsonDocument::fromJson(reply->readAll()).array();
            if (result.isEmpty())
                emit notFound();
            else
                emit found(result);
            break;
        }
        case 204:
            emit notFound();
            break;
        case 403:
            emit failed(tr("PONS rejected the API key, or this dictionary isn't part of your plan."));
            break;
        case 404:
            emit failed(tr("PONS answered \"not found\": check the API key and the dictionary."));
            break;
        case 503:
            emit failed(tr("Daily PONS query limit reached."));
            break;
        case 0: // no HTTP answer at all
            emit failed(reply->error() == QNetworkReply::OperationCanceledError ? tr("PONS didn't answer in time.")
                                                                                : reply->errorString());
            break;
        default:
            emit failed(tr("PONS error (HTTP %1).").arg(status));
            break;
        }
    });
}
