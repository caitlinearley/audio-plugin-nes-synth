# NES-Style Synthesiser Audio Plugin  
*A real-time audio plugin implementing classic NES sound features via DSP techniques.*

## Overview  
This project implements a synthesiser audio plugin that captures the classic 8-bit “NES” sound aesthetic by using Fourier analysis, bit-crushing and a custom DSP signal loop. It is built in C++ (with Objective-C parts) to work in a real-time audio context.  

## Features & Techniques  
- Classic NES waveform synthesis: square, triangle, noise channels, etc.  
- Fourier-based signal processing and custom bit-crushing algorithm to emulate retro sound quality.  
- Real‐time audio processing architecture (DSP loop, sample-accurate) in a plugin host environment.  
- Modular design using C++ classes: oscillator engine, sampler, plugin editor and processor components.

## Key Skills Demonstrated  
- C++ and audio DSP programming for real-time applications.  
- Algorithmic design for waveform generation, bit-crushing, and custom audio effects.  
- Plugin architecture: user interface, processor/editor separation, efficient real-time audio loops.  
- Translating a retro game-audio aesthetic into a technically rigorous implementation.

## Repository Structure  
