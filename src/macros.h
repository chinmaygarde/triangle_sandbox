#pragma once

#include <fml/closure.h>
#include <hedley.h>

#define FML_DEFER(x)                                              \
  fml::ScopedCleanupClosure HEDLEY_CONCAT(_fml_cleanup_for_line_, \
                                          __LINE__)([&]() {       \
    {                                                             \
      x;                                                          \
    }                                                             \
  });
