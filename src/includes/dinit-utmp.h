// Wrappers for utmp/wtmp & equivalent database access.

#ifndef DINIT_UTMP_H_INCLUDED
#define DINIT_UTMP_H_INCLUDED

#ifndef USE_UTMPX
#if __linux__ || __FreeBSD__ || __DragonFly__
#define USE_UTMPX 1
#else
#define USE_UTMPX 0
#endif
#endif

#ifndef USE_UPDWTMPX
#ifdef __linux__
#define USE_UPDWTMPX 1
#else
#define USE_UPDWTMPX 0
#endif
#endif

#if USE_UTMPX

#include <cstring>

#include <utmpx.h>
#include <sys/time.h>

// Set the time for a utmpx record to the current time.
inline void set_current_time(struct utmpx *record)
{
#ifdef __linux__
    // On Linux, ut_tv is not actually a struct timeval:
    timeval curtime;
    gettimeofday(&curtime, nullptr);
    record->ut_tv.tv_sec = curtime.tv_sec;
    record->ut_tv.tv_usec = curtime.tv_usec;
#else
    gettimeofday(&record->ut_tv, nullptr);
#endif
}

// Log the boot time to the wtmp database (or equivalent).
inline bool log_boot()
{
    struct utmpx record;
    memset(&record, 0, sizeof(record));
    record.ut_type = BOOT_TIME;

    set_current_time(&record);

    // On FreeBSD, putxline will update all appropriate databases. On Linux, it only updates
    // the utmp database: we need to update the wtmp database explicitly:
#if USE_UPDWTMPX
    updwtmpx(_PATH_WTMPX, &record);
#endif

    setutxent();
    bool success = (pututxline(&record) != NULL);
    endutxent();

    return success;
}

// Create a utmp entry for the specified process, with the given id and tty line.
inline bool create_utmp_entry(const char *utmp_id, const char *utmp_line, pid_t pid)
{
    struct utmpx record;
    memset(&record, 0, sizeof(record));

    record.ut_type = INIT_PROCESS;
    record.ut_pid = pid;
    set_current_time(&record);
    strncpy(record.ut_id, utmp_id, sizeof(record.ut_id));
    strncpy(record.ut_line, utmp_line, sizeof(record.ut_line));

    setutxent();
    bool success = (pututxline(&record) != NULL);
    endutxent();

    return success;
}

// Clear the utmp entry for the given id/line/process.
inline void clear_utmp_entry(const char *utmp_id, const char *utmp_line)
{
    struct utmpx record;
    memset(&record, 0, sizeof(record));

    record.ut_type = DEAD_PROCESS;
    set_current_time(&record);
    strncpy(record.ut_id, utmp_id, sizeof(record.ut_id));
    strncpy(record.ut_line, utmp_line, sizeof(record.ut_line));

    struct utmpx *result;

    setutxent();

    // Try to find an existing entry by id/line and copy the process ID:
    if (*utmp_id) {
        result = getutxid(&record);
    }
    else {
        result = getutxline(&record);
    }

    if (result) {
        record.ut_pid = result->ut_pid;
    }

    pututxline(&record);
    endutxent();
}

#else // Don't update databases:

static inline bool log_boot()
{
    return true;
}

static inline bool create_utmp_entry(const char *utmp_id, const char *utmp_line)
{
    return true;
}

#endif

#endif