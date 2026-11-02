<img src="assets/logo.svg" width="96" alt="Floating Translator logo">

# Floating Translator

A small always-on-top Windows panel with two tabs:

- **Sentence**: translates text with the DeepL API (Ctrl+Enter to translate).
- **Word**: looks up a word or expression in the PONS dictionary.

The title bar has a pin (always on top on/off), an opacity slider, the API key settings, and minimize/close.
Drag the title bar to move the window and the bottom-right corner to resize it.

## API keys

Both keys are entered in the settings dialog (gear icon). It opens automatically on first launch.

- **DeepL**: create a DeepL API Free account at <https://www.deepl.com/pro-api> (500,000 characters/month),
  then copy the key from your account page. Free keys end in `:fx`.
- **PONS**: log in at pons.com and fill in the API registration form at
  <https://en.pons.com/p/online-dictionary/developers/api> (1,000 lookups/month free). PONS calls the key a "secret".

The keys are stored in the registry under `HKEY_CURRENT_USER\Software\FloatingTranslator\FloatingTranslator`,
never in the project files.

## Build

Requires Qt 6.5 or newer (Widgets and Network) and CMake 3.21 or newer. Tested with Qt 6.10.3 and MinGW 13.1.
The compiler and Ninja must be on the `PATH`. For Qt's MinGW, that's `C:\Qt\Tools\mingw1310_64\bin`.

From PowerShell, in this folder:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.10.3/mingw_64
cmake --build build
cmake --install build --prefix "$PWD\dist"
```

The last command creates `dist\bin\FloatingTranslator.exe` with the Qt DLLs next to it, so it runs without Qt
installed. The prefix must be an absolute path.

To build from Qt Creator or VS Code (CMake Tools), add a `CMakeUserPresets.json` with your Qt and compiler paths.
Git ignores that file because the paths are specific to each machine.

## License

MIT, see [LICENSE](LICENSE). The app links dynamically to Qt, which is available under the LGPLv3.
