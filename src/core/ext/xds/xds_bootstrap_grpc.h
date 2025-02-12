//
// Copyright 2019 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef GRPC_CORE_EXT_XDS_XDS_BOOTSTRAP_GRPC_H
#define GRPC_CORE_EXT_XDS_XDS_BOOTSTRAP_GRPC_H

#include <grpc/support/port_platform.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

#include "src/core/ext/xds/certificate_provider_store.h"
#include "src/core/ext/xds/xds_bootstrap.h"
#include "src/core/lib/json/json.h"
#include "src/core/lib/json/json_args.h"
#include "src/core/lib/json/json_object_loader.h"

namespace grpc_core {

class GrpcXdsBootstrap : public XdsBootstrap {
 public:
  class GrpcNode : public Node {
   public:
    const std::string& id() const override { return id_; }
    const std::string& cluster() const override { return cluster_; }
    const std::string& locality_region() const override {
      return locality_.region;
    }
    const std::string& locality_zone() const override { return locality_.zone; }
    const std::string& locality_sub_zone() const override {
      return locality_.sub_zone;
    }
    const Json::Object& metadata() const override { return metadata_; }

    static const JsonLoaderInterface* JsonLoader(const JsonArgs&);

   private:
    struct Locality {
      std::string region;
      std::string zone;
      std::string sub_zone;

      static const JsonLoaderInterface* JsonLoader(const JsonArgs&);
    };

    std::string id_;
    std::string cluster_;
    Locality locality_;
    Json::Object metadata_;
  };

  class GrpcXdsServer : public XdsServer {
   public:
    const std::string& server_uri() const override { return server_uri_; }

    bool ShouldUseV3() const override;
    bool IgnoreResourceDeletion() const override;

    bool Equals(const XdsServer& other) const override;

    const std::string& channel_creds_type() const {
      return channel_creds_.type;
    }
    const Json::Object& channel_creds_config() const {
      return channel_creds_.config;
    }

    static const JsonLoaderInterface* JsonLoader(const JsonArgs&);
    void JsonPostLoad(const Json& json, const JsonArgs& args,
                      ErrorList* errors);

    Json ToJson() const;

   private:
    struct ChannelCreds {
      std::string type;
      Json::Object config;

      static const JsonLoaderInterface* JsonLoader(const JsonArgs&);
    };

    std::string server_uri_;
    ChannelCreds channel_creds_;
    std::set<std::string> server_features_;
  };

  class GrpcAuthority : public Authority {
   public:
    const XdsServer* server() const override {
      return servers_.empty() ? nullptr : &servers_[0];
    }

    const std::string& client_listener_resource_name_template() const {
      return client_listener_resource_name_template_;
    }

    static const JsonLoaderInterface* JsonLoader(const JsonArgs&);

   private:
    std::vector<GrpcXdsServer> servers_;
    std::string client_listener_resource_name_template_;
  };

  // Creates bootstrap object from json_string.
  static absl::StatusOr<std::unique_ptr<GrpcXdsBootstrap>> Create(
      absl::string_view json_string);

  static const JsonLoaderInterface* JsonLoader(const JsonArgs&);
  void JsonPostLoad(const Json& json, const JsonArgs& args, ErrorList* errors);

  std::string ToString() const override;

  const XdsServer& server() const override { return servers_[0]; }
  const Node* node() const override {
    return node_.has_value() ? &*node_ : nullptr;
  }
  const Authority* LookupAuthority(const std::string& name) const override;
  const XdsServer* FindXdsServer(const XdsServer& server) const override;

  const std::string& client_default_listener_resource_name_template() const {
    return client_default_listener_resource_name_template_;
  }
  const std::string& server_listener_resource_name_template() const {
    return server_listener_resource_name_template_;
  }
  const CertificateProviderStore::PluginDefinitionMap& certificate_providers()
      const {
    return certificate_providers_;
  }

  // Exposed for testing purposes only.
  const std::map<std::string, GrpcAuthority>& authorities() const {
    return authorities_;
  }

 private:
  std::vector<GrpcXdsServer> servers_;
  absl::optional<GrpcNode> node_;
  std::string client_default_listener_resource_name_template_;
  std::string server_listener_resource_name_template_;
  std::map<std::string, GrpcAuthority> authorities_;
  CertificateProviderStore::PluginDefinitionMap certificate_providers_;
};

}  // namespace grpc_core

#endif  // GRPC_CORE_EXT_XDS_XDS_BOOTSTRAP_GRPC_H
