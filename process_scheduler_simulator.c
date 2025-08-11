// Process Scheduler Simulator
// Demonstrates how CPU cores handle multiple processes through context switching

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_PROCESSES 10
#define MAX_CORES 4
#define TIME_QUANTUM_MS 10
#define SIMULATION_TIME_MS 200
#define CLEAR_SCREEN "\033[H\033[J"
#define MOVE_CURSOR(x,y) printf("\033[%d;%dH", (y), (x))

typedef enum {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_WAITING,
    STATE_TERMINATED
} ProcessState;

typedef struct {
    int pid;
    char name[16];
    ProcessState state;
    int remaining_time_ms;
    int total_time_ms;
    int wait_time_ms;
    int io_wait_ms;
    int assigned_core;
    int time_on_core_ms;
} Process;

typedef struct {
    int core_id;
    Process* current_process;
    int idle_time_ms;
    int busy_time_ms;
    char activity_history[SIMULATION_TIME_MS/TIME_QUANTUM_MS + 1];
    int history_index;
} CPUCore;

typedef struct {
    Process processes[MAX_PROCESSES];
    int process_count;
    Process* ready_queue[MAX_PROCESSES];
    int ready_queue_size;
    int ready_queue_front;
    Process* waiting_queue[MAX_PROCESSES];
    int waiting_queue_size;
} ProcessManager;

// Color codes for terminal output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

const char* process_colors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, 
    COLOR_MAGENTA, COLOR_CYAN, COLOR_RED, COLOR_GREEN,
    COLOR_YELLOW, COLOR_BLUE
};

void init_process(Process* p, int pid, const char* name, int total_time) {
    p->pid = pid;
    strncpy(p->name, name, 15);
    p->state = STATE_NEW;
    p->total_time_ms = total_time;
    p->remaining_time_ms = total_time;
    p->wait_time_ms = 0;
    p->io_wait_ms = 0;
    p->assigned_core = -1;
    p->time_on_core_ms = 0;
}

void init_core(CPUCore* core, int id) {
    core->core_id = id;
    core->current_process = NULL;
    core->idle_time_ms = 0;
    core->busy_time_ms = 0;
    core->history_index = 0;
    memset(core->activity_history, '.', sizeof(core->activity_history));
    core->activity_history[SIMULATION_TIME_MS/TIME_QUANTUM_MS] = '\0';
}

void enqueue_ready(ProcessManager* pm, Process* p) {
    if (pm->ready_queue_size < MAX_PROCESSES) {
        pm->ready_queue[pm->ready_queue_size++] = p;
        p->state = STATE_READY;
    }
}

Process* dequeue_ready(ProcessManager* pm) {
    if (pm->ready_queue_size == 0) return NULL;
    
    Process* p = pm->ready_queue[pm->ready_queue_front];
    
    // Shift queue
    for (int i = 0; i < pm->ready_queue_size - 1; i++) {
        pm->ready_queue[i] = pm->ready_queue[i + 1];
    }
    pm->ready_queue_size--;
    
    return p;
}

void print_header() {
    printf(CLEAR_SCREEN);
    printf("╔════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║               CPU PROCESS SCHEDULER SIMULATOR                              ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════╝\n\n");
}

void print_process_info(ProcessManager* pm) {
    printf("┌─────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ PROCESS STATUS                                                          │\n");
    printf("├─────┬──────────────┬───────────┬────────────┬──────────┬──────────────┤\n");
    printf("│ PID │ Name         │ State     │ Remaining  │ Wait     │ Core         │\n");
    printf("├─────┼──────────────┼───────────┼────────────┼──────────┼──────────────┤\n");
    
    for (int i = 0; i < pm->process_count; i++) {
        Process* p = &pm->processes[i];
        const char* state_str;
        const char* color;
        
        switch(p->state) {
            case STATE_NEW: state_str = "NEW      "; color = COLOR_CYAN; break;
            case STATE_READY: state_str = "READY    "; color = COLOR_YELLOW; break;
            case STATE_RUNNING: state_str = "RUNNING  "; color = COLOR_GREEN; break;
            case STATE_WAITING: state_str = "WAITING  "; color = COLOR_MAGENTA; break;
            case STATE_TERMINATED: state_str = "TERMINATED"; color = COLOR_RED; break;
        }
        
        printf("│ %s%3d%s │ %-12s │ %s%s%s │ %4d ms    │ %4d ms  │ ",
               process_colors[i % 10], p->pid, COLOR_RESET,
               p->name, color, state_str, COLOR_RESET,
               p->remaining_time_ms, p->wait_time_ms);
        
        if (p->assigned_core >= 0) {
            printf("Core %d        │\n", p->assigned_core);
        } else {
            printf("-             │\n");
        }
    }
    printf("└─────┴──────────────┴───────────┴────────────┴──────────┴──────────────┘\n\n");
}

