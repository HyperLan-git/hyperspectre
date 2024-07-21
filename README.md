# HyperSpectre
I just want to prove I can code a good plugin and I'm gonna try to apply the frequency reassignment algorithm. It doesn't look perfect but it works!
I dislike how juce is designed but I can't get myself to make much better anyways so ig I'll have to do with it.

![How the plugin currently looks](https://github.com/HyperLan-git/hyperspectre/blob/master/test.png?raw=true)

## Dependencies
On windows, you just need Visual Studio 2022 and Juce.
In addition to juce's main dependencies on linux, you'll need libasan to compile in debug mode (I love when I see third party commercial libraries causing segfaults or leaks with zero means to prevent it).

## How to compile
On windows, simply open the project from projucer and build the solution you need.

On linux, use the makefile :
```sh
make
```

To compile in Release mode (with optimisations and no memory sanitizer), use `make CONFIG=Release`.
You can clean binaries with `make clean`.
