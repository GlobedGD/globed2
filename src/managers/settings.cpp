#include "settings.hpp"

#include <asp/misc/traits.hpp>
#include <asp/fs.hpp>
#include <asp/time/Instant.hpp>

#include <util/format.hpp>
#include <util/net.hpp>

using namespace geode::prelude;
using namespace asp::time;
using SaveSlot = GlobedSettings::SaveSlot;

GlobedSettings::LaunchArgs GlobedSettings::_launchArgs = {};

void dumpJsonToFile(const matjson::Value& val, const std::filesystem::path& path) {
    auto res = asp::fs::write(path, val.dump(4));
    if (!res) {
        log::warn("dumpJsonToFile failed to write to file (at {}): {}", path, res.unwrapErr().message());
    }
}

GlobedSettings::GlobedSettings() {
    // necessary cause race conditions n stuff lol
    (void) Mod::get()->loadData();

    this->loadLaunchArguments();

    // determine save slot path
    saveSlotPath = Mod::get()->getSaveDir() / "saveslots";

    if (!asp::fs::exists(saveSlotPath)) {
        auto res = asp::fs::createDirAll(saveSlotPath);
        if (!res) {
            log::error("Failed to create saveslot directory at {}", saveSlotPath);
            log::warn("Error: {}", res.unwrapErr().message());
            log::warn("Will be using root of the save directory for storage");
            saveSlotPath = Mod::get()->getSaveDir();
        }
    }

    if (this->forceResetSettings) {
        this->deleteAllSaveSlotFiles();
        this->forceResetSettings = false;
    }

    auto& container = Mod::get()->getSaveContainer();

    // determine the save slot to use
    auto slot = container.get("settingsv2-save-slot").map([](auto& v) {
        return v.template as<size_t>().unwrapOr(-1);
    }).unwrapOr(-1);

    if (slot == (size_t)-1) {
        // this is user's first time using settings v2, which means it's either first time using the mod,
        // or they have not migrated from settings v1 yet.

        // check if there are any v1 settings present
        bool hasV1 = false;
        for (const auto& [k, v] : container) {
            if (k.starts_with("_gsetting-")) {
                hasV1 = true;
                break;
            }
        }

        if (hasV1) {
            this->migrateFromV1();
            return;
        }

        // if the user has no v1 settings, simply select slot 0 and proceed to load settings (which creates defaults if the file is not present)
        slot = 0;
    }

    this->switchSlot(slot);

    // start worker thread
    workerThread.setStartFunction([] { geode::utils::thread::setName("Settings Worker"); });
    workerThread.setLoopFunction([this](auto& stopToken) {
        // yeah so all it does is write to a file :D
        auto task = this->workerChannel.pop();
        dumpJsonToFile(task.data, task.path);
    });
    workerThread.start();
}

void GlobedSettings::save() {
    this->reflect(TaskType::Save);

    // save json container to file
    this->pushWorkerTask();
}

void GlobedSettings::reload() {
    this->reloadFromContainer(this->readSlotData(this->selectedSlot).unwrapOrDefault());
}

void GlobedSettings::reset() {
    this->reflect(TaskType::Reset);

    // save json container to file
    this->pushWorkerTask();
}

void GlobedSettings::switchSlot(size_t index) {
    this->selectedSlot = index;
    Mod::get()->setSavedValue("settingsv2-save-slot", index);
    this->reload();
}

Result<size_t> GlobedSettings::createSlot() {
    auto id = GEODE_UNWRAP(this->getNextFreeSlot());

    matjson::Value slotData;
    slotData["_saveslot-name"] = fmt::format("Slot {}", id);

    // save to file
    dumpJsonToFile(slotData, this->pathForSlot(id));

    return Ok(id);
}

size_t GlobedSettings::getSelectedSlot() {
    return selectedSlot;
}

const GlobedSettings::LaunchArgs& GlobedSettings::launchArgs() {
    return _launchArgs;
}

