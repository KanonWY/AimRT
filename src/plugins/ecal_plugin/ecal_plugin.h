// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <ecal_plugin/ecal_manager.h>
#include <memory>
#include <thread>

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"

namespace aimrt::plugins::ecal_plugin {

class EcalPlugin : public AimRTCorePluginBase {
 public:
  struct Options {
    std::string node_name;
    // SingleThreaded | StaticSingleThreaded | MultiThreaded
    std::string executor_type = "MultiThreaded";
    uint32_t executor_thread_num = 2;
    bool auto_initialize_logging = true;
  };

 public:
  EcalPlugin() = default;
  ~EcalPlugin() override = default;

  std::string_view Name() const noexcept override { return "ecal_plugin"; }

  bool Initialize(runtime::core::AimRTCore* core_ptr) noexcept override;
  void Shutdown() noexcept override;

 private:
  void SetPluginLogger();
  void RegisterEcalRpcBackend();
  void RegisterEcalChannelBackend();

 private:
  runtime::core::AimRTCore* core_ptr_ = nullptr;
  Options options_;
  bool init_flag_ = false;

  std::shared_ptr<EcalManager> ecal_manager_ptr_ = std::make_shared<EcalManager>();
};

}  // namespace aimrt::plugins::ecal_plugin
