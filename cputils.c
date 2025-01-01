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
        return result;
    }

    CPU_ZERO(&cpuset);
    result = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        errno = result;
        return result;
    }
    return 0;
}


int set_realtime_scheduler(int policy, int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    return sched_setscheduler(0, policy, &param);
}

void reset_scheduler() {
    struct sched_param param;

    param.sched_priority = 0;

    sched_setscheduler(0, SCHED_OTHER, &param);


}
void reset_cpu_affinity() {
    cpu_set_t cpuset;
    pthread_t thread = pthread_self();

    CPU_ZERO(&cpuset);

    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus == -1) {
        return;
    }

    for (int i = 0; i < num_cpus; i++) {
        CPU_SET(i, &cpuset);
    }

    int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        return;
    }

    CPU_ZERO(&cpuset);
    result = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        errno = result;
        perror("pthread_getaffinity_np");
        return;
    }

}