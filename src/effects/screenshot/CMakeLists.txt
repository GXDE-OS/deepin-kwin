#######################################
# Effect

# Source files
set(screenshot_SOURCES
    main.cpp
    screenshot.cpp
    screenshotdbusinterface1.cpp
    screenshotdbusinterface2.cpp
)

ecm_qt_declare_logging_category(screenshot_SOURCES
    HEADER screenshotlogging.h
    IDENTIFIER KWIN_SCREENSHOT
    CATEGORY_NAME kwin_effect_screenshot
    DEFAULT_SEVERITY Warning
)

if (DEFINED DEFINE_DDE_DOCK_PATH)
  add_compile_definitions(DEFINE_DDE_DOCK_PATH="${DEFINE_DDE_DOCK_PATH}/")
else()
  add_compile_definitions(DEFINE_DDE_DOCK_PATH="/usr/bin/")
endif()

qt_add_dbus_adaptor(screenshot_SOURCES org.kde.KWin.ScreenShot2.xml screenshotdbusinterface2.h KWin::ScreenShotDBusInterface2)

kwin4_add_effect_module(kwin4_effect_screenshot ${screenshot_SOURCES})
