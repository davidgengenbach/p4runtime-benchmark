#!/usr/bin/env bash

set -eux

ROOT_DIR=$(cd $(dirname "${BASH_SOURCE[0]}")/.. >/dev/null 2>&1 && pwd)
PROTO_OUT="${ROOT_DIR}/third_party/proto_compiled"

main() {
  cd ${ROOT_DIR}
  (convert)
}

convert() {
  TMP=$(mktemp -d)

  cp -r third_party/googleapis/google ${TMP}
  cp -r third_party/p4runtime/proto/* ${TMP}

  cd "${TMP}"

  rm -rf ${PROTO_OUT} && mkdir -p ${PROTO_OUT}

  protoc \
    --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
    --grpc_out ${PROTO_OUT} \
    --cpp_out ${PROTO_OUT} \
    --proto_path=${TMP} \
    ${TMP}/p4/v1/p4*.proto \
    ${TMP}/p4/config/v1/p4*.proto \
    ${TMP}/google/rpc/*.proto
  rm -rf google
}

main