#include <iostream>
#include <stdexcept>
#include "multitasking.h"
#include <cstdlib>

struct Foo
{
	Foo()
	{
		std::cerr << "Foo" << std::endl;
	}

	~Foo()
	{
		std::cerr << "~Foo" << std::endl;
	}
};

struct Bar
{
	Bar()
	{
		std::cerr << "Bar" << std::endl;
	}

	~Bar()
	{
		std::cerr << "~Bar" << std::endl;
	}
};

void loop3()
{
	int i=7;
	while(i--)
	{
		std::cout << "loop3 " << i << std::endl;
		mt::yield();
	}
}


void loop1()
{
	Bar a;
	int i=10;
	while(i--)
	{
		Bar b;
		std::cout << "loop1 " << i << std::endl;
		mt::yield();
		if (i==2)
		{
			mt::add_task(loop3);
			mt::yield();
		}
	}
	std::cout << "loop1 return" << std::endl;
}

void loop2()
{
	int i=5;
	Foo a;
	while(i--)
	{
		Foo b;
		if (i==3)
			throw std::runtime_error("aaa");
		std::cout << "loop2 " << i << std::endl;
		mt::yield();
	}

	std::cout << "loop2 return" << std::endl;
}

void threaded_main()
{
	mt::add_task(loop1);
	mt::add_task(loop2);
	mt::add_task(loop2);
	std::cerr << "threaded_main returns" << std::endl;
}

int main()
{
	std::srand(time(NULL));
	try
	{
		mt::execute(threaded_main);
		std::cerr << " -----------------" << std::endl;
		mt::execute(threaded_main);
		std::cerr << "main returns" << std::endl;
	}
	catch(...)
	{
		std::cerr << "catch @main" << std::endl;
	}
}
