# Plasma Integration

Integrates Qt applications with the KDE workspace by providing a QPlatformTheme. Applications do not need to link to this directly.

This plugin provides [the KdePlatformTheme](/qt6/src/platformtheme/kdeplatformtheme.h). It handles integrating Qt applications, such as displaying native KDE file dialogs. The theme also controls other miscellaneous integrations like setting the default Qt Quick Controls style and fonts.

## Building

The easiest way to make changes and test Plasma Integration during development is to [build it with kde-builder](https://develop.kde.org/docs/getting-started/building).

When building Plasma Integration manually, only the Qt6 version is built.
