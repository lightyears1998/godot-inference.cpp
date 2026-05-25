# Contributing

Thanks for your interest! Any contribution is warmly welcome.

## Style Guidelines

These are guidelines, not strict rules. Use your best judgment.

```cpp
class MyClass {
public:
    int my_var(); // getter
    void set_my_var(); // setter

private:
    int my_var_;

    void private_func();
};
```

```cpp
// 1. corresponding headers
#include "my_class.h"

// 2. project headers
#include "other_component.h"
#include "utils/string_helper.h"

// 3. 3rd party headers and C/C++ standard headers
#include <boost/algorithm/string.hpp>
#include <opencv2/core.hpp>
#include <algorithm>
#include <memory>
#include <vector>
#include <string.h>
```
