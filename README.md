# NVirsual Piano From Above
A clone of the original [Piano From Above](https://github.com/brian-pantano/PianoFromAbove) made by Brian Pantano.
Now available on android.

This clone is powered by Qishipai's [midi processing library](https://github.com/qishipai/NVirsual) and using SDL3 to render the notes and the keyboard.

> [!WARNING]
> This project is still in early development and any release could be unstable. Consider building and testing it at your own risk!!
> #### No public builds are allowed to be fully shared or redistributed!!!

## Features
- [x] Default soundfonts
- [X] Custom background color support
- [X] Custom themed GUI
- [X] Settings storage to file (.json)
- [X] Voice count settings for bass stream
- [X] Midi and soundfont list providers
- [X] Play / Pause playback
- [X] Seek forward
- [X] Seek backwards
- [X] Live Note speed change
- [x] Live Note color change
- [x] Custom note colors presets through .ccol files
- [X] Improving the audio limiter
- [X] Better UI layout on mobile
- [x] File information support
- [X] Custom note color array support
- [x] Vertical lines on the background
- [x] Background image support
- [X] Setting custom key ranges (Keyboard re-scaling to be exact, both using settings and on MIDI File load)
- [X] Common midi and soundfont reloading (This must not require restarting the app after each midi playing session)
- [x] MIDI File History
- [X] Built-in File Dialog
- [ ] Performance improvements
- [X] Logging to text file (useful for development purposes)
- [X] Custom icon and app name
- [ ] GPU Renderer
- [ ] Video capture

## Build Guide

### Prerequisites
- [xmake](https://xmake.io) (v2.8+)
- C++ compiler with C++17 support (gcc, clang, or MSVC)
- [SDL3](https://github.com/libsdl-org/SDL) development libraries
- [Android NDK](https://developer.android.com/ndk) & SDK (for Android builds)
- Java 17+ (for Android builds)

### Linux Desktop
```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install build-essential xmake libsdl3-dev

# Build & run
xmake build
xmake run
```

### Android
```bash
xmake f -p android -a arm64-v8a --toolchain=ndk \
  --android_sdk=~/Android/Sdk --ndk=~/Android/Sdk/ndk/<version>

# Build APK (Java 17 must be in PATH)
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export PATH=$JAVA_HOME/bin:$PATH
xmake build

adb install -r build/android/arm64-v8a/release/nvi-pfa.apk
```






### All Contributors
- Qishipai: Provider of his C++ MIDI processing library
- Tweak: Creating the PFA imitation and upgrading to SDL3, Implementing a simple overlap remover
- Kpitunor: Most of the UI design, settings, and translations
- Nerdly: UI Font Choice
- Hex: Playback options: Play/Pause, seek backward/forward
- Zeal: Custom Icon
- SlothPlayer: A few built-in themes
