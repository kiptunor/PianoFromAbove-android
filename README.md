# NVirsual Piano From Above
A clone of the original [Piano From Above](https://github.com/brian-pantano/PianoFromAbove) made by Brian Panatano.
Now available on on android.

This clone is powered by Qishipai's [midi processing library](https://github.com/qishipai/NVirsual) and using SDL3 to render the notes and the keyboard.

> [!WARNING]
> This project is still in early development and any release could be unstable. Consider building and testing them at your own risk!!
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
- [x] Custom note colors presets trough .ccol files
- [X] Improving the audio limiter
- [X] Better UI layout on mobile
- [x] File information support
- [X] Custom note color array support
- [x] Veritcal lines on the background
- [x] Background image support
- [ ] Setting custom key ranges
- [X] Common midi and soundfont reloading (This must not require app restarting after each midi playing session)
- [x] MIDI File History
- [X] Builtin File Dialog
- [ ] Performance improvements
- [ ] Loging to text file (This can be useful for development purposes)
- [X] Custom icon and app name
- [ ] GPU Renderer
- [ ] Video capture

# How to build an apk
First you need to make sure you have the android studio and android-sdk installed.

2) Git clone this repo and cd into it.
3) Set the path to the android sdk in [local.properties](local.properties)
4) Run the following commands:
```
./gradlew assembleDebug
```
This only builds the debug version of the apk

*) If you want to build the desktop version (which is currently available and works best on arch linux atm) check out [the guide build](app/jni/src/README.md) from the core source code of the app.






### All Contributors
- Qishipai: provider of his c++ midi processing library
- Tweak: Creating the PFA imitation and upgrading to SDL3, implementing a simple overlap remover
- Kpitunor: Most UI design, settings and translations
- Nerdly: UI Font Choice
- Hex: Playback options: play / pause, seek backwards/forwards
- Zeal: Custom Icon