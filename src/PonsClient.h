#pragma once

#include <QJsonArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>

struct PonsDirection
{
    QString label;      // e.g. "French » English"
    QString dictionary; // e.g. "enfr": both languages in alphabetical order
    QString sourceLang; // e.g. "fr"
};

// Minimal client for the PONS Online Dictionary API (https://api.pons.com/v1).
class PonsClient : public QObject
{
    Q_OBJECT

public:
    explicit PonsClient(QObject* parent = nullptr);

    void setApiKey(const QString& key);
    bool hasApiKey() const { return !m_apiKey.isEmpty(); }

    // Used until fetchDirections() has received the full list.
    static QList<PonsDirection> defaultDirections();

    // Lists every translation direction PONS offers. This endpoint needs no API key.
    void fetchDirections();

    // A newer call cancels a pending one.
    void lookup(const QString& term, const PonsDirection& direction);

signals:
    void directionsReady(const QList<PonsDirection>& directions);
    void found(const QJsonArray& result);
    void notFound();
    void failed(const QString& message);

private:
    QNetworkAccessManager m_network;
    QString m_apiKey;
    QPointer<QNetworkReply> m_pending;
};
