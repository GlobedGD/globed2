if [ -d "samplerate-git" ]; then
    :
else
    git clone https://github.com/libsndfile/libsamplerate "samplerate-git"
fi

cd samplerate-git

echo "Pulling the repo for updates.."
git pull

echo "Running autogen.."

./autogen.sh

echo "Running configure.."

export CC="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi28-clang"
export CXX="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi28-clang++"
export AR="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar"
export AS="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-as"
export LD="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/lld"
export RANLIB="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ranlib"

./configure --host=armv7a-linux-androideabi28 --disable-shared --enable-static --disable-sndfile

if [ $? -ne 0 ]; then
    echo "Configure failed."
    exit 0
fi

echo "Running make.."

make clean
make

if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 0
fi

export OUT_DIR_ARMV7=../libs/libsamplerate/android-armv7/

if [ -d "$OUT_DIR_ARMV7" ]; then
    :
else
    mkdir "$OUT_DIR_ARMV7"
fi

cp -f src/.libs/libsamplerate.a $OUT_DIR_ARMV7

if [ $? -ne 0 ]; then
    echo "Copying libsamplerate binary failed."
    exit 0
fi
