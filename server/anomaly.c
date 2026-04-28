/*
 * server/anomaly.c
 * Anomaly detection: static thresholds + moving average + 2-sigma check.
 * OS Concepts: mutex-protected shared patient history.
 */
#include "server.h"


/* ─── Compute mean and stddev of last N readings for a field ── */
typedef float (*VitalGetter)(const Vitals*);
static float get_hr (const Vitals* v){ return v->heart_rate; }
static float get_sbp(const Vitals* v){ return v->systolic_bp; }
static float get_sp (const Vitals* v){ return v->spo2; }
static float get_tm (const Vitals* v){ return v->temperature; }
static float get_rr (const Vitals* v){ return v->resp_rate; }

static void moving_stats(const Patient* p, VitalGetter fn,
                          float* mean_out, float* std_out) {
    if (p->history_count == 0) { *mean_out = 0; *std_out = 0; return; }
    float sum = 0;
    int n = p->history_count;
    for (int i = 0; i < n; i++) sum += fn(&p->history[i]);
    float mean = sum / n;
    float var = 0;
    for (int i = 0; i < n; i++) {
        float d = fn(&p->history[i]) - mean;
        var += d * d;
    }
    *mean_out = mean;
    *std_out  = (n > 1) ? sqrtf(var / (n-1)) : 0.0f;
}

/* ─── Build human-readable anomaly description ──────────────── */
static AlertSeverity describe_vitals(const Vitals* v, char* msg, size_t msz) {
    AlertSeverity worst = SEV_NONE;
    char part[64];
    msg[0] = '\0';

    struct { const char* name; float val; Threshold thr; } checks[] = {
        { "Heart Rate",   v->heart_rate,   THR_HR   },
        { "Systolic BP",  v->systolic_bp,  THR_SBP  },
        { "Diastolic BP", v->diastolic_bp, THR_DBP  },
        { "SpO2",         v->spo2,         THR_SPO2 },
        { "Temperature",  v->temperature,  THR_TEMP },
        { "Resp Rate",    v->resp_rate,    THR_RR   },
    };
    for (int i = 0; i < 6; i++) {
        AlertSeverity s = check_threshold(checks[i].val, checks[i].thr);
        if (s == SEV_NONE) continue;
        if (s > worst) worst = s;
        snprintf(part, sizeof(part), "%s=%.1f(%s) ",
                 checks[i].name, checks[i].val, sev_str(s));
        strncat(msg, part, msz - strlen(msg) - 1);
    }
    return worst;
}

/* ─── Main entry: analyse vitals, return severity + fill Alert */
AlertSeverity detect_anomaly(const Patient* p, const Vitals* v, Alert* out) {
    /* 1. Static threshold check */
    char thresh_msg[256] = "";
    AlertSeverity threshold_sev = describe_vitals(v, thresh_msg, sizeof(thresh_msg));

    /* 2. Moving-average sigma check (flags as WARNING at minimum) */
    AlertSeverity sigma_sev = SEV_NONE;
    char sigma_msg[128] = "";

    struct { const char* name; VitalGetter fn; float val; } fields[] = {
        { "HR",  get_hr,  v->heart_rate  },
        { "SBP", get_sbp, v->systolic_bp },
        { "SpO2",get_sp,  v->spo2        },
        { "Temp",get_tm,  v->temperature },
        { "RR",  get_rr,  v->resp_rate   },
    };
    for (int i = 0; i < 5; i++) {
        float mean, sd;
        moving_stats(p, fields[i].fn, &mean, &sd);
        if (sd > 0.01f && fabsf(fields[i].val - mean) > 2.0f * sd) {
            sigma_sev = SEV_WARNING;
            char part[48];
            snprintf(part, sizeof(part), "%s σ-outlier(%.1f vs avg %.1f) ",
                     fields[i].name, fields[i].val, mean);
            strncat(sigma_msg, part, sizeof(sigma_msg)-strlen(sigma_msg)-1);
        }
    }

    AlertSeverity final_sev = (threshold_sev > sigma_sev) ? threshold_sev : sigma_sev;
    if (final_sev == SEV_NONE) return SEV_NONE;

    /* Fill Alert struct */
    memset(out, 0, sizeof(Alert));
    out->patient_id = p->patient_id;
    out->doctor_id  = p->assigned_doctor_id;
    strncpy(out->patient_name, p->name, sizeof(out->patient_name)-1);
    out->severity   = final_sev;
    out->timestamp  = v->timestamp;
    out->value      = v->heart_rate; /* representative value */
    /* Combine messages safely */
    strncpy(out->message, thresh_msg, sizeof(out->message)-1);
    out->message[sizeof(out->message)-1] = '\0';
    size_t used = strlen(out->message);
    if (used < sizeof(out->message)-2) {
        out->message[used] = ' ';
        strncpy(out->message+used+1, sigma_msg,
                sizeof(out->message)-used-2);
        out->message[sizeof(out->message)-1] = '\0';
    }

    return final_sev;
}
