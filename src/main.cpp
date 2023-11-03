#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <UIBuilder.hpp>

#include <crypto/box.hpp>
#include <data/bytebuffer.hpp>
#include <util/all.hpp>
#include <bit>

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
		std::string key = "My epic key";
		auto hashed = util::crypto::simpleHash(key);
		log::debug("raw key: {}, encoded: {}", key, util::debugging::hexDumpAddress(hashed.data(), hashed.size()));
		auto totp = util::crypto::simpleTOTP(hashed);

		log::debug("totp: {}", totp);
	}	
};