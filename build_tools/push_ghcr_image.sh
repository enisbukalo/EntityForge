#!/usr/bin/env bash

echo "$GHCR_TOKEN" | docker login ghcr.io -u enisbukalo --password-stdin
IMAGE="ghcr.io/enisbukalo/entityforge:latest"
docker build -t "$IMAGE" .
docker push "$IMAGE"