std::vector<SaveSlot> GlobedSettings::getSaveSlots() {
    std::vector<SaveSlot> slots;

    auto iterres = asp::fs::iterdir(this->saveSlotPath);

    if (!iterres) {
        log::error("Failed to list saveslot directory: {}", iterres.unwrapErr().message());
        return slots;
    }

    auto iter = std::move(iterres).unwrap();

    for (const auto& entry : iter) {
        if (entry.path().extension() == ".json") {
            std::string filename;

            try {
                filename = entry.path().filename().string();
            } catch (const std::exception& e) {
                continue;
            }

            if (!filename.starts_with("saveslot-")) {
                continue;
            }

            // extract slot id from filename
            auto slotIdStr = std::string_view(filename).substr(9, filename.size() - 14);
            size_t slotId = util::format::parse<size_t>(slotIdStr).value_or(-1);

            if (slotId == -1) {
                continue;
            }

            auto metares = this->readSlotMetadata(entry, slotId);
            if (metares) {
                slots.push_back(std::move(metares).unwrap());
            }
        }
    }

    return slots;
}

void GlobedSettings::migrateFromV1() {
    // clone all values from old save container to a new json value, remove _gsetting- prefix
    auto& container = Mod::get()->getSaveContainer();
    std::vector<std::string> toRemove;

    matjson::Value newContainer;
    // assign name
    newContainer["_saveslot-name"] = "Slot 0";

    for (const auto& [k, v] : container) {
        if (k.starts_with("_gsetting-")) {
            toRemove.push_back(k);
            // migrate the key
            newContainer[k.substr(10)] = v;
        }
    }

    for (const auto& k : toRemove) {
        // TODO: uncomment after mat fixes the bug
        // container.erase(k);
    }

    (void) Mod::get()->saveData();

    // dump everything to save slot 0
    dumpJsonToFile(newContainer, this->pathForSlot(0));

    // reload slot 0
    this->switchSlot(0);
}

std::filesystem::path GlobedSettings::pathForSlot(size_t idx) {
    return this->saveSlotPath / fmt::format("saveslot-{}.json", idx);
}

Result<matjson::Value> GlobedSettings::readSlotData(size_t idx) {
    return this->readSlotData(this->pathForSlot(idx));
}

Result<matjson::Value> GlobedSettings::readSlotData(const std::filesystem::path& path) {
    std::ifstream infile(path);
    if (!infile.is_open()) {
        return Err("Failed to open file");
    }

    auto value = matjson::Value::parse(infile).unwrapOr(matjson::Value::object());

    // if slot name is missing, add a default
    if (!value.contains("_saveslot-name")) {
        value["_saveslot-name"] = fmt::format("Slot {}", this->selectedSlot);
    }

    return Ok(std::move(value));
}

Result<SaveSlot> GlobedSettings::readSlotMetadata(size_t idx) {
    return this->readSlotMetadata(this->pathForSlot(idx), idx);
}

Result<SaveSlot> GlobedSettings::readSlotMetadata(const std::filesystem::path& path, size_t idx) {
    auto slot = GEODE_UNWRAP(this->readSlotData(path));

    // precondition: _saveslot-name must be present, it's safe to assume so since `readSlotData` inserts it if it's missing
    // however, we have no guarantees it's a string :)
    return Ok(SaveSlot {
        .index = idx,
        .name = slot["_saveslot-name"].as<std::string>().unwrapOrElse(
            [idx] { return fmt::format("Slot {}", idx); }
        )
    });
}

Result<size_t> GlobedSettings::getNextFreeSlot() {
    size_t idx = 0;

    while (asp::fs::exists(this->pathForSlot(idx))) {
        idx++;
    }

    return Ok(idx);
}

void GlobedSettings::deleteSlot(size_t index) {
    std::error_code ec;
    auto path = this->saveSlotPath / fmt::format("saveslot-{}.json", index);

    if (asp::fs::exists(path)) {
        auto res = asp::fs::removeFile(path);

        if (!res) {
            log::warn("Failed to delete slot {}, system error: {}", index, res.unwrapErr().message());
        }
    } else {
        log::warn("Failed to delete slot {}, was not found", index);
    }
}

