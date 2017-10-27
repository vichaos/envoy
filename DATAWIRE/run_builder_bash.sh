#!/bin/bash

set -e

ROOT=$(cd $(dirname $0)/..; pwd)

cd "$ROOT"

. ci/envoy_build_sha.sh

[[ -z "${IMAGE_ID}" ]] && IMAGE_ID="${ENVOY_BUILD_SHA}"

docker pull lyft/envoy-build:"${IMAGE_ID}"

echo 'Now you can run /xfer/DATAWIRE/go.sh to copy, rebuild, and run Envoy'

docker run --privileged -it -u 0:0 -p 9999:9999 -p 8001:8001 -v "$(pwd):/xfer" --name envoy-build \
    lyft/envoy-build:"${IMAGE_ID}" /bin/bash
