// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include "core/channel/channel_backend_base.h"
#include "core/channel/channel_backend_tools.h"
#include "net/asio_tcp_cli.h"
#include "net/asio_tcp_svr.h"
#include "net_plugin/msg_handle_registry.h"

namespace aimrt::plugins::net_plugin {

class TcpChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  using TcpMsgHandleRegistry = MsgHandleRegistry<boost::asio::ip::tcp::endpoint>;

  struct Options {
    struct PubTopicOptions {
      std::string topic_name;
      std::vector<std::string> server_url_list;
    };

    std::vector<PubTopicOptions> pub_topics_options;
  };

 public:
  TcpChannelBackend(
      const std::shared_ptr<boost::asio::io_context>& io_ptr,
      const std::shared_ptr<aimrt::common::net::AsioTcpClientPool>& tcp_cli_pool_ptr,
      const std::shared_ptr<aimrt::common::net::AsioTcpServer>& tcp_svr_ptr,
      const std::shared_ptr<TcpMsgHandleRegistry>& msg_handle_registry_ptr)
      : io_ptr_(io_ptr),
        tcp_cli_pool_ptr_(tcp_cli_pool_ptr),
        tcp_svr_ptr_(tcp_svr_ptr),
        msg_handle_registry_ptr_(msg_handle_registry_ptr) {}

  ~TcpChannelBackend() override = default;

  std::string_view Name() const noexcept override { return "tcp"; }

  void Initialize(YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  void SetChannelRegistry(const runtime::core::channel::ChannelRegistry* channel_registry_ptr) noexcept override {
    channel_registry_ptr_ = channel_registry_ptr;
  }

  bool RegisterPublishType(
      const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept override;
  bool Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept override;
  void Publish(runtime::core::channel::MsgWrapper& msg_wrapper) noexcept override;

 private:
  enum class State : uint32_t {
    kPreInit,
    kInit,
    kStart,
    kShutdown,
  };

  Options options_;
  std::atomic<State> state_ = State::kPreInit;

  const runtime::core::channel::ChannelRegistry* channel_registry_ptr_ = nullptr;

  std::shared_ptr<boost::asio::io_context> io_ptr_;
  std::shared_ptr<aimrt::common::net::AsioTcpClientPool> tcp_cli_pool_ptr_;
  std::shared_ptr<aimrt::common::net::AsioTcpServer> tcp_svr_ptr_;
  std::shared_ptr<TcpMsgHandleRegistry> msg_handle_registry_ptr_;

  std::unordered_map<
      std::string,
      std::unique_ptr<aimrt::runtime::core::channel::SubscribeTool>>
      tcp_subscribe_wrapper_map_;

  struct PubCfgInfo {
    std::vector<boost::asio::ip::tcp::endpoint> server_ep_vec;
  };
  std::unordered_map<std::string_view, PubCfgInfo> pub_cfg_info_map_;
};

}  // namespace aimrt::plugins::net_plugin