#pragma once

#include <QColor>
#include <QFont>
#include <QPalette>
#include <QString>
#include <QToolButton>

// Icon glyphs from Segoe Fluent Icons (Windows 11), with Segoe MDL2 Assets (Windows 10) as fallback.
namespace Glyph {
constexpr char16_t Pin = 0xE718;
constexpr char16_t Pinned = 0xE840;
constexpr char16_t Settings = 0xE713;
constexpr char16_t Minimize = 0xE921;
constexpr char16_t Close = 0xE8BB;
constexpr char16_t Swap = 0xE8AB;
} // namespace Glyph

inline constexpr char kErrorColor[] = "#d13438";

inline QToolButton* makeGlyphButton(char16_t glyph, const QString& toolTip, QWidget* parent)
{
    auto* button = new QToolButton(parent);
    QFont font = button->font();
    font.setFamilies({QStringLiteral("Segoe Fluent Icons"), QStringLiteral("Segoe MDL2 Assets")});
    button->setFont(font);
    button->setText(QString(QChar(glyph)));
    button->setToolTip(toolTip);
    button->setAutoRaise(true);
    return button;
}

// Text color faded toward the background, readable in both light and dark themes.
inline QString mutedColor(const QPalette& palette)
{
    const QColor text = palette.color(QPalette::Text);
    const QColor base = palette.color(QPalette::Base);
    const auto mix = [](int a, int b) { return (3 * a + 2 * b) / 5; };
    return QColor(mix(text.red(), base.red()), mix(text.green(), base.green()), mix(text.blue(), base.blue()))
        .name();
}
