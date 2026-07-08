// SD-card detection logger for the T-Dongle C5. See sd_log.h for the design.
#ifdef BOARD_T_DONGLE_C5

#include "sd_log.h"
#include "display.h"   // display_spi() — the shared SPI bus

#include <FS.h>
#include <SD.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// SD card chip-select (from LilyGO pin_config.h). CLK/MOSI/MISO come from the
// shared bus that the display already started.
#define SD_CS_PIN   23
#define LOG_PATH    "/flock_log.csv"

#define RING_SIZE   24    // pending-write queue depth
#define SEEN_MAX    128   // unique MACs remembered for de-duplication
#define MAC_LEN     20
#define NAME_LEN    34

// One queued detection.
typedef struct {
    char proto[10];
    char method[24];
    char name[NAME_LEN];
    char mac[MAC_LEN];
    int  rssi;
    int  channel;
    int  threat;
    unsigned long ts;
} LogEntry;

// --- shared SPI bus serialization (LCD + SD) -------------------------------
static SemaphoreHandle_t spiMutex = nullptr;

void board_spi_init() {
    if (!spiMutex) spiMutex = xSemaphoreCreateMutex();
}
void board_spi_lock() {
    if (spiMutex) xSemaphoreTake(spiMutex, portMAX_DELAY);
}
void board_spi_unlock() {
    if (spiMutex) xSemaphoreGive(spiMutex);
}

// --- lock-free-ish ring buffer (producers: WiFi/BLE tasks; consumer: loop) --
static LogEntry ring[RING_SIZE];
static volatile uint8_t ring_head = 0;   // next write slot
static volatile uint8_t ring_tail = 0;   // next read slot
static portMUX_TYPE ringMux = portMUX_INITIALIZER_UNLOCKED;

// De-dup list (only touched from the single-threaded flush path).
static char seen[SEEN_MAX][MAC_LEN];
static int  seen_count = 0;

static bool sd_ok = false;
static File logFile;

bool sd_log_init() {
    board_spi_lock();
    // Reuse the SPI instance the display already started (bus is up with the
    // correct SCK/MISO/MOSI pins; SD just adds its own CS).
    sd_ok = SD.begin(SD_CS_PIN, display_spi());
    if (sd_ok) {
        bool newFile = !SD.exists(LOG_PATH);
        logFile = SD.open(LOG_PATH, FILE_APPEND);
        if (logFile) {
            if (newFile) {
                logFile.println("timestamp_ms,protocol,detection_method,identifier,mac,rssi,channel,threat_score,latitude,longitude,altitude_m,accuracy_m");
                logFile.flush();
            }
        } else {
            sd_ok = false;
        }
    }
    board_spi_unlock();

    if (sd_ok) {
        printf("[SD] card mounted - logging detections to %s\n", LOG_PATH);
    } else {
        printf("[SD] no card / mount failed - running without logging\n");
    }
    return sd_ok;
}

bool sd_log_available() { return sd_ok; }

void sd_log_enqueue(const char* protocol, const char* method,
                    const char* identifier, const char* mac,
                    int rssi, int channel, int threat) {
    if (!sd_ok) return;

    portENTER_CRITICAL(&ringMux);
    uint8_t next = (uint8_t)((ring_head + 1) % RING_SIZE);
    if (next != ring_tail) {                 // room in the queue
        LogEntry* e = &ring[ring_head];
        strncpy(e->proto,  protocol   ? protocol   : "", sizeof(e->proto)  - 1); e->proto[sizeof(e->proto)   - 1] = '\0';
        strncpy(e->method, method     ? method     : "", sizeof(e->method) - 1); e->method[sizeof(e->method) - 1] = '\0';
        strncpy(e->name,   identifier ? identifier : "", sizeof(e->name)   - 1); e->name[sizeof(e->name)     - 1] = '\0';
        strncpy(e->mac,    mac        ? mac        : "", sizeof(e->mac)    - 1); e->mac[sizeof(e->mac)       - 1] = '\0';
        e->rssi = rssi; e->channel = channel; e->threat = threat; e->ts = millis();
        ring_head = next;
    }
    // If full we simply drop it; a dropped entry is almost always a duplicate
    // of a device we're about to log anyway.
    portEXIT_CRITICAL(&ringMux);
}

static bool seen_before(const char* mac) {
    for (int i = 0; i < seen_count; i++) {
        if (strcasecmp(seen[i], mac) == 0) return true;
    }
    if (seen_count < SEEN_MAX) {
        strncpy(seen[seen_count], mac, MAC_LEN - 1);
        seen[seen_count][MAC_LEN - 1] = '\0';
        seen_count++;
    }
    return false;
}

void sd_log_flush() {
    if (!sd_ok) return;

    for (;;) {
        LogEntry e;
        bool have = false;

        portENTER_CRITICAL(&ringMux);
        if (ring_tail != ring_head) {
            e = ring[ring_tail];
            ring_tail = (uint8_t)((ring_tail + 1) % RING_SIZE);
            have = true;
        }
        portEXIT_CRITICAL(&ringMux);

        if (!have) break;

        // Log each unique MAC once.
        if (e.mac[0] && seen_before(e.mac)) continue;

        board_spi_lock();
        if (logFile) {
            // identifier is quoted in case an SSID contains a comma.
            logFile.printf("%lu,%s,%s,\"%s\",%s,%d,%d,%d,,,,\n",
                           e.ts, e.proto, e.method, e.name, e.mac,
                           e.rssi, e.channel, e.threat);
            logFile.flush();
        }
        board_spi_unlock();
    }
}

#endif // BOARD_T_DONGLE_C5
