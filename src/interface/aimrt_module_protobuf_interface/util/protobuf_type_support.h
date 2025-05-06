// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "aimrt_module_c_interface/util/type_support_base.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "aimrt_module_protobuf_interface/util/protobuf_zero_copy_stream.h"

#if GOOGLE_PROTOBUF_VERSION >= 3022000
  #include <google/protobuf/json/json.h>
#else
  #include <google/protobuf/stubs/stringpiece.h>
#endif
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

namespace aimrt {

  inline bool HasFile(const google::protobuf::FileDescriptorSet& fset_, const std::string& fname_) {
    for (auto findex = 0; findex < fset_.file_size(); ++findex) {
      if (fset_.file(findex).name() == fname_) {
        return (true);
      }
    }
    return (false);
  }
  
  inline bool GetFileDescriptor(const google::protobuf::Descriptor* desc_, google::protobuf::FileDescriptorSet& fset_) {
    if (desc_ == nullptr) return (false);
    const google::protobuf::FileDescriptor* fdesc = desc_->file();
  
    for (auto dep = 0; dep < fdesc->dependency_count(); ++dep) {
      // iterate containing messages
      const google::protobuf::FileDescriptor* sfdesc = fdesc->dependency(dep);
      for (auto mtype = 0; mtype < sfdesc->message_type_count(); ++mtype) {
        const google::protobuf::Descriptor* desc = sfdesc->message_type(mtype);
        GetFileDescriptor(desc, fset_);
      }
  
      // containing enums ?
      if (sfdesc->enum_type_count() > 0) {
        const google::protobuf::EnumDescriptor* edesc = sfdesc->enum_type(0);
        const google::protobuf::FileDescriptor* efdesc = edesc->file();
  
        if (!HasFile(fset_, efdesc->name())) {
          google::protobuf::FileDescriptorProto* epdesc = fset_.add_file();
          efdesc->CopyTo(epdesc);
        }
      }
  
      // containing services ?
      if (sfdesc->service_count() > 0) {
        const google::protobuf::ServiceDescriptor* svdesc = sfdesc->service(0);
        const google::protobuf::FileDescriptor* svfdesc = svdesc->file();
  
        if (!HasFile(fset_, svfdesc->name())) {
          google::protobuf::FileDescriptorProto* svpdesc = fset_.add_file();
          svfdesc->CopyTo(svpdesc);
        }
      }
    }
  
    if (HasFile(fset_, fdesc->name())) return (true);
  
    google::protobuf::FileDescriptorProto* pdesc = fset_.add_file();
    fdesc->CopyTo(pdesc);
    for (auto field = 0; field < desc_->field_count(); ++field) {
      const google::protobuf::FieldDescriptor* fddesc = desc_->field(field);
      const google::protobuf::Descriptor* desc = fddesc->message_type();
      GetFileDescriptor(desc, fset_);
    }
  
    return (true);
  }
  
