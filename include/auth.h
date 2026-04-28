#ifndef AUTH_H
#define AUTH_H

#include "common.h"

typedef struct {
    int   user_id;
    char  username[32];
    char  password[64];       /* plaintext for demo */
    Role  role;
    int   assigned_ids[10];   /* patient IDs for doctor/nurse */
    int   assigned_count;
    pid_t pid;                /* doctor: PID for SIGUSR1 */
    int   active;
} User;

#endif /* AUTH_H */
