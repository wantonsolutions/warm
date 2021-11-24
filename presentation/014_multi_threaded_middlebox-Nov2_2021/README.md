# Short Description

This is a placeholder for the hero graph. I've installed Vtune and done a number
of performance improvements for the single core implementation. The point of
this set of experiments is to determine what the best single core performance
is, and when to jump to the multi-core experiments.

Most of what I've done here is improve the algorithms internally without
changing any of the function. For instance many of the functions which searched
for id's are now O(1). These are easy performance gains.
