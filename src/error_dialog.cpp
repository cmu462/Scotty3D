#include "error_dialog.h"

#include "CMU462/viewer.h"

namespace CMU462 {

// Simple passthrough function to hide cass member function call
void showError(std::string errorString, bool fatal) {
  Viewer::showError(errorString, fatal);
}

}  // namespace CMU462
