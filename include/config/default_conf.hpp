#pragma once

#include <string>

const std::string DEFAULT_CONFIG = R"(
[global]
# enable/disable lsfg on every game
# enable = true

# specify where Lossless.dll is stored
# dll = "/games/Lossless Scaling/Lossless.dll"

# change the fps multiplier
# multiplier = 2

# change the flow scale (lower = faster)
# flow_scale = 1.0

# toggle performance mode (2x-8x performance increase)
# performance_mode = false

# enable hdr mode (doesn't support scrgb)
# hdr_mode = false

# example entry for a game
# [[game]]
# exe = "Game.exe"
#
# enable = true
# dll = "/games/Lossless Scaling/Lossless.dll"
# multiplier = 2
# flow_scale = 1.0
# performance_mode = false
# hdr_mode = false

[[game]] # configure benchmark
exe = "benchmark"
enable = true

multiplier = 4
performance_mode = false

[[game]] # override GenshinImpact.exe
exe = "GenshinImpact.exe"
enable = true

multiplier = 3

[[game]] # override vkcube
exe = "vkcube"
enable = true

multiplier = 4
performance_mode = true
)";
