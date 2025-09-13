# SPDX-FileCopyrightText: Budgie Desktop Developers
#
# SPDX-License-Identifier: MPL-2.0

cmake_minimum_required(VERSION 3.20)

project(budgie-daemon-v2 VERSION 0.0.1)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_WARNINGS_IGNORE "deprecated-literal-operators")

set(KDE_COMPILERSETTINGS_LEVEL 6.0)

set(PROJECT_DEP_VERSION "6.1.80")
set(QT_MIN_VERSION "6.7")
set(KF6_MIN_VERSION "6.6.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(FetchContent)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(FeatureSummary)
include(ECMSetupVersion)
include(KDEInstallDirs)
include(KDECMakeSettings)
# include(KDECompilerSettings NO_POLICY_SCOPE)
include(GenerateExportHeader)
include(ECMGenerateHeaders)

find_package(Qt6 ${QT_MIN_VERSION} NO_MODULE COMPONENTS Core DBus WaylandClient)

set_package_properties(
  Qt6 PROPERTIES
  TYPE REQUIRED
  PURPOSE "Basic application components")

find_package(Wayland REQUIRED)
find_package(QtWaylandScanner REQUIRED)
find_package(KWayland REQUIRED)

# Please do not build the universe with toml11
set(TOML11_BUILD_EXAMPLES OFF)
set(TOML11_BUILD_TESTS OFF)

FetchContent_Declare(
  toml11
  FIND_PACKAGE_ARGS NAMES toml11
  GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
  GIT_SUBMODULES ""
  GIT_TAG        v4.4.0
)
FetchContent_MakeAvailable(toml11)

option(INSTALL_SERVICE_FILES "Install service files for autostarting" ON)
option(INSTALL_LABWC "Install autostart files for labwc" ON)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES
                         FATAL_ON_MISSING_REQUIRED_PACKAGES)

#file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.hpp)
#kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

#kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
