# Compile Time DFA

Build deterministic finite automaton(DFA) based on regex string at compile time. (using c++20)


### Usage examples:
```cpp

#include "ct_regex.h"

...

//build regex(will complete at compile time)
auto regex = DefineRegex<"\\([0-9]*(.[0-9]+)?\\)",false/*false for not minimize*/>();
std::cout << regex.Match("(2)")<< std::endl;
...

```

