#include "IOUtil.hpp"

#include <stdio.h>
#include <unistd.h>

namespace Glacius {
namespace IOUtil {

void disableStdoutBuffering()
{
    setvbuf(stdout, NULL, _IONBF, 0);
}

bool isStdoutATty()
{
    return isatty(fileno(stdin));
}

void waitForSignal()
{
    pause();
}

}
}
