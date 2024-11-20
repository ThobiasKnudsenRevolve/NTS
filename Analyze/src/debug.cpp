#include "../include/debug.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    void* ptr;
    size_t size_bytes;
    size_t line;
    char* file;
} alloc_tracking_t;
typedef struct {
    alloc_tracking_t* all_allocs;
    size_t            all_allocs_size;
    size_t            all_allocs_count;
    unsigned int      start_time_ms;
    char*             code_location;
    size_t            code_location_size;
} debug_data_t;
static debug_data_t debug_data;
void ExitFunction() {
	if (debug_data.all_allocs_count == 0) {
		return;
	}
	printf("\nMEMORY NOT FREED:\n");
	for (unsigned int i = 0; i < debug_data.all_allocs_count; i++) {
		printf("	%p %zu bytes | allocated at%s:%zu\n", 	debug_data.all_allocs[i].ptr,
									debug_data.all_allocs[i].size_bytes,
									debug_data.all_allocs[i].file,
									debug_data.all_allocs[i].line);
		free(debug_data.all_allocs[i].ptr);
	}
}
void CrashFunction(int sig, siginfo_t *info, void *context) {
    size_t buffer_size = 2048;
    char buffer[buffer_size];
    int offset = 0;
    int temp;
    // Construct the initial error message
    temp = snprintf(buffer + offset, buffer_size - offset,
                    "\nERROR program crashed with signal %d in %s\n",
                    sig, debug_data.code_location);
    if (temp > 0 && (unsigned int)temp < buffer_size - offset) {
        offset += temp;
    }
    if (debug_data.all_allocs_count == 0) {
        write(STDERR_FILENO, buffer, offset);  // Output the message
        _exit(1);
    }
    // Additional message about freeing memory
    temp = snprintf(buffer + offset, buffer_size - offset, "\nfreeing memory\n");
    if (temp > 0 && (unsigned int)temp < buffer_size - offset) {
        offset += temp;
    }
    // Iterate over all allocations
    for (unsigned int i = 0; i < debug_data.all_allocs_count; i++) {
        temp = snprintf(buffer + offset, buffer_size - offset,
                        "    address %p | %zu bytes | at %s:%zu\n",
                        debug_data.all_allocs[i].ptr,
                        debug_data.all_allocs[i].size_bytes,
                        debug_data.all_allocs[i].file,
                        debug_data.all_allocs[i].line);
        if (temp > 0 && (unsigned int)temp < buffer_size - offset) {
            offset += temp;
        }

        free(debug_data.all_allocs[i].ptr);  // This is still not safe in a SIGSEGV handler
    }
    // Write the entire message at once
    write(STDERR_FILENO, buffer, offset);
    _exit(1);  // Terminate the program immediately
}
__attribute__((constructor))
void DebugDataInit() {
    debug_data.all_allocs = (alloc_tracking_t*)malloc(256*sizeof(alloc_tracking_t));
	if (debug_data.all_allocs == NULL) {
		printf("ERROR | debug_data.all_allocs = malloc(2048*sizeof(debug_data_t));\n");
		exit(EXIT_FAILURE);
	}

    debug_data.all_allocs_size = 256;
    debug_data.all_allocs_count = 0;
    debug_data.start_time_ms = (unsigned int)(clock() * 1000 / CLOCKS_PER_SEC);

    debug_data.code_location = (char*)malloc(2048 * sizeof(char));
    if (debug_data.code_location == NULL) {
		printf("ERROR | debug_data.code_location = malloc(2048 * sizeof(char));\n");
		exit(EXIT_FAILURE);
    }

	memset(debug_data.code_location, 0, 2048 * sizeof(char));
    debug_data.code_location_size = 2048;

	atexit(ExitFunction);

	struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = CrashFunction;

    sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);

}
void DebugPrintf(const char* message, size_t line, const char* file) {
	unsigned int current_time = (unsigned int)(clock() * 1000 / CLOCKS_PER_SEC);
	printf("%dms %s %s:%ld | %s", current_time-debug_data.start_time_ms, debug_data.code_location, file, line, message);
}
void DebugStart(size_t line, const char* file) {

	if (!file) {
		printf("ERROR | !file\n");
		exit(EXIT_FAILURE);
	}

    char adding_string[50];
    snprintf(adding_string, sizeof(adding_string), " %s:%ld", file, line);
    size_t current_length = strlen(debug_data.code_location);
    size_t needed_size = current_length + strlen(adding_string) + 1;

    if (debug_data.code_location_size < needed_size) {

        while (debug_data.code_location_size < needed_size) {
            debug_data.code_location_size *= 2;
        }

        void* tmp = realloc(debug_data.code_location, debug_data.code_location_size);
        if (!tmp) {
            printf("ERROR | void* tmp = realloc(debug_data.code_location, debug_data.code_location_size);\n");
            exit(EXIT_FAILURE);
        }

        debug_data.code_location = (char*)tmp;
    }

    strcat(debug_data.code_location + strlen(debug_data.code_location), adding_string);
}
void DebugEnd() {
	int index = -1;
	for (int i = debug_data.code_location_size-1; i >= 0 ; i--) {
		if (debug_data.code_location[i]==' ') {
			index = i;
			break;
		}
	}
	if (index==-1) {
		printf("ERROR | index==-1");
		exit(EXIT_FAILURE);
	}
	debug_data.code_location[index] = '\0';
}
size_t DebugGetSizeBytes(void* ptr) {
	if (!ptr) {
		return 0;
	}
	int index = -1;
	for (unsigned int i = 0; i < debug_data.all_allocs_count; i++) {
		if (debug_data.all_allocs[i].ptr == ptr) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		return 0;
	}
	return debug_data.all_allocs[index].size_bytes;
}
void DebugPrintMemory() {
	printf("\nunfreed memory\n");
	for (unsigned int i = 0; i < debug_data.all_allocs_count; i++) {
		printf("	address %p | %zu bytes | at %s:%zu\n",  debug_data.all_allocs[i].ptr,
															debug_data.all_allocs[i].size_bytes,
															debug_data.all_allocs[i].file,
															debug_data.all_allocs[i].line);
	}
}
