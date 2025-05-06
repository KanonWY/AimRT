#include "ecal_plugin/ecal_manager.h"
#include <ecal/ecal_core.h>
#include <ecal/ecal_publisher.h>
#include <ecal/ecal_subscriber.h>
#include <ecal/ecal_types.h>
#include <memory>
#include <mutex>
#include "log_util.h"

namespace aimrt::plugins::ecal_plugin {

void EcalManager::Initialize() {
}

void EcalManager::Shutdown() {
}

void EcalManager::RegisterSubscriber(const std::string& topic_name, MsgHandleFunc handle) {
  std::lock_guard<std::mutex> lock(ecal_registry_mutex_);
  auto sub = std::make_shared<eCAL::CSubscriber>(topic_name);
  sub->AddReceiveCallback(std::move(handle));
  ecal_sub_registry_.insert({topic_name, sub});
}

void EcalManager::RegisterSubscriber(const std::string& topic_name, MsgHandleFunc handle, eCAL::SDataTypeInformation info) {
  std::lock_guard<std::mutex> lock(ecal_registry_mutex_);
  auto sub = std::make_shared<eCAL::CSubscriber>(topic_name, info);
  sub->AddReceiveCallback(std::move(handle));
  ecal_sub_registry_.insert({topic_name, sub});
}

void EcalManager::RegisterPublisher(const std::string& topic_name) {
  std::lock_guard<std::mutex> lock(ecal_registry_mutex_);
  ecal_pub_registry_.emplace(topic_name, std::make_shared<eCAL::CPublisher>(topic_name));
}

void EcalManager::RegisterPublisher(const std::string& topic_name, eCAL::SDataTypeInformation info) {
  std::lock_guard<std::mutex> lock(ecal_registry_mutex_);
  ecal_pub_registry_.emplace(topic_name, std::make_shared<eCAL::CPublisher>(topic_name, std::move(info)));
}
void EcalManager::RegisterPublisher(const std::string& topic_name, std::string msg_type, std::string desc) {
  eCAL::SDataTypeInformation info{
      .name = msg_type,
      .encoding = "proto",
      .descriptor = desc,
  };
  RegisterPublisher(topic_name, info);
}

}  // namespace aimrt::plugins::ecal_plugin
