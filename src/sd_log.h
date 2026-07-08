#ifndef SD_LOG_H
#define SD_LOG_H

// SD-card detection logger for the T-Dongle C5.
// Only compiled when BOARD_T_DONGLE_C5 is defined.
//
// The C5's SD card slot shares its SPI bus with the LCD. Because detections can
// arrive from background WiFi/BLE tasks (which also drive the LCD) while the
// logger writes from loop(), all access to that shared bus is serialized through
// a FreeRTOS mutex created by board_spi_init(). If no card is present, every
// function below is a safe no-op and the device runs normally.

#ifdef BOARD_T_DONGLE_C5

#include <Arduino.h>

// Create the shared-SPI-bus mutex. MUST be called once, before display_init()
// or any other user of the bus.
void board_spi_init();

// Acquire / release the shared SPI bus (used by both the SD logger and the
// display module). Safe to call before board_spi_init() (no-op until created).
void board_spi_lock();
void board_spi_unlock();

// Mount the SD card on the display's shared SPI bus and open the log file.
// Returns true if a card was found and the log is ready. Safe if no card.
bool sd_log_init();

// Whether a card is mounted and logging is active.
bool sd_log_available();

// Queue a detection to be written to the SD log. Fast and task-safe: it only
// copies into a small ring buffer under a short critical section. The actual
// SD write happens later in sd_log_flush(). Duplicate MACs are skipped so each
// unique device is logged once. channel = -1 for BLE (no WiFi channel).
void sd_log_enqueue(const char* protocol, const char* method,
                    const char* identifier, const char* mac,
                    int rssi, int channel, int threat);

// Drain queued detections to the SD card. Call from loop() (single context).
void sd_log_flush();

#endif // BOARD_T_DONGLE_C5
#endif // SD_LOG_H
