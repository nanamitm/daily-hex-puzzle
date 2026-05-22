# Daily Hexagonal Puzzle and its solver

A hexagonal grid puzzle like the genius [A Puzzle A Day](https://www.dragonfjord.com/product/a-puzzle-a-day/). 

* You can enjoy the puzzle every day.
* Using the solver program, I confirmed there is at least one solution for every case.
* The smaller versions (43cells ver.1 and ver.2) can be solved without flipping pieces.

![twos](https://user-images.githubusercontent.com/86639425/160240637-bd596194-aeb0-4a33-8ce1-047b24ac6391.jpg)
![doublesidess](https://user-images.githubusercontent.com/86639425/162553448-dc9250c2-12c1-4eb3-b639-b00b35baaf6a.jpg)

![two-sym](https://user-images.githubusercontent.com/86639425/160240662-4750d954-3689-4ada-a098-f406767a54ca.jpg)
![ss-twos](https://user-images.githubusercontent.com/86639425/160240657-ea850562-6e41-4a3c-a31c-401118102dfb.jpg)
![43v2s](https://user-images.githubusercontent.com/86639425/160533646-ec43ba7a-1fba-483d-b0c9-895b62a3d31d.jpg)

## Desktop app (Qt Widgets)

A Qt 6 desktop solver that runs on Windows, Linux x86-64, and Linux ARM64.
Download the latest release from the [Releases](../../releases) page.

| Platform | File |
|----------|------|
| Windows x64 | `DailyHexPuzzle-windows-x64.zip` |
| Linux x86-64 | `DailyHexPuzzle-linux-x64.AppImage` |
| Linux ARM64  | `DailyHexPuzzle-linux-arm64.AppImage` |

**Features**
- Three board variants: 43-cell v1, 43-cell v2, 61-cell
- Find-all mode and slideshow (cycles through all solutions)
- Auto-update at midnight

**Build (desktop)**
```bash
cmake -S qt-client -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x
cmake --build build --config Release
```

## Android app (Qt Quick / QML)

A Qt Quick port for Android (arm64-v8a).
Download `DailyHexPuzzle-android-arm64.apk` from the [Releases](../../releases) page.

**Same features as the desktop app**, plus touch gestures:
- Swipe left/right to cycle through solutions
- Tap the date label to open the date picker; swipe the label to go ±1 day
- Long-press the date label to jump to today

**Requirements to build**
- Qt 6.11.1 for Android arm64-v8a (+ Linux host tools)
- Android NDK 27.2
- Android SDK build-tools 36 / platform 36
- JDK 21

```bash
cmake -S android-client -B build-android \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_NDK=/path/to/ndk \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.11.1/android_arm64_v8a \
  -DQT_HOST_PATH=/path/to/Qt/6.11.1/gcc_64
cmake --build build-android --target apk
```

**CI / Release signing secrets** (required to enable signed APK in GitHub Actions)

| Secret name | Value |
|-------------|-------|
| `ANDROID_KEYSTORE_BASE64` | `base64 -w0 your-release.jks` |
| `ANDROID_KEYSTORE_ALIAS`  | Key alias inside the keystore |
| `ANDROID_KEYSTORE_PASS`   | Keystore (and key) password |
