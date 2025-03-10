#include "errno.h"
#include "task/task.h"


int __errno_location(void) {
    return task_get_errno();
}
