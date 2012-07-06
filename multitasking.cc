#include "multitasking.h"
#include <iostream>
#include <cassert>
#include <ucontext.h>
#include <vector>
#include <cstdlib>


/** @warning this code runs only on machines where sizeof(void*) == 2 * sizeof(int) */

namespace mt
{

static std::vector<ucontext_t*> threads;
static ucontext_t               scheduler_context;
static size_t                   current_thread = -1; // TODO: volatile? would it matter in case of context switches? atomic?

void destroy_task(size_t index);

size_t pick_next_task(size_t hint)
{
	assert(!threads.empty());

	/* STRATEGY: round robin */
	/*
	size_t next = hint;
	if (hint == threads.size())
	{
		next = 0;
	}
	*/

	/* STRATEGY: random */
	size_t next = std::rand()%threads.size();

	return next;
}

void switch_from_to(ucontext_t* now, size_t next)
{
	// std::cerr << "swap from " << current_thread << " to " << next << std::endl;

	current_thread = next;
	swapcontext(now, threads[next]);
}

/** Called in a task's context.
 * Should switch to a different task. May switch to the same one, if there are
 * no more left. */
void yield()
{
	switch_from_to(threads[current_thread],
		       pick_next_task(current_thread+1));
}

/** Wrap function in a try-catch and execute it.
 * This is an entry point of each task. Same as with threads, we really don't
 * want any exception to exit this scope, this would crash because beyond this
 * function, there is only C code that transfers execution back to a different
 * context. Nobody there knows about exceptions.*/
void run_thread(int func_as_int_low, int func_as_int_high)
{
	// bleeagh, vomit. i wish there would be one good old void*
	// parameter...
	typedef void (*fp)();
	int buffer[2] = {func_as_int_low, func_as_int_high};
	assert(sizeof(buffer)==sizeof(fp));

	void (*func)() = reinterpret_cast<fp>(*reinterpret_cast<fp*>(buffer));
	try
	{
		func();
	}
	catch(...)
	{
		std::cerr << "C'mon, don't let any exception reach this "
			     "place..." << std::endl;
	}
}

void add_task(void (*func)())
{
	const size_t stack_size = 163840;

	ucontext_t* ctx = new ucontext_t;
	char *stack     = new char[stack_size]; // fixme: exception-safety

	getcontext(ctx);

	ctx->uc_stack.ss_sp   = stack;
	ctx->uc_stack.ss_size = stack_size;

	// when thread finishes, pass control back to scheduler
	ctx->uc_link = &scheduler_context;

	// hack, hack, hack, trala la, strict aliasing, portability fixme
	typedef void (*fp)();
	int buffer[2] = {0, 0};
	assert(sizeof(buffer)==sizeof(fp));
	buffer[0] = reinterpret_cast<int*>(&func)[0];
	buffer[1] = reinterpret_cast<int*>(&func)[1];

	// I'm a crazy spell caster, stay away or I'll kill you.
	makecontext(ctx, reinterpret_cast<void (*)()>(run_thread), 2, buffer[0], buffer[1]); // fixme: sizeof(void (*)()) != sizeof(int)
	threads.push_back(ctx);
}

void destroy_task(size_t index)
{
	ucontext* ctx = threads[index];
	delete static_cast<char*>(ctx->uc_stack.ss_sp);
	delete ctx;

	threads.erase(threads.begin()+index);
}

void run_scheduler()
{
	// first thread enters here and saves scheduler_context.
	//
	// every task is configured to go back to scheduler_context when it
	// finishes, so when switch_from_to returns, we know that most recently
	// run task finished and we can destroy it.
	//
	// then we try to switch to the next one, or return if all are done.
	//
	// NOTE: the key to make destructors and exceptions work is to never
	// loose any context, i.e. execute all of them until completion

	assert(!threads.empty());
	switch_from_to(&scheduler_context, 0);

	while (true)
	{
		destroy_task(current_thread);

		if (threads.empty())
		{
			break;
		}

		switch_from_to(&scheduler_context,
			       pick_next_task(current_thread));
	}

	// std::cerr << "run scheduler exit" << std::endl;
}


void execute(void (*entry_point)())
{
	add_task(entry_point);
	run_scheduler();
}

}
