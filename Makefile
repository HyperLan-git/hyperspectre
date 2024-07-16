all:
	cd HyperSpectre/Builds/LinuxMakefile/ && make

VST:
	cd HyperSpectre/Builds/LinuxMakefile/ && make VST3

Standalone:
	cd HyperSpectre/Builds/LinuxMakefile/ && make Standalone

HyperSpectre/Builds/LinuxMakefile/build/HyperSpectre: HyperSpectre/Source/PluginEditor.cpp HyperSpectre/Source/PluginEditor.hpp HyperSpectre/Source/PluginProcessor.cpp HyperSpectre/Source/PluginProcessor.hpp
	cd HyperSpectre/Builds/LinuxMakefile/ && make Standalone

test: HyperSpectre/Builds/LinuxMakefile/build/HyperSpectre
	./HyperSpectre/Builds/LinuxMakefile/build/HyperSpectre

clean:
	cd HyperSpectre/Builds/LinuxMakefile/ && make clean