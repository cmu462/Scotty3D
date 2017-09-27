#ifndef CMU462_MESHEDIT_H
#define CMU462_MESHEDIT_H

#include "halfEdgeMesh.h"

using namespace std;

namespace CMU462 {

class MeshResampler {
 public:
  MeshResampler(){};
  ~MeshResampler() {}

  void upsample(HalfedgeMesh& mesh);
  void downsample(HalfedgeMesh& mesh);
  void resample(HalfedgeMesh& mesh);
};

}  // namespace CMU462

#endif  // CMU462_MESHEDIT_H
