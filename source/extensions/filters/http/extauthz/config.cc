#include "extensions/filters/http/extauthz/config.h"

#include "envoy/registry/registry.h"

#include "extensions/filters/http/extauthz/extauthz.h"
#include "common/json/config_schemas.h"

namespace Envoy {
namespace Server {
namespace Configuration {

const std::string EXTAUTH_HTTP_FILTER_SCHEMA(R"EOF(
  {
    "$schema": "http://json-schema.org/schema#",
    "type" : "object",
    "properties" : {
      "cluster" : {"type" : "string"},
      "timeout_ms": {"type" : "integer"},
      "allowed_headers": {
        "type": "array",
        "items": {
            "type": "string"
        },
        "minItems": 1,
        "uniqueItems": true
      },
      "path_prefix": {"type" : "string"}
    },
    "required" : ["cluster", "timeout_ms"],
    "additionalProperties" : false
  }
  )EOF");

Http::FilterFactoryCb ExtAuthConfig::createFilterFactory(const Json::Object& json_config,
                                                       const std::string& stats_prefix,
                                                       FactoryContext& context) {
  json_config.validateSchema(EXTAUTH_HTTP_FILTER_SCHEMA);

  std::string prefix =
      json_config.hasObject("path_prefix") ? json_config.getString("path_prefix") : "";

  Http::ExtAuthConfigConstSharedPtr config(new Http::ExtAuthConfig{
      context.clusterManager(), Http::ExtAuth::generateStats(stats_prefix, context.scope()),
      json_config.getString("cluster"),
      std::chrono::milliseconds(json_config.getInteger("timeout_ms")),
      json_config.getStringArray("allowed_headers", true), prefix});

  return [config](Http::FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addStreamDecoderFilter(std::make_shared<Http::ExtAuth>(config));
  };
}

/**
 * Static registration for the extauth filter. @see RegisterHttpFilterConfigFactory.
 */
static Registry::RegisterFactory<ExtAuthConfig, NamedHttpFilterConfigFactory> register_;

} // namespace Configuration
} // namespace Server
} // namespace Envoy
