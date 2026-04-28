#ifndef IPC_H
#define IPC_H

#include "common.h"

/* Alert struct transmitted over named pipe and mqueue */
typedef struct {
    int           patient_id;
    int           doctor_id;
    char          patient_name[64];
    char          vital_field[32];
    float         value;
    char          message[256];
    time_t        timestamp;
    AlertSeverity severity;
} Alert;

/* Build named-pipe path for a doctor */
static inline void alert_pipe_path(char* buf, size_t n, int doctor_id) {
    snprintf(buf, n, "/tmp/icu_alert_%d", doctor_id);
}

static inline void print_alert(const Alert* a) {
    char ts[32]; timestamp_str(ts, sizeof(ts), a->timestamp);
    const char* col = sev_colour(a->severity);
    printf("\n%s%s┌─ ALERT ─────────────────────────────────────────────┐%s\n",
           BOLD, col, RESET);
    printf("%s%s│ [%s] %s%s\n", BOLD, col, sev_str(a->severity), ts, RESET);
    printf("%s%s│ Patient #%d — %s%s\n", BOLD, col, a->patient_id, a->patient_name, RESET);
    printf("%s%s│ %s%s\n", BOLD, col, a->message, RESET);
    printf("%s%s└──────────────────────────────────────────────────────┘%s\n\n",
           BOLD, col, RESET);
}

#endif /* IPC_H */
