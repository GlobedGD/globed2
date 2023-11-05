#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <UIBuilder.hpp>

#include <crypto/box.hpp>
#include <data/bytebuffer.hpp>
#include <util/all.hpp>
#include <bit>

#include <audio/opus_codec.hpp>
#include <audio/audio_manager.hpp>
#include <audio/voice_playback_manager.hpp>

using namespace geode::prelude;

$on_mod(Loaded) {
	// if there is a logic error in the crypto code, sodium_misuse() gets called
	sodium_set_misuse_handler([](){
		log::error("sodium_misuse called. we are officially screwed.");
		util::debugging::suicide();
	});
}

class $modify(MenuLayer) {
	void onMoreGames(CCObject*) {
		auto& vm = GlobedAudioManager::get();
		vm.setActiveRecordingDevice(2);
		log::debug("Listening to: {}", vm.getRecordingDevice().name);

		vm.startRecording([&vm](const EncodedAudioFrame& frame){
			ByteBuffer bb;
			bb.writeValue(frame);

			bb.setPosition(0);

			auto decodedFrame = bb.readValueUnique<EncodedAudioFrame>();
			VoicePlaybackManager::get().playFrame(0, *decodedFrame.get());
		});
	}	
};