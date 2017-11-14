#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

/* Define this to use SpinLock implementation from pthread library.
 * If it's not defined, then our own atomics-based implementation
 * will be used.
 */
// #define USE_PTHREADS

/* Number of threads and number of iterations per thread.
 * Adjust this to have enough of iterations to:
 * (a) check whether lock is operating correct.
 * (b) have enough time to see the whole system to hung.
 */
#define NUM_THREADS 512
#define NUM_THREAD_ITERATION 1024

#ifdef USE_PTHREADS
#  include <pthread.h>
#endif

/* Spin Locks */

#if defined(__APPLE__)
typedef OSSpinLock;
#elif defined(_MSC_VER) && !defined(USE_PTHREADS)
typedef volatile int SpinLock;
#else
typedef pthread_spinlock_t SpinLock;
#endif

static void BLI_spin_init(SpinLock *spin)
{
#if defined(__APPLE__)
	*spin = OS_SPINLOCK_INIT;
#elif defined(_MSC_VER) && !defined(USE_PTHREADS)
	*spin = 0;
#else
	pthread_spin_init(spin, 0);
#endif
}

static void BLI_spin_lock(SpinLock *spin)
{
#if defined(__APPLE__)
	OSSpinLockLock(spin);
#elif defined(_MSC_VER) && !defined(USE_PTHREADS)
	while (InterlockedExchangeAcquire(spin, 1)) {
		while (*spin) {
			/* pass */
		}
	}
#else
	pthread_spin_lock(spin);
#endif
}

static void BLI_spin_unlock(SpinLock *spin)
{
#if defined(__APPLE__)
	OSSpinLockUnlock(spin);
#elif defined(_MSC_VER) && !defined(USE_PTHREADS)
	_ReadWriteBarrier();
	*spin = 0;
#else
	pthread_spin_unlock(spin);
#endif
}

static void BLI_spin_end(SpinLock *spin)
{
#if defined(__APPLE__)
#elif defined(_MSC_VER) && !defined(USE_PTHREADS)
#else
	pthread_spin_destroy(spin);
#endif
}

/* Thread pool */

typedef struct ThreadData {
	SpinLock* spin;
	int thread_id;
	int offset;
	size_t* global;
} ThreadData;

static DWORD WINAPI thread_function(LPVOID param)
{
	/* Don't start thread immediatelly. */
	Sleep(5);
	ThreadData* data = (ThreadData*)param;
	for (int i = 0; i < NUM_THREAD_ITERATION; ++i) {
		BLI_spin_lock(data->spin);
		*data->global += (size_t)data->thread_id;
		/* Loop to bias executin time of lock body itself. */
		for (int j = 0; j < rand() % 128; ++j) {
			*data->global += data->offset;
		}
		BLI_spin_unlock(data->spin);
	}
	return 0;
}

/* Main logic */

int  main(int argc, char** argv) {
	/* Global values to check correctness. */
	size_t global_value = 0;
	size_t expected_value = 0;
	/* Spin lock */
	SpinLock spin;
	BLI_spin_init(&spin);
	/* Thread handles anddata. */
	ThreadData thread_data_array[NUM_THREADS];
	DWORD thread_id_array[NUM_THREADS];
	HANDLE thread_array[NUM_THREADS];
	/* Create MAX_THREADS worker threads. */
	for (int i = 0; i < NUM_THREADS; ++i) {
		/* Generate unique data for each thread to work with. */
		thread_data_array[i].spin = &spin;
		thread_data_array[i].thread_id = i;
		thread_data_array[i].global = &global_value;
		thread_data_array[i].offset = 0;
		/* Update expected value. */
		expected_value += ((size_t)i) * NUM_THREAD_ITERATION;
		/* Create the thread to begin execution on its own. */
		thread_array[i] = CreateThread(NULL,
		                               0,
		                               thread_function,
		                               &thread_data_array[i],
		                               0,
		                               &thread_id_array[i]);
		if (thread_array[i] == NULL) {
			return EXIT_FAILURE;
		}
	}
	/* Close all thread handles. */
	for (int i = 0; i < NUM_THREADS; ++i) {
		/* Wait individual objects because WaitForMultipleObjects()
		 * will fail as long as we've got more than MAXIMUM_WAIT_OBJECTS
		 * threads.
		 */
		WaitForSingleObject(thread_array[i], INFINITE);
		CloseHandle(thread_array[i]);
	}
	/* Finish the spin. */
	BLI_spin_end(&spin);
	/* Output the value. */
	printf("Expected value: %Iu\n", expected_value);
	printf("Global value: %Iu\n", global_value);
	return EXIT_SUCCESS;
}