void print_core_status(CPUCore cores[], int num_cores, int current_time) {
    printf("┌─────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ CPU CORE STATUS (Time: %d ms)                                          │\n", current_time);
    printf("├──────┬────────────────┬────────────┬────────────┬──────────────────────┤\n");
    printf("│ Core │ Current Task   │ Busy Time  │ Idle Time  │ Utilization          │\n");
    printf("├──────┼────────────────┼────────────┼────────────┼──────────────────────┤\n");
    
    for (int i = 0; i < num_cores; i++) {
        CPUCore* core = &cores[i];
        float utilization = 0;
        if (core->busy_time_ms + core->idle_time_ms > 0) {
            utilization = (float)core->busy_time_ms / (core->busy_time_ms + core->idle_time_ms) * 100;
        }
        
        printf("│ %2d   │ ", i);
        if (core->current_process) {
            printf("%s%-14s%s │", 
                   process_colors[core->current_process->pid % 10],
                   core->current_process->name,
                   COLOR_RESET);
        } else {
            printf("%-14s │", "IDLE");
        }
        printf(" %5d ms   │ %5d ms   │ %5.1f%%              │\n",
               core->busy_time_ms, core->idle_time_ms, utilization);
    }
    printf("└──────┴────────────────┴────────────┴────────────┴──────────────────────┘\n\n");
}

void print_timeline(CPUCore cores[], int num_cores, int time_slots) {
    printf("┌─────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ EXECUTION TIMELINE (Each slot = %d ms)                                │\n", TIME_QUANTUM_MS);
    printf("├─────────────────────────────────────────────────────────────────────────┤\n");
    
    for (int i = 0; i < num_cores; i++) {
        printf("│ Core %d: ", i);
        
        // Print the activity history with colors
        for (int j = 0; j < cores[i].history_index && j < 60; j++) {
            if (cores[i].activity_history[j] == '.') {
                printf(".");
            } else {
                int proc_num = cores[i].activity_history[j] - '0';
                printf("%s%c%s", process_colors[proc_num % 10], 
                       cores[i].activity_history[j], COLOR_RESET);
            }
        }
        
        // Fill remaining space
        for (int j = cores[i].history_index; j < 60; j++) {
            printf(" ");
        }
        printf(" │\n");
    }
    printf("└─────────────────────────────────────────────────────────────────────────┘\n\n");
}

void print_queue_status(ProcessManager* pm) {
    printf("┌─────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ QUEUE STATUS                                                            │\n");
    printf("├─────────────────────────────────────────────────────────────────────────┤\n");
    printf("│ Ready Queue (%d): ", pm->ready_queue_size);
    
    for (int i = 0; i < pm->ready_queue_size; i++) {
        printf("[%sP%d%s] ", 
               process_colors[pm->ready_queue[i]->pid % 10],
               pm->ready_queue[i]->pid,
               COLOR_RESET);
    }
    
    // Fill remaining space
    for (int i = pm->ready_queue_size; i < 10; i++) {
        printf("     ");
    }
    printf("         │\n");
    
    printf("│ Waiting Queue (%d): ", pm->waiting_queue_size);
    for (int i = 0; i < pm->waiting_queue_size; i++) {
        printf("[%sP%d%s] ", 
               process_colors[pm->waiting_queue[i]->pid % 10],
               pm->waiting_queue[i]->pid,
               COLOR_RESET);
    }
    
    // Fill remaining space
    for (int i = pm->waiting_queue_size; i < 10; i++) {
        printf("     ");
    }
    printf("         │\n");
    printf("└─────────────────────────────────────────────────────────────────────────┘\n\n");
}

