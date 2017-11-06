/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code is in kern/tests/synchprobs.c We will replace that file. This
 * file is yours to modify as you see fit.
 *
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is, of
 * course, stable under rotation)
 *
 *   |0 |
 * --    --
 *    01  1
 * 3  32
 * --    --
 *   | 2|
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X first.
 * The semantics of the problem are that once a car enters any quadrant it has
 * to be somewhere in the intersection until it call leaveIntersection(),
 * which it should call while in the final quadrant.
 *
 * As an example, let's say a car approaches the intersection and needs to
 * pass through quadrants 0, 3 and 2. Once you call inQuadrant(0), the car is
 * considered in quadrant 0 until you call inQuadrant(3). After you call
 * inQuadrant(2), the car is considered in quadrant 2 until you call
 * leaveIntersection().
 *
 * You will probably want to write some helper functions to assist with the
 * mappings. Modular arithmetic can help, e.g. a car passing straight through
 * the intersection entering from direction X will leave to direction (X + 2)
 * % 4 and pass through quadrants X and (X + 3) % 4.  Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in synchprobs.c to record their progress.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * Called by the driver during initialization.
*/
#define NUM_OF_QUADRANTS 4
static struct cv *stop_cv;
static uint32_t num_of_cars_in_quadrants[NUM_OF_QUADRANTS] = {0,0,0,0};
static struct lock *quadrants_locks[NUM_OF_QUADRANTS];
char name[32];

void
stoplight_init()
{
	stop_cv = cv_create("stop_cv");

	for(int i = 0; i < NUM_OF_QUADRANTS; ++i)
	{
		snprintf(name, sizeof(name), "Lock of quardant %d", i);
		quadrants_locks[i] = lock_create(name);
	}

	return;
}

/*
 * Called by the driver during teardown.
 */

void
stoplight_cleanup()
{
	for(int i = 0; i < NUM_OF_QUADRANTS; ++i)
	{
		lock_destroy(quadrants_locks[i]);
	}

	cv_destroy(stop_cv);

	return;
}


// It returns new current quadrant
uint32_t
one_turn(uint32_t pred_quadrant, uint32_t index)
{
	uint32_t current_quadrant = get_next_quadrant(pred_quadrant);
	lock_acquire(quadrants_locks[current_quadrant]);
	num_of_cars_in_quadrants[current_quadrant]++;
	inQuadrant(current_quadrant, index);

	clean_quadrant(pred_quadrant);

	return current_quadrant;
}


uint32_t
get_next_quadrant(uint32_t current_quadrant) {
	return (current_quadrant + NUM_OF_QUADRANTS - 1) % NUM_OF_QUADRANTS;
}

void
clean_quadrant(uint32_t quadrant)
{
	num_of_cars_in_quadrants[quadrant]--;

	// I do not know what's best, to write without condition or with it.
	// if ((num_of_cars_in_quadrants[0] + num_of_cars_in_quadrants[1] +
		// num_of_cars_in_quadrants[2] + num_of_cars_in_quadrants[3]) == 2)
		// {
			// cv_broadcast(stop_cv, quadrants_locks[current_quadrant]);
		// }
	cv_signal(stop_cv, quadrants_locks[quadrant]);
	lock_release(quadrants_locks[quadrant]);
}

void
turnright(uint32_t direction, uint32_t index)
{
	uint32_t current_quadrant = direction;
	lock_acquire(quadrants_locks[current_quadrant]);

	// if there are already three cars at the intersection, we are waiting to avoid deadlock
	while ((num_of_cars_in_quadrants[0] + num_of_cars_in_quadrants[1] +
			num_of_cars_in_quadrants[2] + num_of_cars_in_quadrants[3]) == 3)
			{
				cv_wait(stop_cv, quadrants_locks[current_quadrant]);
			}

	num_of_cars_in_quadrants[current_quadrant]++;
	inQuadrant(current_quadrant, index);

	leaveIntersection(index);
	clean_quadrant(current_quadrant);

	return;
}

void
gostraight(uint32_t direction, uint32_t index)
{
	uint32_t current_quadrant = direction;
	lock_acquire(quadrants_locks[current_quadrant]);

	while ((num_of_cars_in_quadrants[0] + num_of_cars_in_quadrants[1] +
			num_of_cars_in_quadrants[2] + num_of_cars_in_quadrants[3]) == 3)
			{
				cv_wait(stop_cv, quadrants_locks[current_quadrant]);
			}

	num_of_cars_in_quadrants[current_quadrant]++;
	inQuadrant(current_quadrant, index);

	current_quadrant = one_turn(current_quadrant, index);

	leaveIntersection(index);
	clean_quadrant(current_quadrant);

	return;
}
void
turnleft(uint32_t direction, uint32_t index)
{
	uint32_t current_quadrant = direction;
	lock_acquire(quadrants_locks[current_quadrant]);

	while ((num_of_cars_in_quadrants[0] + num_of_cars_in_quadrants[1] +
			num_of_cars_in_quadrants[2] + num_of_cars_in_quadrants[3]) == 3)
			{
				cv_wait(stop_cv, quadrants_locks[current_quadrant]);
			}

	num_of_cars_in_quadrants[current_quadrant]++;
	inQuadrant(current_quadrant, index);

	for (int i = 0; i < 2; ++i)
	{
		current_quadrant = one_turn(current_quadrant, index);
	}

	leaveIntersection(index);
	clean_quadrant(current_quadrant);

	return;
}
