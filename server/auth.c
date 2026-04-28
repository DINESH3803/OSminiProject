/*
 * server/auth.c
 * User management: load/save users.dat (fcntl locked), authenticate, CRUD.
 * OS Concepts: fcntl file locking, mutex for in-memory table.
 */
#include "server.h"


/* ─── fcntl lock helper ─────────────────────────────────────── */
static void fcntl_lock(int fd, int type) {
    struct flock fl = {0};
    fl.l_type   = type;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;
    while (fcntl(fd, F_SETLKW, &fl) == -1 && errno == EINTR);
}
static void fcntl_unlock(int fd) {
    struct flock fl = {0};
    fl.l_type   = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fcntl(fd, F_SETLK, &fl);
}

/* ─── Persist users ─────────────────────────────────────────── */
void load_users(void) {
    pthread_mutex_lock(&g_state.user_mutex);
    int fd = open(USERS_FILE, O_RDONLY);
    if (fd < 0) { pthread_mutex_unlock(&g_state.user_mutex); return; }

    fcntl_lock(fd, F_RDLCK);
    g_state.user_count = 0;
    User u;
    while (read(fd, &u, sizeof(User)) == sizeof(User)) {
        if (g_state.user_count < MAX_USERS)
            g_state.users[g_state.user_count++] = u;
    }
    fcntl_unlock(fd);
    close(fd);
    pthread_mutex_unlock(&g_state.user_mutex);
    server_log("Loaded %d users from %s", g_state.user_count, USERS_FILE);
}

void save_users(void) {
    /* Caller must hold user_mutex */
    int fd = open(USERS_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (fd < 0) { perror("save_users open"); return; }

    fcntl_lock(fd, F_WRLCK);
    for (int i = 0; i < g_state.user_count; i++)
        write(fd, &g_state.users[i], sizeof(User));
    fcntl_unlock(fd);
    close(fd);
}

/* ─── Lookup helpers ────────────────────────────────────────── */
User* find_user_by_name(const char* name) {
    for (int i = 0; i < g_state.user_count; i++)
        if (g_state.users[i].active &&
            strcmp(g_state.users[i].username, name) == 0)
            return &g_state.users[i];
    return NULL;
}

User* find_user_by_id(int id) {
    for (int i = 0; i < g_state.user_count; i++)
        if (g_state.users[i].active && g_state.users[i].user_id == id)
            return &g_state.users[i];
    return NULL;
}

/* ─── Authenticate ──────────────────────────────────────────── */
User* authenticate(const char* name, const char* pass) {
    pthread_mutex_lock(&g_state.user_mutex);
    User* u = find_user_by_name(name);
    User* result = NULL;
    if (u && strcmp(u->password, pass) == 0) result = u;
    pthread_mutex_unlock(&g_state.user_mutex);
    return result;
}

/* ─── Add / Delete (Admin only, enforced in handler) ────────── */
int add_user(const char* name, const char* pass, Role role) {
    pthread_mutex_lock(&g_state.user_mutex);
    if (find_user_by_name(name)) {
        pthread_mutex_unlock(&g_state.user_mutex);
        return -1; /* duplicate */
    }
    if (g_state.user_count >= MAX_USERS) {
        pthread_mutex_unlock(&g_state.user_mutex);
        return -2;
    }
    /* Assign a new ID (max existing + 1) */
    int new_id = 1;
    for (int i = 0; i < g_state.user_count; i++)
        if (g_state.users[i].user_id >= new_id)
            new_id = g_state.users[i].user_id + 1;

    User* u = &g_state.users[g_state.user_count++];
    memset(u, 0, sizeof(User));
    u->user_id = new_id;
    strncpy(u->username, name, sizeof(u->username)-1);
    strncpy(u->password, pass, sizeof(u->password)-1);
    u->role   = role;
    u->active = 1;
    save_users();
    pthread_mutex_unlock(&g_state.user_mutex);
    server_log("Added user '%s' (id=%d, role=%s)", name, new_id, role_to_str(role));
    return new_id;
}

int delete_user(int uid) {
    pthread_mutex_lock(&g_state.user_mutex);
    User* u = find_user_by_id(uid);
    if (!u) { pthread_mutex_unlock(&g_state.user_mutex); return -1; }
    u->active = 0;
    save_users();
    pthread_mutex_unlock(&g_state.user_mutex);
    server_log("Deleted user id=%d", uid);
    return 0;
}
