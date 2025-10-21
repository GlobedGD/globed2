#pragma once

namespace globed {

/// Simple helper struct for multiplying volume layers
struct VolumeLayer {
    float volume = 1.f;

    VolumeLayer() = default;
    VolumeLayer(float volume) : volume(volume) {}

    VolumeLayer chain(const VolumeLayer& other) const {
        return (*this) * other;
    }

    float apply(float input) const {
        return input * this->volume;
    }

    void set(float volume) {
        this->volume = volume;
    }

    VolumeLayer& operator=(float volume) {
        this->set(volume);
        return *this;
    }

    VolumeLayer operator*(const VolumeLayer& other) const {
        return VolumeLayer{this->volume * other.volume};
    }

    VolumeLayer& operator*=(const VolumeLayer& other) {
        this->volume *= other.volume;
        return *this;
    }

    float operator()(float input) const {
        return this->apply(input);
    }
};

}