/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KConfigWatcher>
#include <KSharedConfig>

#include <QColor>
#include <QObject>
#include <QPalette>

class QWindow;

/*
 * The window's tools area (the toolbars and main menu bar under the title bar, drawn as
 * one surface with it) for Qt Quick applications: the [ToolsArea] settings as KToolsArea
 * (KF6::ColorScheme) resolves them for this application.
 * Publishes the result on qApp as _sonic_tools_area_* properties:
 *   headerColors (QPalette: Window/WindowText, active, inactive, disabled),
 *   separatorColor, blurBehind, opaqueWhenMaximized, titleBarHidden, translucent,
 *   serial (bumped on every change; nothing else is set while the tools area is off).
 * A window that sets _sonic_tools_area_region (QRegion) gets blur behind that region when
 * the tools area is translucent and blurBehind is on, unless it sets
 * _silver_skip_tools_blur. Every change applies live.
 */
class ToolsAreaSettings : public QObject {
    Q_OBJECT

public:
    // The config is a parameter for the tests.
    explicit ToolsAreaSettings(KSharedConfigPtr kdeglobals = KSharedConfig::openConfig(), QObject* parent = nullptr);
    ~ToolsAreaSettings() override;

    void reload();

    struct Values {
        bool available = false;
        QPalette headerColors;
        QColor separatorColor;
        bool blurBehind = false;
        bool opaqueWhenMaximized = false;
        bool titleBarHidden = false;
        bool translucent = false;
    };
    const Values& values() const;
    // Whether window should have blur behind its tools-area region now.
    bool wantsBlur(const QWindow* window) const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:
    void slotNotifyChange();

private:
    void delayedDBusConnects();
    void publish();
    void updateBlur(QWindow* window);

    KSharedConfigPtr m_kdeglobals;
    KConfigWatcher::Ptr m_watcher;
    Values m_values;
    int m_serial = 0;
};
