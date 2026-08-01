#include "include/screen_brightness_pro/screen_brightness_pro_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>
#include <fstream>
#include <iostream>

#define SCREEN_BRIGHTNESS_PRO_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), screen_brightness_pro_plugin_get_type(), \
                              ScreenBrightnessProPlugin))

struct _ScreenBrightnessProPlugin {
  GObject parent_instance;
};

G_DEFINE_TYPE(ScreenBrightnessProPlugin, screen_brightness_pro_plugin, g_object_get_type())

static void screen_brightness_pro_plugin_handle_method_call(
    ScreenBrightnessProPlugin* self,
    FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, "getBrightness") == 0) {
    // Return a default value for Linux
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_float(0.7)));
  } else if (strcmp(method, "setBrightness") == 0 || strcmp(method, "setSystemBrightness") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
  } else if (strcmp(method, "isAutoModeEnabled") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(false)));
  } else if (strcmp(method, "setAutoMode") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
  } else if (strcmp(method, "hasWriteSettingsPermission") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(true)));
  } else if (strcmp(method, "requestWriteSettingsPermission") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
  } else if (strcmp(method, "resetBrightness") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
  } else if (strcmp(method, "getBatteryLevel") == 0) {
    double level = -1.0;
    std::ifstream batteryFile("/sys/class/power_supply/BAT0/capacity");
    if (batteryFile.is_open()) {
        int capacity;
        batteryFile >> capacity;
        level = capacity / 100.0;
        batteryFile.close();
    }
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_float(level)));
  } else if (strcmp(method, "isLowPowerModeEnabled") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(false)));
  } else if (strcmp(method, "setKeepScreenOn") == 0) {
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void screen_brightness_pro_plugin_dispose(GObject* object) {
  G_OBJECT_CLASS(screen_brightness_pro_plugin_parent_class)->dispose(object);
}

static void screen_brightness_pro_plugin_class_init(ScreenBrightnessProPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = screen_brightness_pro_plugin_dispose;
}

static void screen_brightness_pro_plugin_init(ScreenBrightnessProPlugin* self) {}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  ScreenBrightnessProPlugin* self = SCREEN_BRIGHTNESS_PRO_PLUGIN(user_data);
  screen_brightness_pro_plugin_handle_method_call(self, method_call);
}

void screen_brightness_pro_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  ScreenBrightnessProPlugin* self = SCREEN_BRIGHTNESS_PRO_PLUGIN(
      g_object_new(screen_brightness_pro_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "screen_brightness_pro", FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(channel, method_call_cb, self,
                                            g_object_unref);
}
