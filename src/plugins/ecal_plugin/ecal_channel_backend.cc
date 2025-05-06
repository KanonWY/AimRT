#include "ecal_plugin/ecal_channel_backend.h"
#include <ecal/ecal_callback.h>
#include <exception>
#include <memory>
#include <vector>
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "channel/channel_context.h"
#include "channel/channel_context_base.h"
#include "ecal_plugin/ecal_manager.h"
#include "ecal_plugin/global.h"
#include "log_util.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::ecal_plugin::EcalChannelBackend::Options> {
  using Options = aimrt::plugins::ecal_plugin::EcalChannelBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;
    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::ecal_plugin {

void EcalChannelBackend::Initialize(YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::kInit) == State::kPreInit,
      "eCAL channel backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  options_node = options_;
}
void EcalChannelBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::kStart) == State::kInit,
      "Method can only be called when state is 'Init'.");
}
void EcalChannelBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::kShutdown) == State::kShutdown)
    return;
}
bool EcalChannelBackend::RegisterPublishType(const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept {
  try {
    AIMRT_CHECK_ERROR_THROW(state_.load() == State::kInit,
                            "Method can only be called when state is 'Init'.");
    const auto& info = publish_type_wrapper.info;
    auto ref = info.msg_type_support_ref;
    auto desc = ref.Description();
    ecal_manager_ptr_->RegisterPublisher(info.topic_name, filter_raw_pb_msg_type(info.msg_type), std::string(desc.data()));
    return true;
  } catch (const std::exception& e) {
    AIMRT_ERROR("{}", e.what());
    return false;
  }
}

bool EcalChannelBackend::Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept {
  try {
    AIMRT_CHECK_ERROR_THROW(state_.load() == State::kInit,
                            "Method can only be called when state is 'Init'.");

    const auto& info = subscribe_wrapper.info;
    namespace util = aimrt::common::util;

    auto find_itr = subscribe_wrapper_map_.find(info.topic_name);
    if (find_itr != subscribe_wrapper_map_.end()) {
      find_itr->second->AddSubscribeWrapper(&subscribe_wrapper);
      return true;
    }

    auto sub_tool_unique_ptr = std::make_unique<aimrt::runtime::core::channel::SubscribeTool>();
    sub_tool_unique_ptr->AddSubscribeWrapper(&subscribe_wrapper);
    auto* sub_tool_ptr = sub_tool_unique_ptr.get();
    subscribe_wrapper_map_.emplace(info.topic_name, std::move(sub_tool_unique_ptr));

    // TODO: dealwith msg
    auto handle = [this, topic_name = info.topic_name, sub_tool_ptr](const char*, const struct eCAL::SReceiveCallbackData* data) {
      auto ctx_ptr = std::make_shared<aimrt::channel::Context>(aimrt_channel_context_type_t::AIMRT_CHANNEL_SUBSCRIBER_CONTEXT);
      std::vector<char> serialized_data(static_cast<char*>(data->buf), static_cast<char*>(data->buf) + data->size);
      ctx_ptr->SetSerializationType("pb");
      sub_tool_ptr->DoSubscribeCallback(ctx_ptr, "pb", data->buf, data->size);
    };
    ecal_manager_ptr_->RegisterSubscriber(info.topic_name, handle);

    AIMRT_INFO("Register subscribe type to eCAL channel, url: {}", info.topic_name);

    return true;
  } catch (const std::exception& e) {
    AIMRT_ERROR("{}", e.what());
    return false;
  }
}

std::string EcalChannelBackend::filter_raw_pb_msg_type(const std::string& msg_type) {
  if (msg_type.empty()) {
    return "";
  }
  if (msg_type.size() <= 3) [[unlikely]] {
    AIMRT_ERROR("EcalChannelBackend find pb msg less 3: {}", msg_type);
    return "";
  }
  return msg_type.substr(3);
}

// MsgWrapper how passed
void EcalChannelBackend::Publish(runtime::core::channel::MsgWrapper& msg_wrapper) noexcept {
  AIMRT_CHECK_ERROR(state_.load() == State::kStart,
                    "Method can only be called when state is 'Start'.");
  try {
    namespace util = aimrt::common::util;
    const auto& info = msg_wrapper.info;
    std::string ecal_pub_topic = info.topic_name;

    const auto ecal_pub_register_ptr = ecal_manager_ptr_->GetPublisherRegisterMap();
    auto ecal_pub_ptr = ecal_pub_register_ptr.find(ecal_pub_topic);
    if (ecal_pub_ptr == ecal_pub_register_ptr.end()) [[unlikely]] {
      AIMRT_ERROR("eCAL topic {} not register", ecal_pub_topic);
      return;
    }

    // TODO: aimrt  raw support eCAL protobuf
    // auto publish_type_support_ref = info.msg_type_support_ref;
    // auto serialization_type = msg_wrapper.ctx_ref.GetSerializationType();
    // if (serialization_type.empty()) {
    //   serialization_type = publish_type_support_ref.DefaultSerializationType();
    // }
    // auto [meta_key_vals_array, meta_key_vals_array_len] = msg_wrapper.ctx_ref.GetMetaKeyValsArray();
    // AIMRT_CHECK_ERROR_THROW(meta_key_vals_array_len / 2 <= 255,
    //                         "Too much context meta, require less than 255, but actually {}.", meta_key_vals_array_len / 2);
    // auto buffer_array_view_ptr = aimrt::runtime::core::channel::SerializeMsgWithCache(msg_wrapper, serialization_type);
    // ecal_pub_ptr->second->Send(buffer_array_view_ptr.get()->Data(), buffer_array_view_ptr->Size());

    auto msg_ptr = const_cast<void*>(msg_wrapper.msg_ptr);
    EcalChannelBackend::CPayload payload(*reinterpret_cast<google::protobuf::Message*>(msg_ptr));
    ecal_pub_ptr->second->Send(payload);

  } catch (const std::exception& e) {
    AIMRT_ERROR("{}", e.what());
  }
}
}  // namespace aimrt::plugins::ecal_plugin
