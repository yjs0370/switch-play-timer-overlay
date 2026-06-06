#include <tesla.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <switch.h>

#define CONFIG_PATH "sdmc:/config/switch-play-timer/config.txt"
#define STATE_PATH  "sdmc:/config/switch-play-timer/state.txt"

static u32  g_timeLimitSec = 15 * 60;
static u32  g_remainingSec  = 15 * 60;
static bool g_timerActive    = false;
static u64  g_startTicks    = 0;

static void mkdirp(const char *path) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') { *p = '\0'; mkdir(tmp, 0777); *p = '/'; }
    }
    mkdir(tmp, 0777);
}

static void loadConfig() {
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "time_limit_minutes=", 19) == 0) {
            int m = atoi(line + 19);
            if (m > 0 && m <= 480) {
                g_timeLimitSec = (u32)m * 60;
                g_remainingSec  = g_timeLimitSec;
            }
        }
    }
    fclose(f);
}

static void saveConfig() {
    mkdirp(CONFIG_PATH);
    FILE *f = fopen(CONFIG_PATH, "w");
    if (f) { fprintf(f, "time_limit_minutes=%u\n", g_timeLimitSec / 60); fclose(f); }
}

static void loadState() {
    FILE *f = fopen(STATE_PATH, "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "start_ticks=", 12) == 0)
            g_startTicks = strtoull(line + 12, NULL, 10);
        if (strncmp(line, "active=", 7) == 0)
            g_timerActive = atoi(line + 7) != 0;
        if (strncmp(line, "remaining=", 10) == 0)
            g_remainingSec = (u32)strtoul(line + 10, NULL, 10);
    }
    fclose(f);
    if (g_timerActive && g_startTicks > 0) {
        u64 now = armGetSystemTick();
        u64 elapsedTicks = now - g_startTicks;
        u32 elapsedSec = (u32)(elapsedTicks / (ArmSystemTickFreq / 1000000000.0));
        g_remainingSec = (elapsedSec < g_timeLimitSec) ? (g_timeLimitSec - elapsedSec) : 0;
    }
}

static void saveState() {
    mkdirp(STATE_PATH);
    FILE *f = fopen(STATE_PATH, "w");
    if (f) {
        fprintf(f, "start_ticks=%llu\n", (unsigned long long)g_startTicks);
        fprintf(f, "active=%d\n", g_timerActive ? 1 : 0);
        fprintf(f, "remaining=%u\n", g_remainingSec);
        fclose(f);
    }
}

class MainGui : public tsl::Gui {
public:
    MainGui() { loadConfig(); loadState(); }

    virtual tsl::elm::Element* createUI() override {
        auto *root = new tsl::elm::OverlayFrame("Play Timer", "v1.0");
        auto *list = new tsl::elm::List();

        u32 h = g_remainingSec / 3600;
        u32 m = (g_remainingSec % 3600) / 60;
        u32 s = g_remainingSec % 60;
        char buf[64];
        if (h > 0)
            snprintf(buf, sizeof(buf), "%02u:%02u:%02u", h, m, s);
        else
            snprintf(buf, sizeof(buf), "%02u:%02u", m, s);

        list->addItem(new tsl::elm::CategoryHeader(buf));

        if (g_timerActive)
            list->addItem(new tsl::elm::ListItem("Running  (A = Pause)"));
        else if (g_remainingSec == 0)
            list->addItem(new tsl::elm::ListItem("Time is up!  (X = Reset)"));
        else
            list->addItem(new tsl::elm::ListItem("Paused  (A = Start)"));

        char limBuf[64];
        snprintf(limBuf, sizeof(limBuf), "Limit: %u min  (+/- to adjust)", g_timeLimitSec / 60);
        list->addItem(new tsl::elm::ListItem(limBuf));

        list->addItem(new tsl::elm::ListItem("A = Start / Pause"));
        list->addItem(new tsl::elm::ListItem("X = Reset timer"));
        list->addItem(new tsl::elm::ListItem("+ / - = Adjust limit"));

        root->setContent(list);
        return root;
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld,
                            const HidTouchState &touchPos,
                            HidAnalogStickState leftJoyStick,
                            HidAnalogStickState rightJoyStick) override {
        if (keysDown & HidNpadButton_A) {
            if (!g_timerActive) {
                g_startTicks = armGetSystemTick();
                g_timerActive = true;
            } else {
                g_timerActive = false;
            }
            saveState();
            return true;
        }
        if (keysDown & HidNpadButton_X) {
            g_startTicks   = 0;
            g_remainingSec = g_timeLimitSec;
            g_timerActive  = false;
            saveState();
            return true;
        }
        if (keysDown & HidNpadButton_Plus) {
            g_timeLimitSec += 5 * 60;
            if (g_timeLimitSec > 480 * 60) g_timeLimitSec = 480 * 60;
            if (!g_timerActive) g_remainingSec = g_timeLimitSec;
            saveConfig();
            return true;
        }
        if (keysDown & HidNpadButton_Minus) {
            if (g_timeLimitSec > 5 * 60) g_timeLimitSec -= 5 * 60;
            if (!g_timerActive) g_remainingSec = g_timeLimitSec;
            saveConfig();
            return true;
        }
        return false;
    }

    virtual void update() override {
        if (g_timerActive && g_startTicks > 0) {
            u64 now = armGetSystemTick();
            u64 elapsedTicks = now - g_startTicks;
            u32 elapsedSec = (u32)(elapsedTicks / (ArmSystemTickFreq / 1000000000.0));
            g_remainingSec = (elapsedSec < g_timeLimitSec)
                                ? (g_timeLimitSec - elapsedSec) : 0;
            if (g_remainingSec == 0) g_timerActive = false;
            saveState();
        }
    }
};

class PlayTimerOverlay : public tsl::Overlay {
public:
    virtual void onShow() override {}
    virtual tsl::Gui* loadInitialGui() override { return new MainGui(); }
};

int main(int argc, char **argv) {
    return tsl::Loop<PlayTimerOverlay>(argc, argv);
}
