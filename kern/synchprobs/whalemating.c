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
 * Driver code is in kern/tests/synchprobs.c We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>
#include <cpu.h>
#include <current.h>

static bool is_male_ready_to_mating = false;
static bool is_female_ready_to_mating = false;
static struct cv *male_cv;
static struct cv *female_cv;
static struct lock *mating_lock;
static struct lock *matching_lock;

/*
 * Called by the driver during initialization.
 */

void whalemating_init()
{
	male_cv = cv_create("male_cv");
	female_cv = cv_create("female_cv");

	mating_lock = lock_create("mating_lock");
	matching_lock = lock_create("matching_lock");

	return;
}

/*
 * Called by the driver during teardown.
 */

void
whalemating_cleanup()
{
	lock_destroy(matching_lock);
	lock_destroy(mating_lock);

	cv_destroy(female_cv);
	cv_destroy(male_cv);

	return;
}

void
male(uint32_t index)
{
	male_start(index);

	lock_acquire(mating_lock);

	if (!is_male_ready_to_mating) {
		cv_wait(male_cv, mating_lock);
	}

	male_end(index);
	is_male_ready_to_mating = false;

	lock_release(mating_lock);
}

void
female(uint32_t index)
{
	female_start(index);

	lock_acquire(mating_lock);

	if (!is_female_ready_to_mating) {
		cv_wait(female_cv, mating_lock);
	}

	female_end(index);
	is_female_ready_to_mating = false;

	lock_release(mating_lock);
}

void
matchmaker(uint32_t index)
{
	matchmaker_start(index);

	lock_acquire(matching_lock);
	lock_acquire(mating_lock);

	matchmaker_end(index);
	is_female_ready_to_mating = true;
	is_male_ready_to_mating = true;

	cv_signal(male_cv, mating_lock);
	cv_signal(female_cv, mating_lock);
	lock_release(mating_lock);

	while(is_male_ready_to_mating || is_female_ready_to_mating) {
		thread_yield();
	}

	lock_release(matching_lock);
}
