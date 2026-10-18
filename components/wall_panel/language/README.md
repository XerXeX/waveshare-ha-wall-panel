# Wall Panel translations

English (`en.yaml`) is the master catalog and fallback language.

To add a language, copy `en.yaml`, rename the copy to the ISO language code (for example `de.yaml` or `fi.yaml`) and change:

```yaml
code: de
name: Deutsch
translations:
  lights: "Lichter"
  # ...
```

On the next ESPHome compile the file is discovered automatically and its `name` is added to the Home Assistant `Language` selector. No C++ or package list needs to be edited.

A translated file may omit keys while it is being completed. Any missing key automatically falls back to English. New UI text should always be added to `en.yaml` first and referenced by its translation key from the UI code.

For languages that need characters outside the currently compiled Roboto Latin glyph set, the shared font configuration in `packages/ui.yaml` must also include the required glyphs/glyph set.
