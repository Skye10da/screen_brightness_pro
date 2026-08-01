import Cocoa
import FlutterMacOS
import IOKit
import IOKit.pwr_mgt
import IOKit.ps

public class ScreenBrightnessProPlugin: NSObject, FlutterPlugin, FlutterStreamHandler {
  private var eventSink: FlutterEventSink?

  public static func register(with registrar: FlutterPluginRegistrar) {
    let methodChannel = FlutterMethodChannel(name: "screen_brightness_pro", binaryMessenger: registrar.messenger)
    let eventChannel = FlutterEventChannel(name: "screen_brightness_pro_events", binaryMessenger: registrar.messenger)
    
    let instance = ScreenBrightnessProPlugin()
    
    registrar.addMethodCallDelegate(instance, channel: methodChannel)
    eventChannel.setStreamHandler(instance)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "getBrightness":
      result(getBrightness())
    case "setBrightness":
      if let brightness = call.arguments as? Double {
        setBrightness(brightness: brightness)
        result(nil)
      } else {
        result(FlutterError(code: "INVALID_ARGUMENT", message: "Brightness must be a double", details: nil))
      }
    case "isAutoModeEnabled":
      result(false)
    case "setAutoMode":
      result(nil)
    case "hasWriteSettingsPermission":
      result(true)
    case "requestWriteSettingsPermission":
      result(nil)
    case "resetBrightness":
      result(nil)
    case "setSystemBrightness":
      if let brightness = call.arguments as? Double {
        setBrightness(brightness: brightness)
        result(nil)
      } else {
        result(FlutterError(code: "INVALID_ARGUMENT", message: "Brightness must be a double", details: nil))
      }
    case "setKeepScreenOn":
      if let enabled = call.arguments as? Bool {
        setKeepScreenOn(enabled: enabled)
        result(nil)
      } else {
        result(FlutterError(code: "INVALID_ARGUMENT", message: "Enabled must be a boolean", details: nil))
      }
    case "getBatteryLevel":
      result(getMacOSBatteryLevel())
    case "isLowPowerModeEnabled":
      if #available(macOS 12.0, *) {
        result(ProcessInfo.processInfo.isLowPowerModeEnabled)
      } else {
        result(false)
      }
    default:
      result(FlutterMethodNotImplemented)
    }
  }

  public func onListen(withArguments arguments: Any?, eventSink events: @escaping FlutterEventSink) -> FlutterError? {
    self.eventSink = events
    // Brightness change notification on macOS is often handled via custom timers or checking IOKit
    // For now, we'll provide a consistent skeleton that mirrors iOS.
    return nil
  }

  public func onCancel(withArguments arguments: Any?) -> FlutterError? {
    self.eventSink = nil
    return nil
  }

  private func getMacOSBatteryLevel() -> Double {
    let blob = IOPSCopyPowerSourcesInfo().takeRetainedValue()
    let sources = IOPSCopyPowerSourcesList(blob).takeRetainedValue() as [CFTypeRef]
    
    for ps in sources {
      if let desc = IOPSGetPowerSourceDescription(blob, ps).takeUnretainedValue() as? [String: Any] {
        if let level = desc[kIOPSCurrentCapacityKey] as? Double,
           let max = desc[kIOPSMaxCapacityKey] as? Double {
          return level / max
        }
      }
    }
    return -1.0
  }

  private var assertionID: IOPMAssertionID = 0

  private func setKeepScreenOn(enabled: Bool) {
    if enabled {
      if assertionID == 0 {
        IOPMAssertionCreateWithDescription(
          kIOPMAssertionTypeNoDisplaySleep as CFString,
          "ScreenBrightnessPro keeping screen awake" as CFString,
          nil, nil, nil, 0, nil, &assertionID
        )
      }
    } else {
      if assertionID != 0 {
        IOPMAssertionRelease(assertionID)
        assertionID = 0
      }
    }
  }

  private func getBrightness() -> Double {
    var brightness: Float = 0.5
    let service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IODisplayConnect"))
    if service != 0 {
      var level: Float = 0.0
      IODisplayGetFloatParameter(service, 0, kIODisplayBrightnessKey as CFString, &level)
      brightness = level
      IOObjectRelease(service)
    }
    return Double(brightness)
  }

  private func setBrightness(brightness: Double) {
    let service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IODisplayConnect"))
    if service != 0 {
      IODisplaySetFloatParameter(service, 0, kIODisplayBrightnessKey as CFString, Float(brightness))
      IOObjectRelease(service)
    }
  }
}
