ENVOY_BUILD_SHA=$(grep lyft/envoy-build .circleci/config.yml | sed -e 's#.*lyft/envoy-build:\(.*\)#\1#' | uniq)

if [ $(wc -l <<< "${ENVOY_BUILD_SHA}") -ne 1 ]; then
    echo ".circleci/config.yml hashes are inconsistent!" >&2
    exit 1
fi

