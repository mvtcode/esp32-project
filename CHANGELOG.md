# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.2.0] - 2026-01-27

### Added

#### APSTA Mode (Simultaneous AP + STA)

- **Dual WiFi Mode**: ESP32 now runs both Access Point (192.168.4.1) and Station mode simultaneously
- **Always Accessible**: Configuration interface available via both AP IP and home WiFi IP
- **Seamless Experience**: No need to switch between AP and STA modes manually

#### Advanced Configuration Interface

- **New Web UI**: `config_sta.html` for advanced settings (STA mode only)
- **HTTP Basic Authentication**: Secure access with admin username and password
- **Admin Password**: Set during first-time setup, required for accessing advanced config
- **Dual Routing**:
  - AP clients → `index.html` (first-time setup, no auth)
  - STA clients → `config_sta.html` (advanced config, with auth)

#### Runtime Brightness Control

- **Web Slider**: Adjust LED brightness from 0-255 via web interface
- **Real-time Preview**: See brightness value update as you drag the slider
- **Instant Apply**: Changes take effect immediately without restart
- **Persistent Storage**: Brightness setting saved to NVS

#### Smart Sleep Mode

- **Configurable Schedule**: Set sleep time (e.g., 23:00) and wake time (e.g., 06:00)
- **Brightness Presets**: Choose sleep brightness (Off, Very Dim, Dim, Medium)
- **Cross-Midnight Support**: Handles schedules that span midnight (e.g., 23:00 → 06:00)
- **Automatic Activation**: Checks every minute and adjusts brightness automatically
- **Web Configuration**: Enable/disable and configure via web interface

#### REST API Endpoints

- `GET /api/config` - Get current configuration (protected)
- `POST /api/brightness` - Update brightness (protected)
- `POST /api/sleep` - Update sleep settings (protected)
- `POST /api/admin-password` - Change admin password (protected)
- `GET /api/wifi` - Scan WiFi networks (public)
- `POST /api/save` - Save initial config (public, AP mode only)

### Changed

- **Configuration Structure**: Expanded `ConfigData` struct with new fields:
  - `adminPassword[64]` - Admin authentication
  - `brightness` (uint8_t) - LED brightness (0-255)
  - `sleepEnabled` (bool) - Sleep mode toggle
  - `sleepHour`, `sleepMinute` - Sleep start time
  - `wakeHour`, `wakeMinute` - Wake time
  - `sleepBrightness` (uint8_t) - Brightness during sleep
  - MQTT placeholders (for v2.3.x)
  - WebSocket placeholders (for v2.3.x)

- **Web Server**: Complete rewrite of `web_server.cpp`
  - Replaced `setupAPMode()` with `setupAPSTAMode()`
  - Added authentication middleware
  - Implemented dual routing logic
  - Added new API endpoints

- **Main Loop**:
  - Added `checkSleepMode()` function (runs every 60 seconds)
  - Added `updateBrightnessRuntime()` callback
  - Integrated DNS handling for captive portal in APSTA mode
  - Load and apply brightness from config on startup

### Technical Details

- **Authentication**: Using AsyncWebServer's built-in `authenticate()` method
- **Config Sync**: `globalConfig` exported to web_server for auth access
- **Sleep Logic**: Handles both normal and cross-midnight schedules
- **NVS Storage**: All new settings persisted to non-volatile storage

### Documentation

- Updated README.md with:
  - New features in checklist
  - Advanced configuration usage guide
  - API endpoints documentation with curl examples
  - APSTA mode explanation
  - Sleep mode setup instructions

## [2.1.0] - 2026-01-22

### Added

- **WiFi Captive Portal**: Auto-redirect to config page when connected to AP
- **WiFi Network List**: Scan and display available networks in web interface
- **WiFi Security Detection**: Auto-enable/disable password field based on network encryption

### Changed

- Improved web interface UX with dynamic WiFi selection
- Enhanced first-time setup experience

## [2.0.0] - 2026-01-20

### Added

- Initial release
- LED Matrix clock display with gradient effects
- NTP time synchronization
- Weather data from Open-Meteo API
- Lunar calendar display
- Web-based WiFi configuration
- NVS configuration storage
- Hardware reset button (BOOT button)

### Features

- Dual-size font display (hours/minutes in size 2, seconds in size 1)
- Animated colon separator with ping-pong effect
- Rotating display modes: Date → Weather → Lunar Calendar (5s each)
- Background weather updates via FreeRTOS task
- Vietnamese city presets with GPS coordinates

---

## Upgrade Guide

### From v2.0.x/v2.1.x to v2.2.0

1. **Backup**: Note down your current WiFi credentials and location
2. **Upload**: Flash new firmware via PlatformIO
3. **Upload FS**: Run `pio run --target uploadfs` to update web files
4. **First Boot**: ESP32 will enter AP mode (existing config preserved)
5. **Set Admin Password**: Access `http://192.168.4.1/` and set admin password
6. **Done**: ESP32 will restart and connect to your WiFi

> **Note**: Existing WiFi and location settings are preserved. You only need to set the new admin password.

### Breaking Changes

- None. All existing configurations are backward compatible.

---

## Future Roadmap

### v2.3.x (Planned)

- MQTT integration for IoT platforms
- WebSocket support for real-time updates
- Special dates display (holidays, birthdays)
- Lunar calendar good/bad days
- Historical weather data logging

### v3.0.x (Planned)

- Audio module integration (alarm clock, Bluetooth speaker)
- Mobile app for remote control
- Multi-language support
- Custom display modes

---

**Repository**: [ESP32 LED Matrix Clock](https://github.com/yourusername/esp32-led-matrix-clock)  
**Author**: Mạc Tân  
**License**: MIT
