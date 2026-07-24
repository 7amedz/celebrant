# Celebrant

> _"Celebrant is a prominent city. In it, we could find passage wherever we wish to go."_
> — Pattern, _Oathbringer_ (Brandon Sanderson)

Celebrant is a limit-order matching engine optimized for speed and high performance. Simply put, it matches resting orders against incoming ones of opposite nature(buy/sell) and produces a trade, prioritizing price and arrival time.

The design of such systems is fascinating, and the speed requirement forces best practices with regards to data structures and the main program loop.

It's being built from the ground up in c++ with the goal to learn the inner workings of such engines and how exchanges use them to execute trades in high frequency at such a scale.

Currently, only the core engine is implemented, tested, and benchmarked(87ns median per match), but the plan is to expand the project into a fully functioning trading system.

### Build

```sh
# build and test
cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure

# benchmark, pinning to core 3(or whichever one you wish :D) for reproducible results
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release && cmake --build build-release
taskset -c 3 ./build-release/bench/celebrant_bench
```

### TODO

- order entry over TCP
- a simulator for live demo
- a live feed and dashboard to display market (frontend)
