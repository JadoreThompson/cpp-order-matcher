# **Overview**

An implementation of a FIFO matching engine in C++ 14. This implements two execution strategies GTC and FOK (Fill or Kill). Along with two order types market and limit order. If you're looking to see more order types and/or execution strategies let me know by raising an issue.

Validation logic like checking for minimum and null values are beign ignored as they'd be performed on the server. Of which you can find an example of those checks at https://github.com/JadoreThompson/order-matcher

Current performance

| Topic                    | Loop Size | Total Time (s) | Avg Time (ns) |
| ------------------------ | --------- | -------------- | ------------- |
| Queue                    | 1,000,000 | 0.538621       | 538.621       |
| GTC (Good-Til-Cancelled) | 1,000,000 | 10.0           | 3,000         |
| FOK (Fill-Or-Kill)       | 1,000,000 | 1.6            | 1,620         |

# **Requirements**

A chosen compiler e.g. g++.

# **Installation**

```
git clone https://github.com/JadoreThompson/cpp-order-matcher.git

cd cpp-order-matcher

# Compiler - if you're using g++
g++ *.cpp -o program.exe

# Run
program
```
