# **Overview**

An implementation of a FIFO matching engine in C++ 14. This projects serves as the learning
platform for my C++ knowledge. This implements two execution strategies GTC and FOK (Fill or Kill). Along with two order types market and limit order. If you're looking to see more order types and/or execution strategies let me know by raising an issue.

Some logic like checking current price is greater than limit price for buy limits is being ignored as it'd be taken care of via the API layer. For an example checkout https://github.com/JadoreThompson/opti-trader where it's been implemented.

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
