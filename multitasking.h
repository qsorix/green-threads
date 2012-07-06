#ifndef MULTITASKING_990620D8_A579_4469_8A49_36CCF68EF4FA_H
#define MULTITASKING_990620D8_A579_4469_8A49_36CCF68EF4FA_H

namespace mt
{

typedef void (*entry_point)();

/** Run given function and wait for completion.
 * Yes, this blocks until *all* tasks finish.
 */
void execute(entry_point);

/** Switch to next task */
void yield();

/** Add a new task to the set */
void add_task(entry_point);


}

#endif
