#pragma once

#include "PonsClient.h"

#include <QWidget>

class QComboBox;
class QJsonArray;
class QLineEdit;
class QPushButton;
class QTextBrowser;

// "Word" tab: looks up words and expressions in the PONS dictionary.
class WordTab : public QWidget
{
    Q_OBJECT

public:
    explicit WordTab(PonsClient* client, QWidget* parent = nullptr);

    void focusInput();
    void saveSettings() const;

signals:
    void statusChanged(const QString& message, bool isError);
    void settingsRequested();

private:
    void setDirections(const QList<PonsDirection>& directions);
    void swapDirection();
    void lookup();
    void showResult(const QJsonArray& result);
    void showNotFound();

    PonsClient* m_client;
    QComboBox* m_direction;
    QLineEdit* m_search;
    QPushButton* m_lookupButton;
    QTextBrowser* m_results;
    QList<PonsDirection> m_directions; // same order as the m_direction items
    QString m_selectedDirection;       // "dictionary/source", e.g. "enfr/fr"
    QString m_lastTerm;
};
