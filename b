#!/bin/bash
clang -Og -g -Wall -Wextra -std=c11 -fsanitize=undefined pratt.c && ./a.out