void GlobedSettings::renameSlot(size_t index, std::string_view name) {
    if (index == selectedSlot) {
        settingsContainer["_saveslot-name"] = name;

        // save container to file, synchronously for UI responsiveness
        dumpJsonToFile(settingsContainer, this->pathForSlot(index));

        return;
    }

    // if we are renaming an inactive slot, read the file from disk, change the name attribute, and write it again
    auto datares = this->readSlotData(index);
    if (!datares) {
        log::warn("renameSlot called on a non-existent saveslot index: {}", index);
        return;
    }

    auto data = std::move(datares).unwrap();
    data["_saveslot-name"] = name;
    dumpJsonToFile(data, this->pathForSlot(index));
}

matjson::Value GlobedSettings::getSettingContainer() {
    return settingsContainer;
}

void GlobedSettings::reloadFromContainer(const matjson::Value& container) {
    // reset all settings to their defaults first
    this->reflect(TaskType::Reset);

    this->settingsContainer = container;

    // now reload
    this->reflect(TaskType::Load);
}

void GlobedSettings::deleteAllSaveSlotFiles() {
    auto iterres = asp::fs::iterdir(this->saveSlotPath);

    if (!iterres) {
        log::error("Failed to list saveslot directory: {}", iterres.unwrapErr().message());
        return;
    }

    auto iter = std::move(iterres).unwrap();

    for (const auto& entry : iter) {
        if (entry.path().extension() == ".json") {
            try {
                if (!entry.path().filename().string().starts_with("saveslot-")) {
                    continue;
                }
            } catch (const std::exception& e) {
                continue;
            }

            auto res = asp::fs::removeFile(entry);
            if (!res) {
                log::warn("Failed to delete save slot file: {}", res.unwrapErr().message());
            }
        }
    }
}

void GlobedSettings::pushWorkerTask() {
    workerChannel.push(WorkerTask {
        .data = this->settingsContainer,
        .path = this->pathForSlot(this->selectedSlot)
    });
}

void GlobedSettings::loadLaunchArguments() {
    // load launch arguments with reflection
    using LaunchMd = boost::describe::describe_members<LaunchArgs, boost::describe::mod_public>;

    // iterate through them
    boost::mp11::mp_for_each<LaunchMd>([&, this](auto cd) -> void {
        using FlagType = typename asp::member_ptr_to_underlying<decltype(cd.pointer)>::type;
        auto flagArg = FlagType::Name;

        auto& flag = _launchArgs.*cd.pointer;
        auto isSet = Loader::get()->getLaunchFlag(flagArg);

        if (isSet) {
            log::info("Enabled launch flag: {}", (std::string_view) flagArg);
        }

        flag.set(isSet);
    });

    this->forceResetSettings = _launchArgs.resetSettings;

    // some of those options will do nothing unless the mod is built in debug mode
#ifndef GLOBED_DEBUG
    _launchArgs.fakeData = false;
#endif
}

