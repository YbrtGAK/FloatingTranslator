#include "SettingsDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QVBoxLayout>

namespace {

const QString kDeepLApiKey = QStringLiteral("deepl/apiKey");
const QString kPonsApiKey = QStringLiteral("pons/apiKey");

QLabel* makeLinkLabel(const QString& html, QWidget* parent)
{
    auto* label = new QLabel(html, parent);
    label->setTextFormat(Qt::RichText);
    label->setOpenExternalLinks(true);
    return label;
}

} // namespace

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_deeplKey(new QLineEdit(deeplApiKey(), this))
    , m_ponsKey(new QLineEdit(ponsApiKey(), this))
{
    setWindowTitle(tr("API keys"));

    for (QLineEdit* edit : {m_deeplKey, m_ponsKey}) {
        edit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
        edit->setMinimumWidth(300);
    }

    auto* form = new QFormLayout;
    form->addRow(tr("DeepL API key"), m_deeplKey);
    form->addRow(QString(), makeLinkLabel(tr("Free plan, 500,000 characters/month: "
                                             "<a href=\"https://www.deepl.com/pro-api\">deepl.com/pro-api</a>"),
                                          this));
    form->addRow(tr("PONS API key"), m_ponsKey);
    form->addRow(QString(),
                 makeLinkLabel(tr("Free quota, 1,000 lookups/month: <a "
                                  "href=\"https://en.pons.com/p/online-dictionary/developers/api\">PONS API</a>"),
                               this));

    auto* storageNote = new QLabel(tr("Keys are saved in the registry, under your Windows user account."), this);
    storageNote->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(storageNote);
    layout->addWidget(buttons);
}

QString SettingsDialog::deeplApiKey()
{
    return QSettings().value(kDeepLApiKey).toString();
}

QString SettingsDialog::ponsApiKey()
{
    return QSettings().value(kPonsApiKey).toString();
}

void SettingsDialog::accept()
{
    QSettings settings;
    settings.setValue(kDeepLApiKey, m_deeplKey->text().trimmed());
    settings.setValue(kPonsApiKey, m_ponsKey->text().trimmed());
    QDialog::accept();
}
