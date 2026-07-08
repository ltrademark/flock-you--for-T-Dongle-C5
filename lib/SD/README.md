# Vendored ESP32 SD library

This is the SD library that ships with the Arduino-ESP32 core
(`framework-arduinoespressif32/libraries/SD`), copied here on purpose.

PlatformIO's Library Dependency Finder otherwise auto-installs the unrelated
registry package `SD` (arduino-libraries, `architectures=*`), whose
`utility/Sd2PinMap.h` is AVR-only and fails to compile on the ESP32-C5 with
"Architecture or board not supported". Both packages are named `SD`, so
`lib_ignore = SD` can't disambiguate them.

Placing the correct core SD here (project `lib/` has the highest LDF priority)
forces the right library to win. Only used by the T-Dongle C5 SD logging in
`src/sd_log.cpp`.
