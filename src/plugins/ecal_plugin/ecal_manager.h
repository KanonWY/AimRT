// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <ecal/ecal.h>
#include <ecal/ecal_callback.h>
#include <ecal/ecal_publisher.h>
#include <ecal/ecal_subscriber.h>
#include <ecal/ecal_types.h>
#include <memory>
#include <unordered_map>
#include "ecal_plugin/global.h"

namespace aimrt::plugins::ecal_plugin {

class EcalManager {
 public:
  using MsgHandleFunc = eCAL::ReceiveCallbackT;
  EcalManager() = default;
  ~EcalManager() = default;
  EcalManager(const EcalManager&) = delete;
  EcalManager& operator=(const EcalManager&) = delete;

  const std::unordered_map<std::string, std::shared_ptr<eCAL::CPublisher>>& GetPublisherRegisterMap() {
    return ecal_pub_registry_;
  }

  void Initialize();

  void Shutdown();

  void RegisterSubscriber(const std::string& topic_name, MsgHandleFunc handle);
  void RegisterSubscriber(const std::string& topic_name, MsgHandleFunc handle, eCAL::SDataTypeInformation info);
  void RegisterPublisher(const std::string& topic_name);
  void RegisterPublisher(const std::string& topic_name, eCAL::SDataTypeInformation info);
  void RegisterPublisher(const std::string& topic_name, std::string msg_type, std::string desc);

 private:
  std::unordered_map<std::string, std::shared_ptr<eCAL::CPublisher>> ecal_pub_registry_;
  std::unordered_map<std::string, std::shared_ptr<eCAL::CSubscriber>> ecal_sub_registry_;
  std::mutex ecal_registry_mutex_;
};

}  // namespace aimrt::plugins::ecal_plugin
