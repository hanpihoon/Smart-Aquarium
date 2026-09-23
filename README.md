# Aquarium Controller - ESP32 CYD 2.8 Landscape

Prototype firmware UI for ESP32-2432S028R / CYD, 320x240 landscape.

## Included screens
- Dashboard
- Automatic water change schedule/volume
- Temperature management
- Manual outputs
- Settings / safety status

## GitHub build
1. Create a new GitHub repository.
2. Upload this project preserving `.github/workflows/build.yml`.
3. Open **Actions > Build ESP32 CYD Firmware > Run workflow**.
4. Download artifact `aquarium-controller-firmware` containing `aquarium-controller.bin`.

## Important
This first build is a UI/logic prototype. Hardware outputs are intentionally not assigned to real valves/heater/chiller yet. Verify the exact CYD revision and I/O expansion wiring before enabling loads.

Touch calibration differs among CYD revisions. If touch coordinates are wrong, adjust TS_MINX/TS_MAXX/TS_MINY/TS_MAXY and/or mapping in `touchXY()`.
