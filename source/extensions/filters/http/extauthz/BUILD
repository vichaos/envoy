licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "extauth_lib",
    srcs = ["extauthz.cc"],
    hdrs = ["extauthz.h"],
    deps = [
        "//include/envoy/buffer:buffer_interface",
        "//include/envoy/http:filter_interface",
        "//include/envoy/upstream:cluster_manager_interface",
        "//source/common/common:assert_lib",
        "//source/common/common:enum_to_int",
        "//source/common/common:logger_lib",
        "//source/common/http:message_lib",
        "//source/common/http:utility_lib",
    ],
)

envoy_cc_library(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        ":extauth_lib",
        "//include/envoy/registry",
        "//include/envoy/server:filter_config_interface",
        "//source/common/json:config_schemas_lib",
    ],
)

