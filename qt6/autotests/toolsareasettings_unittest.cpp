/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

// What the platform theme does with the [ToolsArea] settings (resolved by KToolsArea,
// tested in KF6::ColorScheme): publish them on qApp, give windows an alpha channel, and
// decide the blur; a theme without them changes nothing.

#include "../src/platformtheme/toolsareasettings.h"

#include <KConfigGroup>

#include <QGuiApplication>
#include <QQuickWindow>
#include <QRegion>
#include <QStandardPaths>
#include <QTest>
#include <QWindow>

namespace {
const QColor headerBackground(200, 210, 220);

// The theme's settings file itself (the default config is the application's, which only
// falls back to it).
KSharedConfigPtr globals()
{
    return KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
}
}

class ToolsAreaSettingsTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void testThemeWithoutToolsAreaChangesNothing();
    void testPublishedOnApplication();
    void testAppliesExceptionsForThisApplication();
    void testBlurDecision();
    void testBlurReturnsAfterApplicationBlur();

private:
    KConfigGroup toolsArea();
};

void ToolsAreaSettingsTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("toolsareatest"));
}

void ToolsAreaSettingsTest::init()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/kdeglobals");
    globals()->markAsClean();
    QFile::remove(path);
    KConfig config(path, KConfig::SimpleConfig);
    config.group(QStringLiteral("ColorEffects:Inactive")).writeEntry("Enable", false);
    config.group(QStringLiteral("Colors:Header")).writeEntry("BackgroundNormal", QStringLiteral("200,210,220"));
    config.group(QStringLiteral("Colors:Header")).writeEntry("ForegroundNormal", QStringLiteral("10,20,30"));
    config.sync();
    globals()->reparseConfiguration();
}

KConfigGroup ToolsAreaSettingsTest::toolsArea()
{
    return globals()->group(QStringLiteral("ToolsArea"));
}

void ToolsAreaSettingsTest::testThemeWithoutToolsAreaChangesNothing()
{
    ToolsAreaSettings settings(globals());
    QVERIFY(!settings.values().available);
    QVERIFY(!QQuickWindow::hasDefaultAlphaBuffer());
    for (const char* name : {"_sonic_tools_area_headerColors",
             "_sonic_tools_area_separatorColor",
             "_sonic_tools_area_blurBehind",
             "_sonic_tools_area_translucent"}) {
        QVERIFY2(!qApp->property(name).isValid(), name);
    }
    QWindow window;
    window.setProperty("_sonic_tools_area_region", QVariant::fromValue(QRegion(0, 0, 400, 40)));
    QVERIFY(!settings.wantsBlur(&window));
}

void ToolsAreaSettingsTest::testPublishedOnApplication()
{
    toolsArea().writeEntry("Enabled", true);
    ToolsAreaSettings settings(globals());
    const int serial = qApp->property("_sonic_tools_area_serial").toInt();
    QCOMPARE(qApp->property("_sonic_tools_area_headerColors").value<QPalette>().color(QPalette::Active, QPalette::Window), headerBackground);
    QCOMPARE(qApp->property("_sonic_tools_area_separatorColor").value<QColor>(), settings.values().separatorColor);
    QCOMPARE(qApp->property("_sonic_tools_area_blurBehind").toBool(), true);
    QVERIFY(QQuickWindow::hasDefaultAlphaBuffer());

    // A change applies on reload, without new windows; turning it off unpublishes.
    toolsArea().writeEntry("Enabled", false);
    settings.reload();
    QCOMPARE(qApp->property("_sonic_tools_area_serial").toInt(), serial + 1);
    QVERIFY(!qApp->property("_sonic_tools_area_headerColors").isValid());
}

void ToolsAreaSettingsTest::testAppliesExceptionsForThisApplication()
{
    toolsArea().writeEntry("Enabled", true);
    toolsArea().writeEntry("ActiveOpacity", 60);
    KConfigGroup exception = toolsArea().group(QStringLiteral("Application 0"));
    exception.writeEntry("Pattern", QStringLiteral("toolsarea.*"));
    exception.writeEntry("ActiveOpacity", 100);
    ToolsAreaSettings settings(globals());
    QCOMPARE(settings.values().headerColors.color(QPalette::Active, QPalette::Window), headerBackground);
}

void ToolsAreaSettingsTest::testBlurDecision()
{
    toolsArea().writeEntry("Enabled", true);
    toolsArea().writeEntry("ActiveOpacity", 60);
    toolsArea().writeEntry("InactiveOpacity", 60);
    ToolsAreaSettings settings(globals());
    QWindow window;
    QVERIFY(!settings.wantsBlur(&window));
    window.setProperty("_sonic_tools_area_region", QVariant::fromValue(QRegion(0, 0, 400, 40)));
    QVERIFY(settings.wantsBlur(&window));
    window.setProperty("_silver_skip_tools_blur", true);
    QVERIFY(!settings.wantsBlur(&window));
    window.setProperty("_silver_skip_tools_blur", QVariant());
    window.setWindowStates(Qt::WindowMaximized);
    QVERIFY(!settings.wantsBlur(&window));

    // Opaque again: no blur.
    toolsArea().writeEntry("ActiveOpacity", 100);
    toolsArea().writeEntry("InactiveOpacity", 100);
    settings.reload();
    window.setWindowStates(Qt::WindowNoState);
    QVERIFY(!settings.wantsBlur(&window));
}

void ToolsAreaSettingsTest::testBlurReturnsAfterApplicationBlur()
{
    if (QGuiApplication::platformName() != QLatin1String("xcb")) {
        QSKIP("The blur needs a native X11 window");
    }
    toolsArea().writeEntry("Enabled", true);
    toolsArea().writeEntry("ActiveOpacity", 60);
    toolsArea().writeEntry("InactiveOpacity", 60);
    ToolsAreaSettings settings(globals());
    QWindow window;
    window.create();
    const QRegion region(0, 0, 400, 40);
    window.setProperty("_sonic_tools_area_region", QVariant::fromValue(region));
    QCOMPARE(window.property("_sonic_tools_area_blurred").value<QRegion>(), region);

    // The application blurs the whole window, then stops: the tools area is blurred again.
    window.setProperty("_silver_skip_tools_blur", true);
    QVERIFY(!window.property("_sonic_tools_area_blurred").isValid());
    window.setProperty("_silver_skip_tools_blur", false);
    QCOMPARE(window.property("_sonic_tools_area_blurred").value<QRegion>(), region);
}

QTEST_MAIN(ToolsAreaSettingsTest)

#include "toolsareasettings_unittest.moc"
