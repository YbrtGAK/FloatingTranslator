#pragma once

#include <QDialog>

class QLineEdit;

// Edits the DeepL and PONS API keys, which are stored with QSettings.
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    static QString deeplApiKey();
    static QString ponsApiKey();

    void accept() override;

private:
    QLineEdit* m_deeplKey;
    QLineEdit* m_ponsKey;
};
