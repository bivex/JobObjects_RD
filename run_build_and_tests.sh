#!/bin/bash
set -e

echo "================================================================="
echo " AgentJobEngine -- 1-Click macOS Automated Build and Test Suite"
echo "================================================================="
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "[1/4] Configuring CMake project..."
cmake -B out/build -DCMAKE_BUILD_TYPE=Debug

echo "[2/4] Building AgentJobEngine static library and test targets..."
cmake --build out/build

echo "[3/4] Executing Integrated PoC Test (AgentJobObject_Test)..."
echo "-----------------------------------------------------------------"
./out/build/bin/AgentJobObject_Test
echo ""

echo "[4/4] Executing Defensive Edge Cases Test Suite (AgentJobEngine_EdgeCases_Test)..."
echo "-----------------------------------------------------------------"
./out/build/bin/AgentJobEngine_EdgeCases_Test
echo ""

echo "================================================================="
echo " ALL TESTS COMPLETED SUCCESSFULLY ON macOS!"
echo "================================================================="
