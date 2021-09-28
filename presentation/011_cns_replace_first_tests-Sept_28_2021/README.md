# Short Description

CNS are expensive, ive replaced all of the CNS operation with writes, and
inforced ordering. Ideally this should have a significant performance boost.
Considering that it took me around a year I hope that this is correct. The goal of this experiment is to test the baseline throughput of the mechanism against a default

## mechanism

## Experiments

With CNS turned on so is all of the mapping functionality. That is we are doing
qp steering towards queues that serve individual keys. The mapping is round
robin. As CNS operations come in they are replaced with writes. When the write
acks come back, I swap them out with atomic ACKS. The big issue with this move
is that when packets are coalesed on the memory side I don't get some of the
acks back to the client. My algorithm now fixes that by injecting acks.



## Experiment 0 - Basic Cached Key - one key


![exp0](experiment_0.png "CNS swapping on vs off")
