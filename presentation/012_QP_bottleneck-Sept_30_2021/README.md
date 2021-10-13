# Short Description

QP on the memory side NICs are a scarce resource, as such we should measure the
effect of using fewer of them. This experiment constricts the number of memory
QP that the clients can use to access remote memory.

## Mechanism

In prior experiments, to choose a memory side QP for the QP mapping, we take
`key % clients`. This makes use of all of the memory QP evenly. In order to have
all memory accesses be ordered we need the reads and writes for specific keys to
land on the same QP. We want to motivate the need to use more than just one. In
each measurement I run `key % num` where num is some factor of two of the full
set of QP.

## Reference

A measurement similar to this was taken in [design guidelines for high
performance RDMA
sysetems](https://www.usenix.org/system/files/conference/atc16/atc16_paper-kalia.pdf).
Their performance metric is taken on RDMA primatives which gives a pure
measurement of the performance on ConnectX-3 NICs.
![ref0](atc_16_rdma_guidelines_fig_11.png). They show a sharp increase in
performance by adding an additional QP to the operations. The additive
performance bennift falls off after 2. In the case of raw IB the performance
continues to grow almost linearly (aprox 6million additional ops per second per
qp).

## Experiments

Running with 64 client threads on two machines.


## Experiment 0 - CNS swapping on vs off throughput measure.

![exp0](QP_restriction.png "Memory QP vs Client QP")

## Experiment 1 - ID QP mapping (still incomplete)

In this second experiment I'm using a different mechanism to determine which QP
the requests land on. The mapping is done entirely by the ID. Where as before
the requests were state based, reliant on the algorithm, in this case they are
100% reliant on the QP connection. In some respects this is a more pure
measurment especially when I perform it using powers of 2.


![exp1](id_qp.png "Memory QP vs Client QP")

## results

This experiment is currently incomplete, however I hypothesize that we will not
see much of a performance degradation here as the absolute value of the ops/s is
not in the range noted by prior work.

## Experiment 2 - ID QP mapping queue pair restrictions

The goal of this experiment is to determine the differences in performance when I funnel multiple threads into a subset of queue pairs. Here I have CAS turned on, there are no atomic replacements, and I'm using queue pairs as identifiers, not keys to steer requests. In each experiment I set the number of queue pairs to be static, each one is given a separate line, and I increase the load by adding client threads. An example of the mapping would be thus

4 threads, 4 qp

1 --> 1 <br>
2 --> 2<br>
3 --> 3<br>
4 --> 4<br>

4 threads, 2 qp

1 --> 1<br>
2 --> 2<br>
3 --> 1<br>
4 --> 2<br>

6 threads, 4 qp

1 --> 1<br>
2 --> 2<br>
3 --> 3<br>
4 --> 4<br>
5 --> 1<br>
6 --> 2<br>

as can be seen the load per qp is a function of the number of threads, not the distribution of the workload. My experiment usually fail when I fold more than one client into a single qp. This is actually relatively new behavior after I stabilized the dequeue mechanism. The point of this experiment is to see if there are any imergent patters now that my results are consistent.

![exp2](Experiment_2-id-to-qp-vs-queues.png "Memory QP vs client threads")

The first key takeaway is that at least at this speed, the number of qp does not
seem to have an apreciable effect on the performance. Each point is taken from a
single run, and they all seem to be within the variance of each other. I've
placed a skull where each theread dies. In most cases it's when I fold at least
2x the number of clients into it. Each skull is the last valid measurement, not
the point of failure. For instance the Green skull at 12 clients shows that the next measurement at 14 failed. 

## results

The number of qp does not seem to make a difference at low speeds, however
folding multiple clients into a single qp is often, but not always a failure
with this current setup. Note that 48 qp is sufficient for 64 threads with no
trouble, but 12 qp is not enough for 14. This is very likely to do with an error
in my code, and not a fundamental issue with the NICs.

