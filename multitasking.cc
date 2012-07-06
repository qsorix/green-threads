#include "multitasking.h"
#include <iostream>
#include <cassert>
#include <ucontext.h>
#include <vector>

namespace mt
{

static std::vector<ucontext_t*> threads;
static ucontext_t               switching_context;
static ucontext_t               go_context;
static size_t                   current_thread = 0;

void yield()
{
	size_t next;
	if (current_thread+1 == threads.size())
	{
		next = 0;
	}
	else
	{
		next = current_thread+1;
	}
	//std::cerr << "swap from " << current_thread << " to " << next << std::endl;
	size_t now = current_thread;
	current_thread = next;
	swapcontext(threads[now], threads[next]);
}

void run_next()
{
	threads.erase(threads.begin()+current_thread);
	if (threads.empty())
	{
		//std::cerr << "run next is done, exiting" << std::endl;
		return;
	}
	if (current_thread == threads.size())
	{
		current_thread = 0;
	}
	ucontext_t* nc = threads[current_thread];
	setcontext(nc);
}

void run_thread(int func_as_int_low, int func_as_int_high)
{
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

void create_task(void (*func)())
{
	const size_t stack_size = 163840;

	ucontext_t* ctx = new ucontext_t;
	char *stack     = new char[stack_size]; // fixme: exception-safety

	getcontext(ctx);

	ctx->uc_stack.ss_sp   = stack;
	ctx->uc_stack.ss_size = stack_size;

	// when thread finishes, pass control back to scheduler
	ctx->uc_link = &switching_context; // fixme: rename switching_context to 'scheduler_context'

	// hack, hack, hack, trala la, strict aliasing, portability fixme
	typedef void (*fp)();
	int buffer[2] = {0, 0};
	assert(sizeof(buffer)==sizeof(fp));
	buffer[0] = reinterpret_cast<int*>(&func)[0];
	buffer[1] = reinterpret_cast<int*>(&func)[1];

	makecontext(ctx, reinterpret_cast<void (*)()>(run_thread), 2, buffer[0], buffer[1]); // fixme: sizeof(void (*)()) != sizeof(int)
	threads.push_back(ctx);
}

void add_task(void (*func)())
{
	create_task(func);
}

bool store_switching_context()
{
	volatile bool first = true;
	getcontext(&switching_context);
	if (first)
	{
		first = false;
		swapcontext(&go_context, threads[0]);
		std::cerr << "go context returns" << std::endl;
		return true;
	}
	else
	{
		//std::cerr << "thread finished" << std::endl;
		run_next();
	}
	swapcontext(&switching_context, &go_context);
	std::cerr << "store_switching_context returns" << std::endl;
	return false;
}

void run_scheduler()
{
	store_switching_context();
	std::cerr << "run scheduler exit" << std::endl;
}


void execute(void (*entry_point)())
{
	create_task(entry_point);
	run_scheduler();
}

}
