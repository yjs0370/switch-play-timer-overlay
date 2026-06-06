#include <switch.h>
#include <stdio.h>
#include <time.h>

// Play Timer Overlay - minimal version without libtesla
// Shows remaining play time as a notification

int main(int argc, char **argv) {
    consoleInit(NULL);
    
    // Initialize services
    pmdmntInitialize();
    pmdinfoInitialize();
    timeInitialize();
    
    consoleUpdate(NULL);
    
    printf("=== Switch Play Timer ===\n");
    printf("Initializing...\n");
    consoleUpdate(NULL);
    
    // Get current play time limit from pctl
    // This requires pctl:npdc service access
    
    u64 playTime = 0;
    u64 timeLimit = 0;
    
    // Try to read play time stats
    Result rc = pmdinfoGetPlayTime(&playTime);
    if (R_SUCCEEDED(rc)) {
        printf("Play time today: %llu seconds\n", playTime);
    } else {
        printf("Failed to get play time: 0x%x\n", rc);
    }
    
    // Try to get restriction info
    PctlRestriction restriction;
    rc = pctlGetRestriction(&restriction);
    if (R_SUCCEEDED(rc)) {
        printf("Time limit: %d min/day\n", restriction.play_timer_seconds / 60);
        timeLimit = restriction.play_timer_seconds;
    } else {
        printf("Failed to get restriction: 0x%x\n", rc);
    }
    
    printf("\nPress + to exit\n");
    consoleUpdate(NULL);
    
    // Main loop
    while (appletMainLoop()) {
        consoleUpdate(NULL);
        
        u64 keys = 0;
        hidScanInput();
        keys = hidKeysDown(CONTROLLER_P1_AUTO);
        
        if (keys & KEY_PLUS)
            break;
            
        // Update remaining time display
        rc = pmdinfoGetPlayTime(&playTime);
        if (R_SUCCEEDED(rc) && timeLimit > 0) {
            u64 remaining = (timeLimit > playTime) ? (timeLimit - playTime) : 0;
            printf("\rRemaining: %llu min %llu sec  ", remaining / 60, remaining % 60);
        }
        
        consoleUpdate(NULL);
        svcSleepThread(1000000000ULL); // 1 second
    }
    
    // Cleanup
    timeExit();
    pmdinfoExit();
    pmdmntExit();
    consoleExit(NULL);
    
    return 0;
}
