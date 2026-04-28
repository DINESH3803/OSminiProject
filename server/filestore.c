/*
 * server/filestore.c
 * Persistent patient records stored as binary files in data/patients/.
 * Every read/write uses fcntl advisory locks + semaphore to cap concurrency.
 * OS Concepts: fcntl (F_RDLCK / F_WRLCK), semaphore (sem_wait/sem_post).
 */
#include "server.h"

/* ─── Build path for a patient data file ───────────────────── */
static void patient_path(char* buf, size_t n, int id) {
    snprintf(buf, n, "%s/patient_%04d.dat", PATIENT_DIR, id);
}

/* ─── fcntl helpers ─────────────────────────────────────────── */
static void flock_set(int fd, int type) {
    struct flock fl = { .l_type=type, .l_whence=SEEK_SET, .l_start=0, .l_len=0 };
    while (fcntl(fd, F_SETLKW, &fl) == -1 && errno == EINTR);
}
static void flock_clr(int fd) {
    struct flock fl = { .l_type=F_UNLCK, .l_whence=SEEK_SET };
    fcntl(fd, F_SETLK, &fl);
}

/* ─── Save one patient to disk ──────────────────────────────── */
void save_patient(const Patient* p) {
    /* Semaphore: cap concurrent writers to 3 */
    sem_wait(&g_state.write_sem);

    char path[256];
    patient_path(path, sizeof(path), p->patient_id);

    int fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (fd < 0) { perror("save_patient open"); sem_post(&g_state.write_sem); return; }

    flock_set(fd, F_WRLCK);   /* exclusive write lock */
    write(fd, p, sizeof(Patient));
    flock_clr(fd);

    close(fd);
    sem_post(&g_state.write_sem);
}

/* ─── Load one patient from disk ────────────────────────────── */
int load_patient_from_file(int id, Patient* out) {
    char path[256];
    patient_path(path, sizeof(path), id);

    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    flock_set(fd, F_RDLCK);   /* shared read lock */
    ssize_t r = read(fd, out, sizeof(Patient));
    flock_clr(fd);
    close(fd);
    return (r == sizeof(Patient)) ? 0 : -1;
}

/* ─── Scan directory and load all patients into g_state ─────── */
void load_patients(void) {
    pthread_mutex_lock(&g_state.patient_mutex);
    g_state.patient_count = 0;

    DIR* d = opendir(PATIENT_DIR);
    if (!d) { pthread_mutex_unlock(&g_state.patient_mutex); return; }

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, "patient_", 8) != 0) continue;
        int id = atoi(entry->d_name + 8);
        if (id <= 0) continue;

        Patient p;
        if (load_patient_from_file(id, &p) == 0 && g_state.patient_count < MAX_PATIENTS)
            g_state.patients[g_state.patient_count++] = p;
    }
    closedir(d);
    pthread_mutex_unlock(&g_state.patient_mutex);
    server_log("Loaded %d patients from disk", g_state.patient_count);
}
/*
 * server/server.h
 * Global server state shared across all server translation units.
 */
#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include "../include/common.h"
#include "../include/auth.h"
#include "../include/patient.h"
#include "../include/ipc.h"

/* ─── Global server state ───────────────────────────────────── */
typedef struct ServerState {
    Patient         patients[MAX_PATIENTS];
    int             patient_count;
    pthread_mutex_t patient_mutex;

    User            users[MAX_USERS];
    int             user_count;
    pthread_mutex_t user_mutex;

    sem_t           write_sem;   /* caps concurrent file writes */

    mqd_t           mqueue;
    FILE*           logfile;
    pthread_mutex_t log_mutex;

    int             server_fd;
    volatile int    running;
} ServerState;

/* The one global instance — defined in server.c, extern everywhere else */
extern ServerState g_state;

/* Per-client thread argument */
typedef struct {
    int                fd;
    struct sockaddr_in addr;
} ClientArg;

/* Function prototypes (implemented across server source files) */

/* server.c */
void server_log(const char* fmt, ...);

/* auth.c */
void  load_users(void);
void  save_users(void);
User* find_user_by_name(const char* name);
User* find_user_by_id(int id);
User* authenticate(const char* name, const char* pass);
int   add_user(const char* name, const char* pass, Role role);
int   delete_user(int uid);

/* filestore.c */
void load_patients(void);
void save_patient(const Patient* p);
int  load_patient_from_file(int id, Patient* out);

/* anomaly.c *//*
 * server/server.h
 * Global server state shared across all server translation units.
 */
#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include "../include/common.h"
#include "../include/auth.h"
#include "../include/patient.h"
#include "../include/ipc.h"

/* ─── Global server state ───────────────────────────────────── */
typedef struct ServerState {
    Patient         patients[MAX_PATIENTS];
    int             patient_count;
    pthread_mutex_t patient_mutex;

    User            users[MAX_USERS];
    int             user_count;
    pthread_mutex_t user_mutex;

    sem_t           write_sem;   /* caps concurrent file writes */

    mqd_t           mqueue;
    FILE*           logfile;
    pthread_mutex_t log_mutex;

    int             server_fd;
    volatile int    running;
} ServerState;

/* The one global instance — defined in server.c, extern everywhere else */
extern ServerState g_state;

/* Per-client thread argument */
typedef struct {
    int                fd;
    struct sockaddr_in addr;
} ClientArg;

/* Function prototypes (implemented across server source files) */

/* server.c */
void server_log(const char* fmt, ...);

/* auth.c */
void  load_users(void);
void  save_users(void);
User* find_user_by_name(const char* name);
User* find_user_by_id(int id);
User* authenticate(const char* name, const char* pass);
int   add_user(const char* name, const char* pass, Role role);
int   delete_user(int uid);

/* filestore.c */
void load_patients(void);
void save_patient(const Patient* p);
int  load_patient_from_file(int id, Patient* out);

/* anomaly.c */
AlertSeverity detect_anomaly(const Patient* p, const Vitals* v, Alert* out);

/* ipc_alert.c */
void create_doctor_pipes(void);
void send_alert(const Alert* a);

/* handler.c */
void* client_thread(void* arg);

#endif /* SERVER_H */

AlertSeverity detect_anomaly(const Patient* p, const Vitals* v, Alert* out);

/* ipc_alert.c */
void create_doctor_pipes(void);
void send_alert(const Alert* a);

/* handler.c */
void* client_thread(void* arg);

#endif /* SERVER_H */
