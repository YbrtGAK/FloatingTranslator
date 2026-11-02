#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>

// Minimal client for the DeepL REST API (https://developers.deepl.com).
class DeepLClient : public QObject
{
    Q_OBJECT

public:
    explicit DeepLClient(QObject* parent = nullptr);

    void setApiKey(const QString& key);
    bool hasApiKey() const { return !m_apiKey.isEmpty(); }

    // An empty sourceLang lets DeepL detect the language. A newer call cancels a pending one.
    void translate(const QString& text, const QString& sourceLang, const QString& targetLang);

signals:
    void translated(const QString& text, const QString& detectedSourceLang);
    void failed(const QString& message);

private:
    QNetworkAccessManager m_network;
    QString m_apiKey;
    QPointer<QNetworkReply> m_pending;
};
