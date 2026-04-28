#ifndef PATIENT_H
#define PATIENT_H

#include "vitals.h"

typedef struct {
    int    patient_id;
    char   name[64];
    int    age;
    char   condition[128];
    int    assigned_doctor_id;
    int    assigned_nurses[8];
    int    nurse_count;
    Vitals history[MAX_HISTORY];  /* circular buffer */
    int    history_count;
    int    history_head;
    int    active;
    time_t admitted_at;
} Patient;

static inline void print_patient_summary(const Patient* p) {
    char ts[32]; timestamp_str(ts, sizeof(ts), p->admitted_at);
    printf(BOLD "  Patient #%d" RESET " — %s%s%s  (Age: %d)\n",
           p->patient_id, BRIGHT_CYAN, p->name, RESET, p->age);
    printf("  Condition : %s\n", p->condition);
    printf("  Doctor ID : %d  |  Admitted: %s\n", p->assigned_doctor_id, ts);
}

static inline void patient_add_vitals(Patient* p, const Vitals* v) {
    p->history[p->history_head] = *v;
    p->history_head = (p->history_head + 1) % MAX_HISTORY;
    if (p->history_count < MAX_HISTORY) p->history_count++;
}

#endif /* PATIENT_H */
