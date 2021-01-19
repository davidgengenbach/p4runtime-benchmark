#!/usr/bin/env bash

set -eux

function cleanup() {
  sudo pkill --full tofino-model || true
}

trap cleanup EXIT

main() {
  mkdir -p logs
  (cleanup)

  echo "Logs can be found under ./logs"

  (start_tofino_model) > logs/tofino_model.log 2>&1 &
  (start_stratum) > logs/stratum_bfrt.log 2>&1
}

start_tofino_model() {
  echo "Starting tofino-model"
  sudo $(which tofino-model) --p4-target-config=/etc/stratum/tofino_skip_p4.conf
}

start_stratum() {
  echo "Starting Stratum"
  # TODO: remove hardcoded path
  STRATUM_DIR="/home/dgengenbach/Projects/stratum"

  export DOCKER_IMAGE="stratumproject/stratum-bfrt"
  export DOCKER_IMAGE_TAG="9.3.0"
  export PLATFORM="barefoot-tofino-model"

  sudo -E $STRATUM_DIR/stratum/hal/bin/barefoot/docker/start-stratum-container.sh \
    -bf_sim \
    -bf_switchd_background=false \
    -enable_onlp=false \
    -v=6
}

main