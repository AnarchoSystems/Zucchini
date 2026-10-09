#pragma once
#include "IManifestUpdates.h"
namespace nManifestUpdates {
class ManifestUpdates : public IManifestUpdates {
public:
  void already_defined() override {}
  // Becomes a virtual override after discovery adds the missing definition.
  void i_have(long value) { EXPECT_EQ(value, 7); }
};
}
