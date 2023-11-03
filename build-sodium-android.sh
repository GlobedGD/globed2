if [ -d "libsodium-git" ]; then
    :
else
    git clone https://github.com/jedisct1/libsodium "libsodium-git"
fi

cd libsodium-git

echo "Checking out to stable and pulling.."
git checkout stable
git pull

echo "Running build.."

NDK_PLATFORM=28 ./dist-build/android-armv7-a.sh

if [ $? -ne 0 ]; then
    echo "Build for armv7 failed."
    exit 0
fi

NDK_PLATFORM=28 ./dist-build/android-armv8-a.sh

if [ $? -ne 0 ]; then
    echo "Build for armv8 failed."
    exit 0
fi

echo "Libsodium build succeeded! Copying.."

export OUT_DIR_ARMV7=../libs/libsodium/android-armv7/

if [ -d "$OUT_DIR_ARMV7" ]; then
    :
else
    mkdir "$OUT_DIR_ARMV7"
fi

cp -f libsodium-android-armv7-a/lib/libsodium.a $OUT_DIR_ARMV7

if [ $? -ne 0 ]; then
    echo "Copying armv7 binary failed."
    exit 0
fi

export OUT_DIR_ARMV8=../libs/libsodium/android-armv8/

if [ -d "$OUT_DIR_ARMV8" ]; then
    :
else
    mkdir "$OUT_DIR_ARMV8"
fi

cp -f libsodium-android-armv8-a+crypto/lib/libsodium.a $OUT_DIR_ARMV8

if [ $? -ne 0 ]; then
    echo "Copying armv8 binary failed."
    exit 0
fi

echo "We are done :)"