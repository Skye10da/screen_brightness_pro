#include "screen_brightness_pro_plugin.h"

// This must be included before many other Windows headers.
#include <Wbemcli.h>
#include <windows.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>

#pragma comment(lib, "wbemuuid.lib")
namespace {
class BstrHolder {
 public:
  explicit BstrHolder(const wchar_t *s) : bstr_(::SysAllocString(s ? s : L"")) {}
  explicit BstrHolder(const char *s) : bstr_(nullptr) {
    if (s) {
      int wlen = ::MultiByteToWideChar(CP_ACP, 0, s, -1, nullptr, 0);
      if (wlen > 0) {
        bstr_ = ::SysAllocStringLen(nullptr, wlen - 1);
        ::MultiByteToWideChar(CP_ACP, 0, s, -1, bstr_, wlen);
      }
    }
  }
  ~BstrHolder() { ::SysFreeString(bstr_); }
  BstrHolder(const BstrHolder &) = delete;
  BstrHolder &operator=(const BstrHolder &) = delete;
  operator BSTR() const { return bstr_; }

 private:
  BSTR bstr_;
};
}  // namespace


namespace screen_brightness_pro {

// static
void ScreenBrightnessProPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "screen_brightness_pro",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<ScreenBrightnessProPlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

ScreenBrightnessProPlugin::ScreenBrightnessProPlugin() {}

ScreenBrightnessProPlugin::~ScreenBrightnessProPlugin() {}

double GetWindowsBrightness() {
  HRESULT hr;
  hr = CoInitializeEx(0, COINIT_MULTITHREADED);

  IWbemLocator *pLoc = NULL;
  hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                        IID_IWbemLocator, (LPVOID *)&pLoc);
  if (FAILED(hr))
    return 0.5;

  IWbemServices *pSvc = NULL;
  hr = pLoc->ConnectServer(BstrHolder(L"ROOT\\WMI"), NULL, NULL, 0, NULL, 0, 0,
                           &pSvc);
  if (FAILED(hr)) {
    pLoc->Release();
    return 0.5;
  }

  IEnumWbemClassObject *pEnumerator = NULL;
  hr = pSvc->ExecQuery(BstrHolder("WQL"),
                       BstrHolder("SELECT * FROM WmiMonitorBrightness"),
                       WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                       NULL, &pEnumerator);

  double brightness = 0.5;
  if (pEnumerator) {
    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (uReturn != 0) {
      VARIANT vtProp;
      hr = pclsObj->Get(L"CurrentBrightness", 0, &vtProp, 0, 0);
      if (SUCCEEDED(hr)) {
        brightness = vtProp.uintVal / 100.0;
      }
      VariantClear(&vtProp);
      pclsObj->Release();
    }
    pEnumerator->Release();
  }

  pSvc->Release();
  pLoc->Release();
  CoUninitialize();
  return brightness;
}

void SetWindowsBrightness(double brightness) {
  HRESULT hr;
  hr = CoInitializeEx(0, COINIT_MULTITHREADED);

  IWbemLocator *pLoc = NULL;
  hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                        IID_IWbemLocator, (LPVOID *)&pLoc);
  if (FAILED(hr))
    return;

  IWbemServices *pSvc = NULL;
  hr = pLoc->ConnectServer(BstrHolder(L"ROOT\\WMI"), NULL, NULL, 0, NULL, 0, 0,
                           &pSvc);
  if (FAILED(hr)) {
    pLoc->Release();
    return;
  }

  IEnumWbemClassObject *pEnumerator = NULL;
  hr = pSvc->ExecQuery(BstrHolder("WQL"),
                       BstrHolder("SELECT * FROM WmiMonitorBrightnessMethods"),
                       WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                       NULL, &pEnumerator);

  if (pEnumerator) {
    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (uReturn != 0) {
      VARIANT vtPath;
      pclsObj->Get(L"__PATH", 0, &vtPath, NULL, NULL);

      IWbemClassObject *pInParamsDefinition = NULL;
      hr = pSvc->GetObject(BstrHolder("WmiMonitorBrightnessMethods"), 0, NULL,
                           &pInParamsDefinition, NULL);

      IWbemClassObject *pClassInstance = NULL;
      pInParamsDefinition->SpawnInstance(0, &pClassInstance);

      VARIANT var;
      var.vt = VT_UI1;
      var.bVal = (BYTE)(brightness * 100);
      pClassInstance->Put(L"Brightness", 0, &var, 0);

      VARIANT varTimeout;
      varTimeout.vt = VT_I4;
      varTimeout.lVal = 0;
      pClassInstance->Put(L"Timeout", 0, &varTimeout, 0);

      pSvc->ExecMethod(vtPath.bstrVal, BstrHolder("WmiSetBrightness"), 0, NULL,
                       pClassInstance, NULL, NULL);

      pClassInstance->Release();
      pInParamsDefinition->Release();
      VariantClear(&vtPath);
      pclsObj->Release();
    }
    pEnumerator->Release();
  }

  pSvc->Release();
  pLoc->Release();
  CoUninitialize();
}

void ScreenBrightnessProPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare("getBrightness") == 0) {
    result->Success(flutter::EncodableValue(GetWindowsBrightness()));
  } else if (method_call.method_name().compare("setBrightness") == 0) {
    const auto *brightness = std::get_if<double>(method_call.arguments());
    if (brightness) {
      SetWindowsBrightness(*brightness);
      result->Success();
    } else {
      result->Error("INVALID_ARGUMENT", "Brightness must be a double");
    }
  } else if (method_call.method_name().compare("isAutoModeEnabled") == 0) {
    result->Success(flutter::EncodableValue(false));
  } else if (method_call.method_name().compare("setAutoMode") == 0) {
    result->Success();
  } else if (method_call.method_name().compare("hasWriteSettingsPermission") ==
             0) {
    result->Success(flutter::EncodableValue(true));
  } else if (method_call.method_name().compare(
                 "requestWriteSettingsPermission") == 0) {
    result->Success();
  } else if (method_call.method_name().compare("resetBrightness") == 0) {
    result->Success();
  } else if (method_call.method_name().compare("setSystemBrightness") == 0) {
    const auto *brightness = std::get_if<double>(method_call.arguments());
    if (brightness) {
      SetWindowsBrightness(*brightness);
      result->Success();
    } else {
      result->Error("INVALID_ARGUMENT", "Brightness must be a double");
    }
  } else if (method_call.method_name().compare("setKeepScreenOn") == 0) {
    const auto *enabled = std::get_if<bool>(method_call.arguments());
    if (enabled) {
      if (*enabled) {
        SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
      } else {
        SetThreadExecutionState(ES_CONTINUOUS);
      }
      result->Success();
    } else {
      result->Error("INVALID_ARGUMENT", "Enabled must be a boolean");
    }
  } else if (method_call.method_name().compare("getBatteryLevel") == 0) {
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status) && status.BatteryLifePercent != 255) {
      result->Success(flutter::EncodableValue(static_cast<double>(status.BatteryLifePercent) / 100.0));
    } else {
      result->Success(flutter::EncodableValue(-1.0));
    }
  } else if (method_call.method_name().compare("isLowPowerModeEnabled") == 0) {
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
      result->Success(flutter::EncodableValue(status.SystemStatusFlag == 1));
    } else {
      result->Success(flutter::EncodableValue(false));
    }
  } else {
    result->NotImplemented();
  }
}

} // namespace screen_brightness_pro
