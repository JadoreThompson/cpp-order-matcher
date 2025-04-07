# **Overview**

An implementation of a FIFO matching engine in C++ 14. This implements two execution strategies GTC and FOK (Fill or Kill). Along with two order types market and limit order. If you're looking to see more order types and/or execution strategies let me know by raising an issue.

Validation logic like checking for minimum and null values are beign ignored as they'd be performed on the server. Of which you can find an example of those checks at https://github.com/JadoreThompson/order-matcher 

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