void GlobedSettings::reflect(GlobedSettings::TaskType tt) {
    using enum GlobedSettings::TaskType;
    using SetMd = boost::describe::describe_members<GlobedSettings, boost::describe::mod_public>;

    // iterate through all categories
    boost::mp11::mp_for_each<SetMd>([&, this](auto cd) -> void {
        using CatType = typename asp::member_ptr_to_underlying<decltype(cd.pointer)>::type;
        auto catName = cd.name;

        auto& category = this->*cd.pointer;
        bool isFlag = std::string_view(catName) == "flags";

        // iterate through all settings in the category
        using CatMd = boost::describe::describe_members<CatType, boost::describe::mod_public>;
        boost::mp11::mp_for_each<CatMd>([&, this](auto setd) -> void {
            using SetTy = typename asp::member_ptr_to_underlying<decltype(setd.pointer)>::type;
            using InnerType = SetTy::Type;
            using InnerStoredType = SetTy::StoredType;
            using SerializedType = SetTy::SerializedType;
            constexpr InnerType Default = SetTy::Default;

            auto setName = setd.name;

            std::string settingKey;
            if (isFlag) {
                settingKey = fmt::format("_gflag-{}", setName);
            } else {
                settingKey = fmt::format("{}{}", catName, setName);
            }

            auto& setting = category.*setd.pointer;

            // now, depending on whether we are saving settings or loading them, do the appropriate thing
            if (!isFlag) {
                switch (tt) {
                    case Save: {
                        if (this->has(settingKey) || setting.get() != Default || isFlag) {
                            this->store(settingKey, static_cast<SerializedType>(setting.get()));
                        }
                    } break;
                    case Load: {
                        if (this->has(settingKey)) {
                            SerializedType value = this->load<SerializedType>(settingKey);
                            setting.set(static_cast<InnerType>(value));
                        }
                    } break;
                    case Reset: [[fallthrough]];
                    case HardReset: {
                        setting.set(Default);
                        this->clear(settingKey);
                    } break;
                }
            } else {
                if (!std::is_same_v<InnerType, bool>) {
                    throw std::runtime_error("non-flag detected in settings flags");
                }

                // flags are saved in saved.json, separate from save slots
                switch (tt) {
                    case Save: {
                        this->storeFlag(settingKey, static_cast<SerializedType>(setting.get()));
                    } break;
                    case Load: {
                        this->loadOptionalFlagInto(settingKey, reinterpret_cast<bool&>(setting.ref())); // reinterpret because this code can be interpreted even for non-flags
                    } break;
                    case Reset:
                        // flags cant be cleared unless hard resetting
                        break;
                    case HardReset: {
                        setting.set(static_cast<InnerType>(0)); // false
                        this->clearFlag(settingKey);
                    } break;
                }
            }
        });
    });
}

bool GlobedSettings::has(std::string_view key) {
    return settingsContainer.contains(key);
}

bool GlobedSettings::hasFlag(std::string_view key) {
    return Mod::get()->getSaveContainer().contains(key);
}

void GlobedSettings::clear(std::string_view key) {
    settingsContainer.erase(key);
}

void GlobedSettings::clearFlag(std::string_view key) {
    Mod::get()->getSaveContainer().erase(key);
}

namespace globed {
    namespace {
    // guard for static destruction
    struct SGuard {
        SGuard() {}
        ~SGuard() {
            destroyed = true;
        }

        volatile bool destroyed = false;
    };
    }

    void _doNetDump(std::string_view str) {
        static SGuard _sguard;
        static auto start = Instant::now();
        static std::ofstream file(Mod::get()->getSaveDir() / "net-dump.log");
        static bool headerWritten = false;
        static asp::Mutex<> mutex;

        if (_sguard.destroyed) {
            return;
        }

        log::debug("[netdump] {}", str);

        // Lock the writing to file part
        auto _guard = mutex.lock();

        if (!file.is_open()) {
            if (!headerWritten) {
                headerWritten = true;
                log::error("Failed to open net dump file for writing at {}", Mod::get()->getSaveDir() / "net-dump.log");
            }

            return;
        }

        if (!headerWritten) {
            headerWritten = true;
            file << fmt::format("Globed netdump, platform: {}", util::net::loginPlatformString()) << std::endl;
        }

        auto timestamp = start.elapsed();

        file << fmt::format("[{:.6f}] [{}] {}", timestamp.seconds<asp::f64>(), utils::thread::getName(), str) << std::endl; // flushing every time is probably ok here
    }
}

// Check that all launch arguments have been specified to the macro

using La = GlobedSettings::LaunchArgs;
static_assert(sizeof(La) == ByteBuffer::calculateStructSize<La>(), "LaunchArgs are incorrectly sized, you probably forgot to add some to the serializable macro!");
