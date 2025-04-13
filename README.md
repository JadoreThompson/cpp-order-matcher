# **Overview**

An implementation of a FIFO matching engine in C++ 14. This implements two execution strategies GTC and FOK (Fill or Kill). Along with two order types market and limit order. If you're looking to see more order types and/or execution strategies let me know by raising an issue.

Validation logic like checking for minimum and null values are beign ignored as they'd be performed on the server. Of which you can find an example of those checks at https://github.com/JadoreThompson/order-matcher

Performance

| Topic                                      | Loop Size | Total Time (s) | Avg Time (ns) |
| ------------------------------------------ | --------- | -------------- | ------------- |
| Queue                                      | 1,000,000 | 0.483688       | 483.688       |
| GTC (Good-Til-Cancelled) (Queue Inclusive) | 1,000,000 | 1.089396       | 1089.396      |
| FOK (Fill-Or-Kill) (Queue Inclusivev)      | 1,000,000 | 0.678202       | 678.202       |

# **Requirements**

A chosen compiler. I've tested this with zig and gcc compiler. The zig compiler (utilising clang under the hood) garners the best results where compiling with gcc makes the program around 1.25x slower.

# **Installation**

```
git clone https://github.com/JadoreThompson/cpp-order-matcher.git

cd cpp-order-matcher

# Compiler - if you're using g++
g++ *.cpp -o program.exe

# Run
program
```
