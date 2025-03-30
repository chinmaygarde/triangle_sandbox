#pragma once

#include <fml/closure.h>
#include <hedley.h>

#define FML_DEFER(x)                                          \
  fml::ScopedCleanupClosure HEDLEY_CONCAT(__cleanup_at_line_, \
                                          __LINE__)([&]() {   \
    {                                                         \
      x;                                                      \
    }                                                         \
  });
