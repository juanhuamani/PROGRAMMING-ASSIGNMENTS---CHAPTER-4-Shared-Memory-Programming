#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <chrono>

using namespace std;
using namespace std::chrono;

long long int total_tosses;
long long int circle_hits = 0;
int thread_count;
pthread_mutex_t mutex;

void* perform_tosses(void* rank) {F 
    long long int tosses_in_circle = 0;
    unsigned int seed = time(nullptr) + (long)rank;  
    long long int tosses_per_thread = total_tosses / thread_count;
    for (long long int i = 0; i < tosses_per_thread; i++) {
        double x = (rand_r(&seed) / (double)RAND_MAX) * 2 - 1;
        double y = (rand_r(&seed) / (double)RAND_MAX) * 2 - 1;
        if (x * x + y * y <= 1) {
            tosses_in_circle++;
        }
    }

    pthread_mutex_lock(&mutex);
    circle_hits += tosses_in_circle;
    pthread_mutex_unlock(&mutex);

    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <number of threads> <number of tosses>" << endl;
        return -1;
    }

    thread_count = atoi(argv[1]);
    total_tosses = atoll(argv[2]);

    pthread_t* threads = new pthread_t[thread_count];
    pthread_mutex_init(&mutex, nullptr);
    auto start = high_resolution_clock::now();
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], nullptr, perform_tosses, (void*)(long)i);
    }
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], nullptr);
    }
    auto end = high_resolution_clock::now();
    duration<double> elapsed = end - start;

    double pi_estimate = 4 * static_cast<double>(circle_hits) / total_tosses;
    cout << "Estimated Pi = " << pi_estimate << endl;
    cout << "Execution Time: " << elapsed.count() << " seconds" << endl;

    pthread_mutex_destroy(&mutex);
    delete[] threads;

    return 0;
}