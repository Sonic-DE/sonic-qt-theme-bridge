/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "toolsareasettings.h"

#include <KConfigGroup>
#include <KToolsArea>
#include <KWindowEffects>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QDynamicPropertyChangeEvent>
#include <QGuiApplication>
#include <QPlatformSurfaceEvent>
#include <QPointer>
#include <QQuickWindow>
#include <QRegion>
#include <QWindow>

namespace {
const char regionProperty[] = "_sonic_tools_area_region";
const char skipBlurProperty[] = "_silver_skip_tools_blur";
const char blurredProperty[] = "_sonic_tools_area_blurred";

}

ToolsAreaSettings::ToolsAreaSettings(KSharedConfigPtr kdeglobals, QObject* parent)
    : QObject(parent)
    , m_kdeglobals(std::move(kdeglobals))
{
    reload();
    // With a theme that uses the tools area, every Quick window gets an alpha channel from
    // the start, so the tools area can turn translucent live. Other themes keep opaque
    // windows (an application's transparent window colour keeps its old meaning).
    if (m_values.available) {
        QQuickWindow::setDefaultAlphaBuffer(true);
    }
    if (QCoreApplication* app = QCoreApplication::instance()) {
        app->installEventFilter(this);
    }
    m_watcher = KConfigWatcher::create(m_kdeglobals);
    connect(m_watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup& group) {
        if (group.name() == QLatin1String("ToolsArea") || group.parent().name() == QLatin1String("ToolsArea")) {
            reload();
        }
    });
    // As KHintsSettings: the session bus may not be up while the application is created.
    QMetaObject::invokeMethod(this, &ToolsAreaSettings::delayedDBusConnects, Qt::QueuedConnection);
}

ToolsAreaSettings::~ToolsAreaSettings() = default;

void ToolsAreaSettings::delayedDBusConnects()
{
    QDBusConnection::sessionBus().connect(QString(),
        QStringLiteral("/KGlobalSettings"),
        QStringLiteral("org.kde.KGlobalSettings"),
        QStringLiteral("notifyChange"),
        this,
        SLOT(slotNotifyChange()));
}

void ToolsAreaSettings::slotNotifyChange()
{
    m_kdeglobals->reparseConfiguration();
    reload();
}

const ToolsAreaSettings::Values& ToolsAreaSettings::values() const
{
    return m_values;
}

void ToolsAreaSettings::reload()
{
    Values values;
    // The colour scheme, by the KDE_COLOR_SCHEME_PATH rule the platform theme's palette
    // follows (KHintsSettings).
    const QString schemePath = qApp ? qApp->property("KDE_COLOR_SCHEME_PATH").toString() : QString();
    const bool globalScheme = schemePath.isEmpty() || schemePath == QStringLiteral("kdeglobals");
    const KSharedConfigPtr scheme = globalScheme ? m_kdeglobals : KSharedConfig::openConfig(schemePath, KConfig::SimpleConfig);
    const KToolsArea::Settings settings = KToolsArea::settings(scheme, QCoreApplication::applicationName());

    if (settings.enabled) {
        values.available = true;
        const KToolsArea::Colors colors = KToolsArea::colors(scheme, settings);
        values.headerColors = colors.palette;
        values.translucent = colors.translucent;
        values.separatorColor = colors.separatorColor;
        values.blurBehind = settings.blurBehind;
        values.opaqueWhenMaximized = settings.opaqueWhenMaximized;
        values.titleBarHidden = settings.hideTitleBar != 0;
    }
    m_values = values;
    publish();
    for (QWindow* window : QGuiApplication::allWindows()) {
        updateBlur(window);
    }
}

void ToolsAreaSettings::publish()
{
    QCoreApplication* app = QCoreApplication::instance();
    if (!app) {
        return;
    }
    const bool on = m_values.available;
    const auto set = [app](const char* name, const QVariant& value) {
        app->setProperty(name, value);
    };
    set("_sonic_tools_area_headerColors", on ? QVariant::fromValue(m_values.headerColors) : QVariant());
    set("_sonic_tools_area_separatorColor", on ? QVariant::fromValue(m_values.separatorColor) : QVariant());
    set("_sonic_tools_area_blurBehind", on ? QVariant(m_values.blurBehind) : QVariant());
    set("_sonic_tools_area_opaqueWhenMaximized", on ? QVariant(m_values.opaqueWhenMaximized) : QVariant());
    set("_sonic_tools_area_titleBarHidden", on ? QVariant(m_values.titleBarHidden) : QVariant());
    set("_sonic_tools_area_translucent", on ? QVariant(m_values.translucent) : QVariant());
    // Last: readers react to this one.
    set("_sonic_tools_area_serial", ++m_serial);
}

bool ToolsAreaSettings::wantsBlur(const QWindow* window) const
{
    if (!m_values.available || !m_values.blurBehind || window->property(skipBlurProperty).toBool()) {
        return false;
    }
    if (window->property(regionProperty).value<QRegion>().isEmpty()) {
        return false;
    }
    const bool maximized = window->windowStates() & (Qt::WindowMaximized | Qt::WindowFullScreen);
    if (maximized && m_values.opaqueWhenMaximized) {
        return false;
    }
    const auto group = window->isActive() ? QPalette::Active : QPalette::Inactive;
    return m_values.headerColors.color(group, QPalette::Window).alpha() < 255;
}

void ToolsAreaSettings::updateBlur(QWindow* window)
{
    // Only windows with a platform window; PlatformSurface brings the others back here.
    if (!window->handle()) {
        return;
    }
    // The region last applied; unset when this class hasn't blurred the window.
    const QVariant blurred = window->property(blurredProperty);
    if (wantsBlur(window)) {
        const QRegion region = window->property(regionProperty).value<QRegion>();
        if (!blurred.isValid() || blurred.value<QRegion>() != region) {
            KWindowEffects::enableBlurBehind(window, true, region);
            window->setProperty(blurredProperty, QVariant::fromValue(region));
            qDebug() << "Tools area blur on" << window << region.boundingRect();
        }
    } else if (blurred.isValid() && !window->property(skipBlurProperty).toBool()) {
        // Only a blur this class set is cleared; an application's own is left alone.
        KWindowEffects::enableBlurBehind(window, false);
        window->setProperty(blurredProperty, QVariant());
        qDebug() << "Tools area blur off" << window;
    }
}

bool ToolsAreaSettings::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {
    case QEvent::DynamicPropertyChange: {
        const QByteArray name = static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName();
        if (watched == QCoreApplication::instance() && name == "KDE_COLOR_SCHEME_PATH") {
            reload();
        } else if (auto* window = qobject_cast<QWindow*>(watched); window && (name == regionProperty || name == skipBlurProperty)) {
            if (name == skipBlurProperty) {
                // The application's own blur replaced ours or was cleared with it: what this
                // class applied is gone, so the tools area is blurred again when wanted.
                window->setProperty(blurredProperty, QVariant());
            }
            updateBlur(window);
        }
        break;
    }
    case QEvent::PlatformSurface:
        if (auto* window = qobject_cast<QWindow*>(watched);
            window && static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            QMetaObject::invokeMethod(
                this,
                [this, target = QPointer<QWindow>(window)] {
                    if (target) {
                        updateBlur(target);
                    }
                },
                Qt::QueuedConnection);
        }
        break;
    case QEvent::WindowStateChange:
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        if (auto* window = qobject_cast<QWindow*>(watched)) {
            updateBlur(window);
        }
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

#include "moc_toolsareasettings.cpp"
