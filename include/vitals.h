#ifndef VITALS_H
#define VITALS_H

#include "common.h"

/* ─── One vital-signs reading ───────────────────────────────── */
typedef struct {
    int    patient_id;
    float  heart_rate;    /* bpm        normal: 60-100  */
    float  systolic_bp;   /* mmHg       normal: 90-140  */
    float  diastolic_bp;  /* mmHg       normal: 60-90   */
    float  spo2;          /* %          normal: 95-100  */
    float  temperature;   /* Celsius    normal: 36.1-37.2 */
    float  resp_rate;     /* breaths/m  normal: 12-20   */
    int    nurse_id;
    time_t timestamp;
} Vitals;

/* ─── Threshold definition {warn_lo, warn_hi, crit_lo, crit_hi} */
typedef struct { float wlo, whi, clo, chi; } Threshold;

/* Normal/warn/critical bands per vital */
static const Threshold THR_HR   = { 50.0f, 110.0f,  40.0f, 130.0f };
static const Threshold THR_SBP  = { 85.0f, 145.0f,  70.0f, 180.0f };
static const Threshold THR_DBP  = { 50.0f,  90.0f,  40.0f, 100.0f };
static const Threshold THR_SPO2 = { 94.0f, 100.0f,  90.0f, 100.0f }; /* only low matters */
static const Threshold THR_TEMP = { 36.0f,  37.5f,  35.0f,  39.0f };
static const Threshold THR_RR   = { 10.0f,  22.0f,   8.0f,  30.0f };

/* Check one value against its threshold; returns SEV_NONE / SEV_WARNING / SEV_CRITICAL */
static inline AlertSeverity check_threshold(float v, Threshold t) {
    if (v <= t.clo || v >= t.chi) return SEV_CRITICAL;
    if (v <= t.wlo || v >= t.whi) return SEV_WARNING;
    return SEV_NONE;
}

/* Return worst severity among all vitals */
static inline AlertSeverity vitals_worst_severity(const Vitals* v) {
    AlertSeverity s = SEV_NONE, tmp;
    tmp = check_threshold(v->heart_rate,   THR_HR);   if (tmp > s) s = tmp;
    tmp = check_threshold(v->systolic_bp,  THR_SBP);  if (tmp > s) s = tmp;
    tmp = check_threshold(v->diastolic_bp, THR_DBP);  if (tmp > s) s = tmp;
    tmp = check_threshold(v->spo2,         THR_SPO2); if (tmp > s) s = tmp;
    tmp = check_threshold(v->temperature,  THR_TEMP); if (tmp > s) s = tmp;
    tmp = check_threshold(v->resp_rate,    THR_RR);   if (tmp > s) s = tmp;
    return s;
}

static inline void print_vitals(const Vitals* v) {
    char ts[32]; timestamp_str(ts, sizeof(ts), v->timestamp);
    AlertSeverity hr_s  = check_threshold(v->heart_rate,   THR_HR);
    AlertSeverity sbp_s = check_threshold(v->systolic_bp,  THR_SBP);
    AlertSeverity sp_s  = check_threshold(v->spo2,         THR_SPO2);
    AlertSeverity tm_s  = check_threshold(v->temperature,  THR_TEMP);
    AlertSeverity rr_s  = check_threshold(v->resp_rate,    THR_RR);
    printf("  %s%-20s%s  %s%6.1f%s bpm  |  "
           "%s%3.0f%s/%s%3.0f%s mmHg  |  "
           "%sSPO2:%s%.1f%%  |  "
           "%sTemp:%s%.1f°C  |  "
           "%sRR:%s%.0f bpm/m\n",
        DIM, ts, RESET,
        sev_colour(hr_s),  v->heart_rate,   RESET,
        sev_colour(sbp_s), v->systolic_bp,  RESET,
        sev_colour(sbp_s), v->diastolic_bp, RESET,
        sev_colour(sp_s),  RESET, v->spo2,
        sev_colour(tm_s),  RESET, v->temperature,
        sev_colour(rr_s),  RESET, v->resp_rate);
}

#endif /* VITALS_H */
