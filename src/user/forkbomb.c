/*
 * Project 1 Test: forkbomb - Process Resource Exhaustion
 *
 * Tests system behavior under rapid process creation (fork bomb pattern).
 * Each process forks up to 10 times, creating exponential process growth.
 *
 * Test sequence:
 * 1. Process forks repeatedly in a loop (up to 10 times per process)
 * 2. Each successful fork prints parent and child PIDs
 * 3. Loop terminates when fork fails (rc < 0) or count exhausted
 * 4. Each process prints goodbye message before exiting
 *
 * Expected behavior:
 * - Many "forked" messages showing process tree growth
 * - Eventually fork fails due to resource exhaustion (memory/process table)
 * - All processes print "so long from <pid>" and exit cleanly
 * - System should recover after all processes exit (no resource leaks)
 */
#include <conio.h>
#include <process.h>
#include <sched.h>
#include <sema.h>
#include <string.h>
#include <fileio.h>

int main(int argc, char **argv) {

  int rc,j=10;
    do {
        rc = Fork();
        if (rc > 0) {
            Print(" %d forked %d\n", Get_PID(), rc);
        }
    } while (rc >= 0 && --j > 0);
    Print(" so long from %d\n", Get_PID());
    
    return 0;
}
