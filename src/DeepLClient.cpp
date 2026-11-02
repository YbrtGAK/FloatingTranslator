#include "DeepLClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {

constexpr int kTimeoutMs = 15000;

QString httpErrorMessage(int status, const QByteArray& body)
{
    switch (status) {
    case 403:
        return DeepLClient::tr("DeepL rejected the API key.");
    case 429:
        return DeepLClient::tr("Too many requests to DeepL, wait a moment.");
    case 456:
        return DeepLClient::tr("Monthly DeepL character quota reached.");
    }
    const QString detail = QJsonDocument::fromJson(body).object().value(QLatin1String("message")).toString();
    return detail.isEmpty() ? DeepLClient::tr("DeepL error (HTTP %1).").arg(status)
                            : DeepLClient::tr("DeepL error (HTTP %1): %2").arg(QString::number(status), detail);
}

} // namespace

DeepLClient::DeepLClient(QObject* parent)
    : QObject(parent)
{
}

void DeepLClient::setApiKey(const QString& key)
{
    m_apiKey = key.trimmed();
}

void DeepLClient::translate(const QString& text, const QString& sourceLang, const QString& targetLang)
{
    if (QNetworkReply* previous = m_pending) {
        m_pending.clear();
        previous->abort();
    }

    // Keys of the free plan end with ":fx" and only work on the free endpoint.
    const QUrl url(m_apiKey.endsWith(QLatin1String(":fx")) ? QStringLiteral("https://api-free.deepl.com/v2/translate")
                                                            : QStringLiteral("https://api.deepl.com/v2/translate"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", "DeepL-Auth-Key " + m_apiKey.toUtf8());
    request.setTransferTimeout(kTimeoutMs);

    QJsonObject body{
        {QStringLiteral("text"), QJsonArray{text}},
        {QStringLiteral("target_lang"), targetLang},
    };
    if (!sourceLang.isEmpty())
        body.insert(QStringLiteral("source_lang"), sourceLang);

    QNetworkReply* reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_pending = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply != m_pending)
            return; // superseded by a newer request
        m_pending.clear();

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 0) { // no HTTP answer at all
            emit failed(reply->error() == QNetworkReply::OperationCanceledError ? tr("DeepL didn't answer in time.")
                                                                                : reply->errorString());
            return;
        }
        const QByteArray data = reply->readAll();
        if (status != 200) {
            emit failed(httpErrorMessage(status, data));
            return;
        }

        const QJsonArray translations =
            QJsonDocument::fromJson(data).object().value(QLatin1String("translations")).toArray();
        if (translations.isEmpty()) {
            emit failed(tr("Unexpected answer from DeepL."));
            return;
        }
        const QJsonObject first = translations.first().toObject();
        emit translated(first.value(QLatin1String("text")).toString(),
                        first.value(QLatin1String("detected_source_language")).toString());
    });
}
