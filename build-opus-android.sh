if [ -d "opus-git" ]; then
    :
else
    git clone https://github.com/xiph/opus "opus-git"
fi

cd opus-git

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

./configure --host=armv7a-linux-androideabi28 --disable-shared --enable-static --disable-extra-programs

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

export OUT_DIR_ARMV7=../libs/opus/android-armv7/

if [ -d "$OUT_DIR_ARMV7" ]; then
    :
else
    mkdir "$OUT_DIR_ARMV7"
fi

cp -f .libs/libopus.a $OUT_DIR_ARMV7

if [ $? -ne 0 ]; then
    echo "Copying libopus binary failed."
    exit 0
fi
