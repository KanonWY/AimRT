// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include "core/channel/channel_backend_base.h"
#include "core/channel/channel_backend_tools.h"
#include "ecal_plugin/ecal_manager.h"

#include <google/protobuf/descriptor.pb.h>
#include <memory>
#include <unordered_map>

namespace aimrt::plugins::ecal_plugin {
class EcalChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  struct Options {
  };

  class CPayload : public eCAL::CPayloadWriter {
   public:
    CPayload(const google::protobuf::Message& message_) : message(message_){};

    ~CPayload() override = default;

    CPayload(const CPayload&) = default;
    CPayload(CPayload&&) noexcept = default;

    CPayload& operator=(const CPayload&) = delete;
    CPayload& operator=(CPayload&&) noexcept = delete;

    bool WriteFull(void* buf_, size_t len_) override {
      return message.SerializeToArray(buf_, static_cast<int>(len_));
    }

    size_t GetSize() override {
      size_t size(0);
#if GOOGLE_PROTOBUF_VERSION >= 3001000
      size = static_cast<size_t>(message.ByteSizeLong());
#else
      size = static_cast<size_t>(message.ByteSize());
#endif
      return (size);
    };

   private:
    const google::protobuf::Message& message;
  };

 public:
  EcalChannelBackend(const std::shared_ptr<EcalManager>& ecal_util_ptr)
      : ecal_manager_ptr_(ecal_util_ptr) {}

  ~EcalChannelBackend() override = default;

  std::string_view Name() const noexcept override { return "ecal"; }

  void Initialize(YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  void SetChannelRegistry(const runtime::core::channel::ChannelRegistry* channel_registry_ptr) noexcept override {
    channel_registry_ptr_ = channel_registry_ptr;
  }
  bool RegisterPublishType(const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept override;

  bool Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept override;

  void Publish(runtime::core::channel::MsgWrapper& msg_wrapper) noexcept override;

 private:
  /**
   * @brief just get the raw pb msg_type
   * @param [in] msg_type msg_type_string
   */
  std::string filter_raw_pb_msg_type(const std::string& msg_type);

  enum class State : uint32_t {
    kPreInit,
    kInit,
    kStart,
    kShutdown,
  };

  Options options_;
  std::atomic<State> state_{State::kPreInit};
  const runtime::core::channel::ChannelRegistry* channel_registry_ptr_ = nullptr;
  std::shared_ptr<EcalManager> ecal_manager_ptr_;

  std::unordered_map<std::string,
                     std::unique_ptr<aimrt::runtime::core::channel::SubscribeTool>>
      subscribe_wrapper_map_;
};

}  // namespace aimrt::plugins::ecal_plugin
