cmake_minimum_required(VERSION 3.10)
project(screen_brightness_pro_plugin LANGUAGES CXX)

add_library(screen_brightness_pro SHARED
  "screen_brightness_pro_plugin.cc"
)

apply_standard_plugin_settings(screen_brightness_pro)

target_compile_definitions(screen_brightness_pro PRIVATE FLUTTER_PLUGIN_IMPL)

target_link_libraries(screen_brightness_pro flutter)
target_link_libraries(screen_brightness_pro PkgConfig::GTK)
target_link_libraries(screen_brightness_pro PkgConfig::GLIB)
