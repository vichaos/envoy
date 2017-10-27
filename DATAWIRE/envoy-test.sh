#!/bin/sh

HERE=$(cd $(dirname $0); pwd)

ENVOY="${HERE}/../bazel-bin/source/exe/envoy-static"
ENVOY_JSON="${HERE}/envoy-test.json"

"$ENVOY" -c "$ENVOY_JSON" -l debug 2>&1 | egrep --color 'ExtAuth|deferring|deferred|$'
