/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QtGlobal>

// These tests check what the application gets from this repository's platform theme, so the
// application must load the freshly built plugin, not the session's or an installed theme.
static void useBuiltPlatformTheme()
{
    QByteArray pluginPath = PLATFORMTHEME_PLUGIN_ROOT;
    const QByteArray inherited = qgetenv("QT_PLUGIN_PATH");
    if (!inherited.isEmpty()) {
        pluginPath += ':' + inherited;
    }
    qputenv("QT_PLUGIN_PATH", pluginPath);
    qputenv("QT_QPA_PLATFORMTHEME", "kde");
}

Q_CONSTRUCTOR_FUNCTION(useBuiltPlatformTheme)
