cmake_minimum_required(VERSION 3.16)

project(PlasmaIntegration)
set(PROJECT_VERSION "6.7.4")

set(QT_MIN_VERSION "6.10.0")
set(KF6_MIN_VERSION "6.26.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)
set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)

find_package(ECM ${KF6_MIN_VERSION}  REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} )

# We need this because we can't include KDECMakeSettings here, because that
# would need KDEInstallDirs, which we can only include in the qt{5,6} subdirs
# to get the correct installation dirs for each Qt version.
option(BUILD_TESTING "Build the testing tree." ON)
if(BUILD_TESTING)
    enable_testing()
endif()

option(DEFAULT_UNION_STYLE "Use the Union QtQuick Controls Style by default." OFF)
if (DEFAULT_UNION_STYLE)
    add_definitions(-DDEFAULT_UNION_STYLE)
endif()

include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(GenerateExportHeader)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMDeprecationSettings)

find_package(XCB COMPONENTS XCB)
set_package_properties(XCB PROPERTIES
    DESCRIPTION "X protocol C-language Binding"
    URL "https://xcb.freedesktop.org"
    TYPE REQUIRED
    PURPOSE "Required to pass style properties to native Windows on X11 Platform"
)

find_package(FontNotoSans)
set_package_properties(FontNotoSans PROPERTIES
    PURPOSE "Default sans-serif font -- this is not detected automatically, pass -DCMAKE_DISABLE_FIND_PACKAGE_FontNotoSans=true to mark it ignored."
    URL "https://www.google.com/get/noto/"
    TYPE RUNTIME
)
find_package(FontNotoColorEmoji)
set_package_properties(FontNotoColorEmoji PROPERTIES
    PURPOSE "Default emoji font -- this is not detected automatically, pass -DCMAKE_DISABLE_FIND_PACKAGE_FontNotoColorEmoji=true to mark it ignored."
    URL "https://www.google.com/get/noto/"
    TYPE RUNTIME
)
find_package(FontHack)
set_package_properties(FontHack PROPERTIES
    PURPOSE "Default monospace font -- this is not detected automatically, pass -DCMAKE_DISABLE_FIND_PACKAGE_FontHack=true to mark it ignored."
    URL "https://sourcefoundry.org/hack/"
    TYPE RUNTIME
)
find_package(XDGDesktopPortalKDE)
set_package_properties(XDGDesktopPortalKDE PROPERTIES
    PURPOSE "Required for open-with dialog to work correctly -- this is not detected automatically, pass -DCMAKE_DISABLE_FIND_PACKAGE_XDGDesktopPortalKDE=true to mark it ignored."
    URL "https://invent.kde.org/plasma/xdg-desktop-portal-kde"
    TYPE RUNTIME
)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
list(FILTER ALL_CLANG_FORMAT_SOURCE_FILES EXCLUDE REGEX ".*\\.ccls-cache/.*")
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

unset(QUERY_EXECUTABLE CACHE)

add_definitions(-DTRANSLATION_DOMAIN=\"plasmaintegration5\")
add_subdirectory(qt6)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
