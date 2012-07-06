#include "multitasking.h"
#include <iostream>
#include <cassert>
#include <ucontext.h>
#include <vector>
#include <cstdlib>

namespace mt
{

static std::vector<ucontext_t*> threads;
static ucontext_t               scheduler_context;
static size_t                   current_thread = -1;

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
	std::cerr << "swap from " << current_thread << " to " << next << std::endl;

	// FIXME: this assumes swapcontext is called immediatelly, which is not
	// nice.
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

/** Called in scheduler's context when a task finishes.
 * Remove current thread from pool and run next one if possible */
bool run_next()
{
	destroy_task(current_thread);

	if (threads.empty())
	{
		return false;
	}

	switch_from_to(&scheduler_context,
		       pick_next_task(current_thread));

	return true;
}

void run_thread(int func_as_int_low, int func_as_int_high)
{
	// bleeagh, vomit.
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
		std::cerr << "catch @run_thread" << std::endl;
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
	// when each thread finihes, it goes back to scheduler_context, i.e.
	// here. then it call run_next, which either does nothing if it was the
	// last task and in such case we return, or it switches to another task.
	switch_from_to(&scheduler_context, 0);
	while (run_next());

	std::cerr << "run scheduler exit" << std::endl;
}


void execute(void (*entry_point)())
{
	add_task(entry_point);
	run_scheduler();
}

}
