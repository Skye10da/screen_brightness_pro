#include "include/screen_brightness_pro/screen_brightness_pro_plugin_c_api.h"
#include "include/screen_brightness_pro/screen_brightness_pro_plugin.h"

#include <flutter/plugin_registrar_windows.h>

#include "screen_brightness_pro_plugin.h"

void ScreenBrightnessProPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  screen_brightness_pro::ScreenBrightnessProPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}

void ScreenBrightnessProPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  ScreenBrightnessProPluginCApiRegisterWithRegistrar(registrar);
}
