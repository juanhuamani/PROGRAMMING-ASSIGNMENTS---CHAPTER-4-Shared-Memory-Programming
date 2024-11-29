#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

const int MAX_KEY = 65536;
const int MAX_THREADS = 1024;

struct operation {
    int type;  // 0 = member, 1 = insert, 2 = delete
    int value;
};

struct node {
    int data;
    struct node* next;
};

struct operation* operations;
int* initial_values;
int n = 100000;    
int m = 1000000;   
float mMember = 0.50;
float mInsert = 0.25;
float mDelete = 0.25;
int thread_count = 1;

pthread_mutex_t rw_mutex;
pthread_cond_t cond_readers;
pthread_cond_t cond_writers;
int readers = 0;
int writers = 0;
int waiting_writers = 0;

int Member(struct node* head, int value) {
    struct node* temp = head;
    while (temp != NULL && temp->data < value) {
        temp = temp->next;
    }
    return (temp != NULL && temp->data == value);
}

int Insert(struct node** head, int value) {
    struct node* current = *head;
    struct node* pred = NULL;
    struct node* temp;
    int return_value = 1;

    while (current != NULL && current->data < value) {
        pred = current;
        current = current->next;
    }

    if (current == NULL || current->data > value) {
        temp = malloc(sizeof(struct node));
        temp->data = value;
        temp->next = current;
        if (pred == NULL) {
            *head = temp;
        } else {
            pred->next = temp;
        }
    } else {
        return_value = 0;
    }
    return return_value;
}

int Delete(struct node** head, int value) {
    struct node* current = *head;
    struct node* pred = NULL;
    int return_value = 1;

    while (current != NULL && current->data < value) {
        pred = current;
        current = current->next;
    }

    if (current != NULL && current->data == value) {
        if (pred == NULL) {
            *head = current->next;
        } else {
            pred->next = current->next;
        }
        free(current);
    } else {
        return_value = 0;
    }
    return return_value;
}

void Clear_Memory(struct node* head) {
    struct node* current = head;
    struct node* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

void ReadLock(int prefer_readers) {
    pthread_mutex_lock(&rw_mutex);
    if (prefer_readers) {
        while (writers > 0) {
            pthread_cond_wait(&cond_readers, &rw_mutex);
        }
    } else {
        while (writers > 0 || waiting_writers > 0) {
            pthread_cond_wait(&cond_readers, &rw_mutex);
        }
    }
    readers++;
    pthread_mutex_unlock(&rw_mutex);
}

void ReadUnlock() {
    pthread_mutex_lock(&rw_mutex);
    readers--;
    if (readers == 0) {
        pthread_cond_signal(&cond_writers);
    }
    pthread_mutex_unlock(&rw_mutex);
}

void WriteLock() {
    pthread_mutex_lock(&rw_mutex);
    waiting_writers++;
    while (readers > 0 || writers > 0) {
        pthread_cond_wait(&cond_writers, &rw_mutex);
    }
    waiting_writers--;
    writers++;
    pthread_mutex_unlock(&rw_mutex);
}

void WriteUnlock() {
    pthread_mutex_lock(&rw_mutex);
    writers--;
    if (waiting_writers > 0) {
        pthread_cond_signal(&cond_writers);
    } else {
        pthread_cond_broadcast(&cond_readers);
    }
    pthread_mutex_unlock(&rw_mutex);
}

void generate_test_data() {
    initial_values = malloc(n * sizeof(int));
    operations = malloc(m * sizeof(struct operation));
    
    for(int i = 0; i < n; i++) {
        initial_values[i] = rand() % MAX_KEY;
    }
    
    for(int i = 0; i < m; i++) {
        float op_choice = (float)rand() / RAND_MAX;
        if(op_choice < mMember) {
            operations[i].type = 0;  // Member
        } else if(op_choice < mMember + mInsert) {
            operations[i].type = 1;  // Insert
        } else {
            operations[i].type = 2;  // Delete
        }
        operations[i].value = rand() % MAX_KEY;
    }
}

struct node* create_initial_list() {
    struct node* head = NULL;
    for(int i = 0; i < n; i++) {
        Insert(&head, initial_values[i]);
    }
    return head;
}

double run_sequential() {
    struct node* head = create_initial_list();
    clock_t start = clock();
    
    for(int i = 0; i < m; i++) {
        switch(operations[i].type) {
            case 0:
                Member(head, operations[i].value);
                break;
            case 1:
                Insert(&head, operations[i].value);
                break;
            case 2:
                Delete(&head, operations[i].value);
                break;
        }
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    Clear_Memory(head);
    return time_taken;
}

struct thread_data {
    struct node** head;
    int prefer_readers;
    int thread_id;
    int ops_per_thread;
};

void* thread_function(void* arg) {
    struct thread_data* data = (struct thread_data*)arg;
    int start = data->thread_id * data->ops_per_thread;
    int end = start + data->ops_per_thread;
    
    for(int i = start; i < end; i++) {
        switch(operations[i].type) {
            case 0:
                ReadLock(data->prefer_readers);
                Member(*(data->head), operations[i].value);
                ReadUnlock();
                break;
            case 1:
                WriteLock();
                Insert(data->head, operations[i].value);
                WriteUnlock();
                break;
            case 2:
                WriteLock();
                Delete(data->head, operations[i].value);
                WriteUnlock();
                break;
        }
    }
    return NULL;
}

double run_concurrent(int num_threads, int prefer_readers) {
    struct node* head = create_initial_list();
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    struct thread_data* thread_args = malloc(num_threads * sizeof(struct thread_data));
    
    pthread_mutex_init(&rw_mutex, NULL);
    pthread_cond_init(&cond_readers, NULL);
    pthread_cond_init(&cond_writers, NULL);
    
    clock_t start = clock();
    
    int ops_per_thread = m / num_threads;
    for(int i = 0; i < num_threads; i++) {
        thread_args[i].head = &head;
        thread_args[i].prefer_readers = prefer_readers;
        thread_args[i].thread_id = i;
        thread_args[i].ops_per_thread = ops_per_thread;
        pthread_create(&threads[i], NULL, thread_function, &thread_args[i]);
    }
    
    for(int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    pthread_mutex_destroy(&rw_mutex);
    pthread_cond_destroy(&cond_readers);
    pthread_cond_destroy(&cond_writers);
    Clear_Memory(head);
    free(threads);
    free(thread_args);
    
    return time_taken;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Uso: %s <número de hilos> <preferencia: 0=escritores, 1=lectores>\n", argv[0]);
        return 1;
    }
    
    int num_threads = atoi(argv[1]);
    int prefer_readers = atoi(argv[2]);
    
    srand(time(NULL));
    generate_test_data();
    
    printf("Ejecutando pruebas con:\n");
    printf("- Lista inicial de %d elementos\n", n);
    printf("- %d operaciones totales\n", m);
    printf("- %.0f%% lecturas, %.0f%% inserciones, %.0f%% eliminaciones\n", 
           mMember*100, mInsert*100, mDelete*100);
    
    double sequential_time = run_sequential();
    printf("\nTiempo versión secuencial: %.6f segundos\n", sequential_time);
    
    double concurrent_time = run_concurrent(num_threads, prefer_readers);
    printf("Tiempo versión concurrente (%d hilos): %.6f segundos\n", 
           num_threads, concurrent_time);
    
    printf("Speedup: %.2fx\n", sequential_time/concurrent_time);
    
    free(initial_values);
    free(operations);
    
    return 0;
}