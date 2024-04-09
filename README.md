# Compile Time DFA

Build deterministic finite automaton(DFA) based on regex string at compile time. (using c++20)

### Usage examples:
```cpp

#include "ct_regex.h"

...

//build regex(will complete at compile time)
auto regex = pkuyo::DefineRegex<"\\([0-9]*(.[0-9]+)?\\)",false/*false for not minimize*/>();
std::cout << regex.Match("(2)")<< std::endl;

...

```
## Process introduction

1. Preprocessing regular expression strings ( "[A-C]BA" => "(A|B|C)&B&A" )
>  use function: pkuyo::pkuyo_detail::ctPreprocess<regex>();

2. Build NFA based on regex string (using improved Thompson's construction)
>  use function: pkuyo::pkuyo_detail::connectNfa<useRegex>();

3. Build Building DFA from NFA
>  use function: pkuyo::pkuyo_detail::buildDfa_impl<NfaGraph,DfaGraph,startNode,initStateList,endStateNode>();

4. DFA minimization
>  use function: DFAGraph<...>::Minimize_impl<AllLetterList,DFAGraph::size>(dfa);


If you want to understand in depth, you can also look **pkuyo::pkuyo_detail::RegexDefine<regex>** at **ct_regex.h** (line 1030)
