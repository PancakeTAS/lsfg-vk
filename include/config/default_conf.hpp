#pragma once

#include <string>

const std::string DEFAULT_CONFIG = R"(version = 1
[global]
# override the location of Lossless Scaling
# dll = "/games/Lossless Scaling"

# [[game]] # example entry
# exe = "Game.exe"
# env = "SteamDeck=0"
#
# multiplier = 3
# flow_scale = 0.7
# performance_mode = true
# hdr_mode = false
#
# experimental_present_mode = fifo
# experimental_fps_limit = 48

[[game]] # default vkcube entry
exe = "vkcube"

multiplier = 4
performance_mode = true

[[game]] # default benchmark entry
exe = "benchmark"

multiplier = 4
performance_mode = false

[[game]] # override Genshin Impact
exe = "Genshin"

multiplier = 3
)";