  inline std::string GetProtoMessageDescription(const google::protobuf::Message& msg_) {
    const google::protobuf::Descriptor* desc = msg_.GetDescriptor();
    google::protobuf::FileDescriptorSet pset;
    if (GetFileDescriptor(desc, pset)) {
      std::string desc_s = pset.SerializeAsString();
      return (desc_s);
    }
    return ("");
  }

template <std::derived_from<::google::protobuf::Message> MsgType>
const aimrt_type_support_base_t* GetProtobufMessageTypeSupport() {
  static const aimrt_string_view_t kChannelProtobufSerializationTypesSupportedList[] = {
      aimrt::util::ToAimRTStringView("pb"),
      aimrt::util::ToAimRTStringView("json")};
  static MsgType msg{};
  static const std::string kMsgTypeName = "pb:" + MsgType().GetTypeName();
  static const std::string kMsgDescriptor = GetProtoMessageDescription(msg);

  static const aimrt_type_support_base_t kTs{
      .type_name = [](void* impl) -> aimrt_string_view_t {
        return aimrt::util::ToAimRTStringView(kMsgTypeName);
      },
      .description = [](void* impl) -> aimrt_string_view_t {
        return aimrt::util::ToAimRTStringView(kMsgDescriptor);
      },
      .create = [](void* impl) -> void* {
        return new MsgType();
      },
      .destroy = [](void* impl, void* msg) {
        delete static_cast<MsgType*>(msg);  //
      },
      .copy = [](void* impl, const void* from, void* to) {
        *static_cast<MsgType*>(to) = *static_cast<const MsgType*>(from);  //
      },
      .move = [](void* impl, void* from, void* to) {
        *static_cast<MsgType*>(to) = std::move(*static_cast<MsgType*>(from));  //
      },
      .serialize = [](void* impl, aimrt_string_view_t serialization_type, const void* msg, const aimrt_buffer_array_allocator_t* allocator, aimrt_buffer_array_t* buffer_array) -> bool {
        try {
          const MsgType& msg_ref = *static_cast<const MsgType*>(msg);
          if (aimrt::util::ToStdStringView(serialization_type) == "pb") {
            BufferArrayZeroCopyOutputStream os(buffer_array, allocator);
            if (!msg_ref.SerializeToZeroCopyStream(&os)) return false;
            os.CommitLastBuf();
            return true;
          }

          if (aimrt::util::ToStdStringView(serialization_type) == "json") {
            // todo: Use zerocopy
            ::google::protobuf::util::JsonPrintOptions op;
#if GOOGLE_PROTOBUF_VERSION >= 5026000
            op.always_print_fields_with_no_presence = true;
#else
            op.always_print_primitive_fields = true;
#endif
            op.preserve_proto_field_names = true;
            std::string str;
            auto status = ::google::protobuf::util::MessageToJsonString(msg_ref, &str, op);
            if (!status.ok()) return false;

            auto buffer = allocator->allocate(allocator->impl, buffer_array, str.size());
            if (buffer.data == nullptr || buffer.len < str.size()) return false;
            memcpy(buffer.data, str.c_str(), str.size());
            return true;
          }

        } catch (const std::exception& e) {
        }
        return false;
      },
      .deserialize = [](void* impl, aimrt_string_view_t serialization_type, aimrt_buffer_array_view_t buffer_array_view, void* msg) -> bool {
        try {
          if (aimrt::util::ToStdStringView(serialization_type) == "pb") {
            BufferArrayZeroCopyInputStream is(buffer_array_view);
            if (!static_cast<MsgType*>(msg)->ParseFromZeroCopyStream(&is))
              return false;
            return true;
          }

          if (aimrt::util::ToStdStringView(serialization_type) == "json") {
            // todo: Use zerocopy
            if (buffer_array_view.len == 1) {
#if GOOGLE_PROTOBUF_VERSION >= 3022000
              auto status = ::google::protobuf::json::JsonStringToMessage(
                  absl::string_view(
                      static_cast<const char*>(buffer_array_view.data[0].data),
                      buffer_array_view.data[0].len),
                  static_cast<MsgType*>(msg),
                  ::google::protobuf::util::JsonParseOptions());
#else
              auto status = ::google::protobuf::util::JsonStringToMessage(
                  ::google::protobuf::StringPiece(
                      static_cast<const char*>(buffer_array_view.data[0].data),
                      buffer_array_view.data[0].len),
                  static_cast<MsgType*>(msg),
                  ::google::protobuf::util::JsonParseOptions());
#endif
              return status.ok();
            }

            if (buffer_array_view.len > 1) {
              size_t total_size = 0;
              for (size_t ii = 0; ii < buffer_array_view.len; ++ii) {
                total_size += buffer_array_view.data[ii].len;
              }
              std::vector<char> buffer_vec(total_size);
              char* buffer = buffer_vec.data();
              size_t cur_size = 0;
              for (size_t ii = 0; ii < buffer_array_view.len; ++ii) {
                memcpy(buffer + cur_size, buffer_array_view.data[ii].data,
                       buffer_array_view.data[ii].len);
                cur_size += buffer_array_view.data[ii].len;
              }
#if GOOGLE_PROTOBUF_VERSION >= 5022000
              auto status = ::google::protobuf::util::JsonStringToMessage(
                  absl::string_view(
                      static_cast<const char*>(buffer), total_size),
                  static_cast<MsgType*>(msg),
                  ::google::protobuf::util::JsonParseOptions());
#else
              auto status = ::google::protobuf::util::JsonStringToMessage(
                  ::google::protobuf::StringPiece(
                      static_cast<const char*>(buffer), total_size),
                  static_cast<MsgType*>(msg),
                  ::google::protobuf::util::JsonParseOptions());
#endif
              return status.ok();
            }
            return false;
          }
        } catch (const std::exception& e) {
        }
        return false;
      },
      .serialization_types_supported_num = [](void* impl) -> size_t {
        return sizeof(kChannelProtobufSerializationTypesSupportedList) /
               sizeof(kChannelProtobufSerializationTypesSupportedList[0]);
      },
      .serialization_types_supported_list = [](void* impl) -> const aimrt_string_view_t* {
        return kChannelProtobufSerializationTypesSupportedList;
      },
      .custom_type_support_ptr = [](void* impl) -> const void* {
        return MsgType::GetDescriptor();
      },
      .impl = nullptr};
  return &kTs;
}

}  // namespace aimrt
