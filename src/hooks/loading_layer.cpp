#include "loading_layer.hpp"

void HookedLoadingLayer::loadingFinished() {
    util::ui::maybePreloadAssets();
    LoadingLayer::loadingFinished();
}