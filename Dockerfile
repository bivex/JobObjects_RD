# ============================================================================
# AgentJobEngine — Docker Container Setup for Linux OS Resource Controller
# Multi-stage build (Alpine 3.20 C++17 Toolchain)
# ============================================================================

FROM alpine:3.20 AS builder

# Install build dependencies and kernel headers
RUN apk add --no-cache cmake g++ make musl-dev linux-headers bash

WORKDIR /app

# Copy source code and CMake build manifests
COPY CMakeLists.txt ./
COPY include/ ./include/
COPY src/ ./src/
COPY tests/ ./tests/

# Build static/dynamic AgentJobEngine binaries
RUN cmake -B out/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build out/build

# ============================================================================
# Production Runner Stage
# ============================================================================
FROM alpine:3.20 AS runner
RUN apk add --no-cache libstdc++ libgcc bash

WORKDIR /app

# Copy built test and benchmark executables
COPY --from=builder /app/out/build/bin/AgentJobObject_Test ./
COPY --from=builder /app/out/build/bin/AgentJobEngine_EdgeCases_Test ./
COPY --from=builder /app/out/build/bin/AgentSwarm_Benchmark ./

# Default execution runs complete Linux validation & benchmark suite
CMD ["sh", "-c", "./AgentJobObject_Test && ./AgentJobEngine_EdgeCases_Test && ./AgentSwarm_Benchmark"]
