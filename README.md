# HyperSpectre
I just want to prove I can code a good plugin and I'm gonna try to apply the frequency reassignment algorithm. It doesn't look perfect but it works!
I dislike how juce is designed but I wouldn't have been able to make much better anyways so ig I'll have to do with it.

## Dependencies
In addition to juce's main dependencies, you'll need libasan to compile in debug mode (I love when I see third party commercial libraries causing segfaults or leaks with zero means to prevent it).

## How to compile
Simply use the makefile :
```sh
make
```

To compile in Release mode (with optimisations and no memory sanitizer), use `make CONFIG=Release`.
You can clean binaries with `make clean`.

I think I messed up my juce installation because most of it won't compile if I don't compile as su.
