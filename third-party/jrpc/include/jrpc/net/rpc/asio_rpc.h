// Copyright (C) 2023 renjj - All Rights Reserved
#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/common.h>

#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include "rpc_meta.pb.h"

namespace jrpc {

class AsioController : public ::google::protobuf::RpcController {
 public:
  void Reset() override{};

  bool Failed() const override { return false; };
  std::string ErrorText() const override { return ""; };
  void StartCancel() override{};
  void SetFailed(const std::string& /* reason */) override{};
  bool IsCanceled() const override { return false; };
  void NotifyOnCancel(::google::protobuf::Closure* /* callback */) override{};
};

class AsioChannel : public ::google::protobuf::RpcChannel {
 public:
  void Init(const std::string& ip, const int port) {
    io_ = boost::make_shared<boost::asio::io_service>();
    sock_ = boost::make_shared<boost::asio::ip::tcp::socket>(*io_);
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(ip),
                                      port);
    sock_->connect(ep);
  }

  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  ::google::protobuf::RpcController* /* controller */,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::google::protobuf::Closure*) override {
    std::string serial_data = request->SerializeAsString();

    jrpc::RpcMeta rpc_meta;
    rpc_meta.set_service_name(method->service()->name());
    rpc_meta.set_method_name(method->name());
    rpc_meta.set_data_size(serial_data.size());

    std::string serial_str = rpc_meta.SerializeAsString();

    int serial_size = serial_str.size();
    serial_str.insert(0, std::string((const char*)&serial_size, sizeof(int)));
    serial_str += serial_data;

    sock_->send(boost::asio::buffer(serial_data));

    char resp_data_size[sizeof(int)];
    sock_->receive(boost::asio::buffer(resp_data_size));

    int resp_data_len = *(int*)resp_data_size;
    std::vector<char> resp_data(resp_data_len, 0);
    sock_->receive(boost::asio::buffer(resp_data));

    response->ParseFromString(std::string(&resp_data[0], resp_data.size()));
  }

 private:
  boost::shared_ptr<boost::asio::io_service> io_;
  boost::shared_ptr<boost::asio::ip::tcp::socket> sock_;
};

class AsioServer {
 public:
  void add(::google::protobuf::Service* service) {
    ServiceInfo service_info;
    service_info.service = service;
    service_info.sd = service->GetDescriptor();
    for (int i = 0; i < service_info.sd->method_count(); ++i) {
      service_info.mds[service_info.sd->method(i)->name()] =
          service_info.sd->method(i);
    }

    services_[service_info.sd->name()] = service_info;
  }

  void start(const std::string& ip, int port);

 private:
  struct MsgStruct {
    ::google::protobuf::Message* recv_msg;
    ::google::protobuf::Message* resp_msg;
    boost::shared_ptr<boost::asio::ip::tcp::socket> sock;

    MsgStruct() : recv_msg(nullptr), resp_msg(nullptr), sock(nullptr) {}
    MsgStruct(::google::protobuf::Message* recv_msg,
              ::google::protobuf::Message* resp_msg,
              boost::shared_ptr<boost::asio::ip::tcp::socket> sock)
        : recv_msg(recv_msg), resp_msg(resp_msg), sock(sock) {}
    ~MsgStruct() = default;
  };

  struct ServiceInfo {
    ::google::protobuf::Service* service;
    const ::google::protobuf::ServiceDescriptor* sd;
    std::map<std::string, const ::google::protobuf::MethodDescriptor*> mds;

    ServiceInfo() : service(nullptr), sd(nullptr) {}
    ~ServiceInfo() = default;
  };

  void dispatch_msg(const std::string& service_name,
                    const std::string& method_name,
                    const std::string& serialzied_data,
                    boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
  void on_resp_msg_filled(MsgStruct msg_struct);
  void pack_message(const ::google::protobuf::Message* msg,
                    std::string* serialized_data) {
    int serialized_size = msg->ByteSize();
    serialized_data->assign((const char*)&serialized_size,
                            sizeof(serialized_size));
    msg->AppendToString(serialized_data);
  }

  // service_name -> {Service*, ServiceDescriptor*, MethodDescriptor* []}
  std::map<std::string, ServiceInfo> services_;
};

}  // namespace jrpc
