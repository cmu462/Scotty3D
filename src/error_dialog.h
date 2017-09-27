#ifndef CMU462_ERROR_DIALOG_H
#define CMU462_ERROR_DIALOG_H

#include <string>

namespace CMU462 {

void showError(std::string errorString, bool fatal = false);

}  // namespace CMU462

#endif  // CMU462_ERROR_DIALOG_H
