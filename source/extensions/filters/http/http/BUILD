licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "extauth_lib",
    srcs = ["extauth_config.cc"],
    hdrs = ["extauth_config.h"],
    deps = [
        "//source/common/http/filter:extauth_lib",
        "//source/server:configuration_lib",
        "//source/server/config/network:http_connection_manager_lib",
    ],
)