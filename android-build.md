## android building

this is outdated uhh yeah todo and stuff none of this is needed

first it's time to clone openssl:

```sh
cd libs/
git clone https://github.com/openssl/openssl.git
cd openssl
```

then you want to configure it:

```sh
export NDK_HOME=/opt/android-ndk # set path to your ndk if it's different
PATH=$NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin:$NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64:$PATH ./Configure android-arm -D__ANDROID_API__=24
```

then you want to build it, BUT keep in mind there's no need to build the whole thing. all you want is for codegen to run, you won't need the actual built libraries as they are already provided by geode.

```sh
make
```

after you stop seeing lines that look similar to these:

```sh
/usr/bin/perl "-I." -Mconfigdata "util/dofile.pl" "-oMakefile" include/openssl/x509.h.in > include/openssl/x509.h
/usr/bin/perl "-I." -Mconfigdata "util/dofile.pl" "-oMakefile" include/openssl/x509_vfy.h.in > include/openssl/x509_vfy.h
/usr/bin/perl "-I." -Mconfigdata "util/dofile.pl" "-oMakefile" include/openssl/x509v3.h.in > include/openssl/x509v3.h
/usr/bin/perl "-I." -Mconfigdata "util/dofile.pl" "-oMakefile" test/provider_internal_test.cnf.in > test/provider_internal_test.cnf
```

you can Ctrl+C and stop the build. **If `make` fails and shows an error, that is fine! As long as the headers were generated properly, there is no need to worry.**

after that's done you just proceed to build the mod like any other android mod.