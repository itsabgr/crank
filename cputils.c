#include <sched.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


int set_current_thread_affinity(int cpu_core) {
    cpu_set_t cpuset;
    pthread_t thread = pthread_self();

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_core, &cpuset);

    int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        errno = result;
        perror("pthread_setaffinity_np");
        return -1;
    }

    // Optional: Verify the affinity was set correctly
    CPU_ZERO(&cpuset);
    result = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        errno = result;
        perror("pthread_getaffinity_np");
        return -1;
    }
    return 0;
}


int set_realtime_scheduler(int policy, int priority) {
    struct sched_param param;

    param.sched_priority = priority;

    if (sched_setscheduler(0, policy, &param) == -1) {
        perror("sched_setscheduler failed");
        return -1;
    }

    return 0;
}

int reset_scheduler() {
    struct sched_param param;

    // For SCHED_OTHER, priority is typically ignored and should be 0
    param.sched_priority = 0;

    // Set scheduler to SCHED_OTHER for the current process
    if (sched_setscheduler(0, SCHED_OTHER, &param) == -1) {
        perror("sched_setscheduler failed");
        return -1;
    }

    return 0;
}
int reset_cpu_affinity() {
    cpu_set_t cpuset;
    pthread_t thread = pthread_self();

    CPU_ZERO(&cpuset); // Initialize the CPU set to be empty

    // Get the number of available CPUs
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus == -1) {
        perror("sysconf");
        return -1;
    }

    // Add all CPUs to the set
    for (int i = 0; i < num_cpus; i++) {
        CPU_SET(i, &cpuset);
    }

    // Set the affinity mask for the thread to all CPUs
    int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        errno = result;
        perror("pthread_setaffinity_np");
        return -1;
    }

    // Optional: Verify the affinity was set correctly
    CPU_ZERO(&cpuset);
    result = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        errno = result;
        perror("pthread_getaffinity_np");
        return -1;
    }

    return 0;
}