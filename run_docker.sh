#!/bin/bash
set -e

echo "================================================================="
echo " AgentJobEngine -- Docker Container Build and Execution Script"
echo "================================================================="
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

IMAGE_NAME="agent-job-engine:latest"

echo "[1/2] Building Docker Image ($IMAGE_NAME)..."
docker build -t "$IMAGE_NAME" .

echo ""
echo "[2/2] Running AgentJobEngine Test & Benchmark Suite in Container..."
echo "-----------------------------------------------------------------"
docker run --rm "$IMAGE_NAME"

echo ""
echo "================================================================="
echo " DOCKER CONTAINER EXECUTION COMPLETED SUCCESSFULLY!"
echo "================================================================="
