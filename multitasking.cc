#include "multitasking.h"
#include <iostream>
#include <cassert>
#include <ucontext.h>
#include <vector>

namespace mt
{

static std::vector<ucontext_t*> threads;
static ucontext_t               scheduler_context;
static ucontext_t               go_context;
static size_t                   current_thread = 0;

void activate_next_task(ucontext_t* now, size_t next)
{
	if (next == threads.size())
	{
		next = 0;
	}

	//std::cerr << "swap from " << current_thread << " to " << next << std::endl;
	current_thread = next;
	swapcontext(now, threads[next]);
}

void yield()
{
	assert(!threads.empty());
	ucontext_t* now = threads[current_thread];

	size_t next = current_thread+1;
	activate_next_task(now, next);
}

/** Called in scheduler's context when a task finishes.
 * Remove current thread from pool and run next one if possible */
bool run_next()
{
	threads.erase(threads.begin()+current_thread);
	if (threads.empty())
	{
		return false;
	}

	ucontext_t* now = &scheduler_context;

	size_t next = current_thread; // element at that index just got erased, so it now points to next
	activate_next_task(now, next);

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

void run_scheduler()
{
	// first thread enters here and saves scheduler_context.
	//
	// when each thread finihes, it goes back to scheduler_context, i.e.
	// here. then it call run_next, which either does nothing if it was the
	// last task and in such case we return, or it switches to another task.
	swapcontext(&scheduler_context, threads[0]);
	while (run_next());

	std::cerr << "run scheduler exit" << std::endl;
}


void execute(void (*entry_point)())
{
	add_task(entry_point);
	run_scheduler();
}

}
