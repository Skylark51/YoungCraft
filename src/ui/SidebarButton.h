#pragma once

#include "UiIcons.h"

#include <QAbstractButton>
#include <QKeySequence>
#include <QStyle>

namespace youngcraft {

inline constexpr int sidebarButtonSize = 32;
inline constexpr int sidebarIconSize = 18;

// Accessible text should not announce Qt's mnemonic marker. Keep literal &&.
inline QString sidebarButtonLabel(const QString &text) {
    QString label;
    label.reserve(text.size());
    for (qsizetype i = 0; i < text.size(); ++i) {
        if (text[i] != QLatin1Char('&')) label += text[i];
        else if (i + 1 < text.size() && text[i + 1] == QLatin1Char('&')) {
            label += QLatin1Char('&');
            ++i;
        }
    }
    return label.trimmed();
}

inline void sizeSidebarButton(QAbstractButton *button) {
    button->setProperty("sidebarButton", true);
    button->setFixedSize(sidebarButtonSize, sidebarButtonSize);
    button->setIconSize(QSize(sidebarIconSize, sidebarIconSize));
    button->setFocusPolicy(Qt::StrongFocus);
    if (button->accessibleName().isEmpty())
        button->setAccessibleName(sidebarButtonLabel(button->text()));
}

// Does not resize or clear text: also safe for text buttons and split buttons.
inline void setSidebarButtonIcon(QAbstractButton *button, const QIcon &icon, const QString &label) {
    button->setIcon(icon);
    button->setAccessibleName(sidebarButtonLabel(label));
    button->setToolTip(label);
}

inline void setSidebarButtonIcon(QAbstractButton *button, QStyle::StandardPixmap icon, const QString &label) {
    setSidebarButtonIcon(button, button->style()->standardIcon(icon), label);
}

inline void setSidebarButtonIcon(QAbstractButton *button, UiIcon icon, const QString &label) {
    setSidebarButtonIcon(button, uiIcon(icon, button), label);
}

// Only for standalone icon buttons. Keep split-menu and explicit text buttons
// at their layout's natural size and use setSidebarButtonIcon() instead.
inline void makeSidebarIconButton(QAbstractButton *button, const QIcon &icon) {
    const auto label = button->accessibleName().isEmpty()
        ? sidebarButtonLabel(button->text()) : button->accessibleName();
    const auto hint = button->toolTip();
    const auto shortcut = button->shortcut();
    sizeSidebarButton(button);
    button->setText({});
    // Clearing text also removes an implicit &mnemonic in QAbstractButton.
    if (!shortcut.isEmpty()) button->setShortcut(shortcut);
    button->setIcon(icon);
    button->setAccessibleName(label);
    button->setToolTip(!hint.isEmpty() ? hint : shortcut.isEmpty() ? label
        : QStringLiteral("%1 (%2)").arg(label, shortcut.toString(QKeySequence::NativeText)));
}

inline void makeSidebarIconButton(QAbstractButton *button, QStyle::StandardPixmap icon) {
    makeSidebarIconButton(button, button->style()->standardIcon(icon));
}

inline void makeSidebarIconButton(QAbstractButton *button, UiIcon icon) {
    makeSidebarIconButton(button, uiIcon(icon, button));
}

// Compatibility helpers for existing callers. New callers should pass a widget
// context to uiIcon() to follow the palette of their containing window.
inline QIcon sidebarLockIcon() { return uiIcon(UiIcon::Lock); }
inline QIcon sidebarSaveIcon() { return uiIcon(UiIcon::Save); }
inline QIcon sidebarCopyIcon() { return uiIcon(UiIcon::Copy); }

} // namespace youngcraft