void simulate_scheduling() {
    ProcessManager pm = {0};
    CPUCore cores[MAX_CORES];
    
    // Initialize processes with different workloads
    init_process(&pm.processes[0], 0, "WebBrowser", 80);
    init_process(&pm.processes[1], 1, "TextEditor", 60);
    init_process(&pm.processes[2], 2, "Compiler", 100);
    init_process(&pm.processes[3], 3, "MusicPlayer", 120);
    init_process(&pm.processes[4], 4, "FileManager", 40);
    init_process(&pm.processes[5], 5, "Terminal", 50);
    init_process(&pm.processes[6], 6, "Calculator", 30);
    init_process(&pm.processes[7], 7, "VideoPlayer", 90);
    pm.process_count = 8;
    
    // Initialize CPU cores
    for (int i = 0; i < MAX_CORES; i++) {
        init_core(&cores[i], i);
    }
    
    // Add all processes to ready queue
    for (int i = 0; i < pm.process_count; i++) {
        enqueue_ready(&pm, &pm.processes[i]);
    }
    
    // Simulation loop
    int current_time = 0;
    int time_slots = 0;
    
    while (current_time < SIMULATION_TIME_MS) {
        // Schedule processes on available cores
        for (int i = 0; i < MAX_CORES; i++) {
            CPUCore* core = &cores[i];
            
            // Check if current process finished its quantum or is done
            if (core->current_process) {
                core->current_process->time_on_core_ms += TIME_QUANTUM_MS;
                core->current_process->remaining_time_ms -= TIME_QUANTUM_MS;
                
                if (core->current_process->remaining_time_ms <= 0) {
                    // Process completed
                    core->current_process->state = STATE_TERMINATED;
                    core->current_process->remaining_time_ms = 0;
                    core->current_process->assigned_core = -1;
                    core->current_process = NULL;
                } else if (core->current_process->time_on_core_ms >= TIME_QUANTUM_MS) {
                    // Time quantum expired, preempt
                    core->current_process->assigned_core = -1;
                    core->current_process->time_on_core_ms = 0;
                    
                    // Simulate I/O wait (30% chance)
                    if (rand() % 100 < 30) {
                        core->current_process->state = STATE_WAITING;
                        core->current_process->io_wait_ms = 20 + rand() % 30;
                        pm.waiting_queue[pm.waiting_queue_size++] = core->current_process;
                    } else {
                        enqueue_ready(&pm, core->current_process);
                    }
                    core->current_process = NULL;
                }
            }
            
            // Assign new process if core is idle
            if (!core->current_process && pm.ready_queue_size > 0) {
                Process* next = dequeue_ready(&pm);
                if (next && next->state != STATE_TERMINATED) {
                    core->current_process = next;
                    next->state = STATE_RUNNING;
                    next->assigned_core = i;
                    next->time_on_core_ms = 0;
                    core->busy_time_ms += TIME_QUANTUM_MS;
                    
                    // Record in history
                    if (core->history_index < SIMULATION_TIME_MS/TIME_QUANTUM_MS) {
                        core->activity_history[core->history_index++] = '0' + next->pid;
                    }
                }
            } else if (!core->current_process) {
                core->idle_time_ms += TIME_QUANTUM_MS;
                if (core->history_index < SIMULATION_TIME_MS/TIME_QUANTUM_MS) {
                    core->activity_history[core->history_index++] = '.';
                }
            } else {
                // Core is busy
                if (core->history_index < SIMULATION_TIME_MS/TIME_QUANTUM_MS) {
                    core->activity_history[core->history_index++] = '0' + core->current_process->pid;
                }
            }
        }
        
        // Update waiting processes
        for (int i = 0; i < pm.waiting_queue_size; i++) {
            Process* p = pm.waiting_queue[i];
            p->io_wait_ms -= TIME_QUANTUM_MS;
            if (p->io_wait_ms <= 0) {
                // I/O complete, move back to ready queue
                enqueue_ready(&pm, p);
                // Remove from waiting queue
                for (int j = i; j < pm.waiting_queue_size - 1; j++) {
                    pm.waiting_queue[j] = pm.waiting_queue[j + 1];
                }
                pm.waiting_queue_size--;
                i--;
            }
        }
        
        // Update wait times for ready processes
        for (int i = 0; i < pm.ready_queue_size; i++) {
            pm.ready_queue[i]->wait_time_ms += TIME_QUANTUM_MS;
        }
        
        // Print current state
        print_header();
        print_process_info(&pm);
        print_core_status(cores, MAX_CORES, current_time);
        print_timeline(cores, MAX_CORES, time_slots);
        print_queue_status(&pm);
        
        printf("Press ENTER to advance time (or Ctrl+C to exit)...");
        getchar();
        
        current_time += TIME_QUANTUM_MS;
        time_slots++;
    }
    
    // Final statistics
    printf("\n╔════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          SIMULATION COMPLETE                               ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Final Statistics:\n");
    printf("─────────────────\n");
    for (int i = 0; i < MAX_CORES; i++) {
        float utilization = (float)cores[i].busy_time_ms / (cores[i].busy_time_ms + cores[i].idle_time_ms) * 100;
        printf("Core %d: %.1f%% utilization\n", i, utilization);
    }
    
    printf("\nProcess Statistics:\n");
    printf("──────────────────\n");
    int total_wait = 0, completed = 0;
    for (int i = 0; i < pm.process_count; i++) {
        Process* p = &pm.processes[i];
        total_wait += p->wait_time_ms;
        if (p->state == STATE_TERMINATED) completed++;
        printf("P%d (%s): Wait time = %d ms, ", p->pid, p->name, p->wait_time_ms);
        if (p->state == STATE_TERMINATED) {
            printf("Completed\n");
        } else {
            printf("%d ms remaining\n", p->remaining_time_ms);
        }
    }
    
    printf("\nAverage wait time: %.1f ms\n", (float)total_wait / pm.process_count);
    printf("Processes completed: %d/%d\n", completed, pm.process_count);
}

