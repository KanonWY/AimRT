
#include "aimrt_core_plugin_interface/aimrt_core_plugin_main.h"
#include "ecal_plugin/ecal_plugin.h"

extern "C" {

aimrt::AimRTCorePluginBase* AimRTDynlibCreateCorePluginHandle() {
  return new aimrt::plugins::ecal_plugin::EcalPlugin();
}

void AimRTDynlibDestroyCorePluginHandle(const aimrt::AimRTCorePluginBase* plugin) {
  delete plugin;
}
}
