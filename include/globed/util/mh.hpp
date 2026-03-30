#pragma once

namespace globed::mh {

static const std::string MH_FRAME_EXTRAPOLATION = "DISPLAY/HACK_FRAME_EXTRAPOLATION";

bool isLoaded();
bool isHackEnabled(const std::string& hack);
void setHackEnabled(const std::string& hack, bool enabled);

}