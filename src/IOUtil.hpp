#ifndef GLACIUS_IOUTIL_HPP
#define GLACIUS_IOUTIL_HPP

namespace Glacius {
namespace IOUtil {

void disableStdoutBuffering();
bool isStdoutATty();
void waitForSignal();

}
}

#endif
