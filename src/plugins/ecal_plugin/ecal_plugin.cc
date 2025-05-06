#include "ecal_plugin/ecal_plugin.h"
#include <ecal/ecal.h>
#include <memory>
#include "channel/channel_backend_base.h"
#include "core/aimrt_core.h"
#include "ecal_plugin/ecal_channel_backend.h"
#include "ecal_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::ecal_plugin::EcalPlugin::Options> {
  using Options = aimrt::plugins::ecal_plugin::EcalPlugin::Options;

  static Node encode(const Options& rhs) {
    Node node;
    node["node_name"] = rhs.node_name;
    node["executor_type"] = rhs.executor_type;
    if (rhs.executor_type == "MultiThreaded") {
      node["executor_thread_num"] = rhs.executor_thread_num;
    }
    node["auto_initialize_logging"] = rhs.auto_initialize_logging;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    rhs.node_name = node["node_name"].as<std::string>();

    if (node["executor_type"])
      rhs.executor_type = node["executor_type"].as<std::string>();

    if (rhs.executor_type == "MultiThreaded" && node["executor_thread_num"])
      rhs.executor_thread_num = node["executor_thread_num"].as<uint32_t>();

    if (node["auto_initialize_logging"]) {
      rhs.auto_initialize_logging = node["auto_initialize_logging"].as<bool>();
    }

    return true;
  }
};
}  // namespace YAML

bool aimrt::plugins::ecal_plugin::EcalPlugin::Initialize(runtime::core::AimRTCore* core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;
    YAML::Node plugin_options_node = core_ptr_->GetPluginManager().GetPluginOptionsNode(Name());

    if (plugin_options_node && !plugin_options_node.IsNull()) {
      options_ = plugin_options_node.as<Options>();
    }

    init_flag_ = true;
    if (not eCAL::IsInitialized()) {
      // TODO: add component choose and module_name in monitor
      eCAL::Initialize(0, nullptr);
    }
    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::kPreInitChannel, [this] {
      RegisterEcalChannelBackend();
    });
    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::kPostInitRpc, [this] {
      RegisterEcalRpcBackend();
    });
    return true;
  } catch (const std::exception& e) {
    AIMRT_ERROR("Initialize failed, {}", e.what());
  }
  return false;
}
void aimrt::plugins::ecal_plugin::EcalPlugin::Shutdown() noexcept {
  try {
    if (not init_flag_) {
      return;
    }
    eCAL::Finalize();
  } catch (std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
  }
}
void aimrt::plugins::ecal_plugin::EcalPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(
      core_ptr_->GetLoggerManager().GetLoggerProxy().NativeHandle()));
}
void aimrt::plugins::ecal_plugin::EcalPlugin::RegisterEcalRpcBackend() {
}
void aimrt::plugins::ecal_plugin::EcalPlugin::RegisterEcalChannelBackend() {
  std::unique_ptr<runtime::core::channel::ChannelBackendBase> ecal_channel_backend_ptr = std::make_unique<EcalChannelBackend>(ecal_manager_ptr_);
  core_ptr_->GetChannelManager().RegisterChannelBackend(std::move(ecal_channel_backend_ptr));
}