// Demonstration of single-core vs multi-core execution
void demonstrate_parallelism() {
    printf("\n╔════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║              PARALLELISM vs CONCURRENCY DEMONSTRATION                      ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("CONCURRENCY (Single Core) - Time-sliced execution:\n");
    printf("──────────────────────────────────────────────────\n");
    printf("Time:    0ms    10ms   20ms   30ms   40ms   50ms   60ms   70ms\n");
    printf("Core 0:  [P1]   [P2]   [P3]   [P1]   [P2]   [P3]   [P1]   [P2]\n");
    printf("         ↑      ↑      ↑      ↑      ↑      ↑      ↑      ↑\n");
    printf("         └──────┴──────┴──────┴──────┴──────┴──────┴──────┘\n");
    printf("                     Context switches every 10ms\n\n");
    
    printf("Total time for 3 processes (30ms each): 90ms\n");
    printf("Actual parallelism: 1 (only one process runs at a time)\n\n");
    
    printf("PARALLELISM (4 Cores) - True parallel execution:\n");
    printf("────────────────────────────────────────────────\n");
    printf("Time:    0ms    10ms   20ms   30ms   40ms   50ms   60ms   70ms\n");
    printf("Core 0:  [─────────── P1 ──────────]\n");
    printf("Core 1:  [─────────── P2 ──────────]\n");
    printf("Core 2:  [─────────── P3 ──────────]\n");
    printf("Core 3:  [idle][─────── P4 ────────]\n\n");
    
    printf("Total time for 3 processes (30ms each): 30ms\n");
    printf("Actual parallelism: 3 (three processes run simultaneously)\n");
    printf("Speedup: 3x faster than single core!\n\n");
    
    printf("KEY INSIGHTS:\n");
    printf("────────────\n");
    printf("• Concurrency: Multiple processes making progress (interleaved)\n");
    printf("• Parallelism: Multiple processes executing simultaneously\n");
    printf("• Single core can only provide concurrency through time-slicing\n");
    printf("• Multiple cores provide true parallelism\n");
    printf("• Hyperthreading allows 2 threads per core (logical parallelism)\n\n");
}

int main() {
    srand(time(NULL));
    
    printf("Select demonstration:\n");
    printf("1. Interactive Process Scheduler Simulation\n");
    printf("2. Parallelism vs Concurrency Comparison\n");
    printf("Choice: ");
    
    int choice;
    scanf("%d", &choice);
    getchar(); // Consume newline
    
    if (choice == 1) {
        simulate_scheduling();
    } else {
        demonstrate_parallelism();
    }
    
    return 0;
}