# OI-DBT: The OpenISA Dynamic Binary Translator

OI-DBT implements a fast OpenISA interpreter together with an optimizing dynamic compiler supporting several region formation techniques. It dynamically compiles OI instructions into target instructions using the LLVM (7.0) infrastructure. Moreover, OI-DBT translates OI instructions in parallel to the emulation decreasing the compilation overhead. All to achieve a high-performance emulation of OpenISA binaries.

## Bulding it

To build OI-DBT, you are going to need g++7.1 or later with C++17 support, CMake 2.8 or later, PAPI and LLVM 7.0. After installing all dependencies, it is a simple cmake/make usage:

```
cd dbt-openisa
mkdir build
cd build
cmake ..
make
sudo make install
```

### Usage

After building and installing oi-dbt, you can easily use it to emulate OpenISA elf binaries using the following commands:

```
oi-dbt [-rft {net, mret2, lei, lef, netplus} | -interpreter] -bin PathToBinary [-v]

ARGUMENTS:
  -bin : Path to the binary which will be emulated.
  -h : Displays the help message
  -interpret : Only interpret.
  -rft : Region Formation Technique (net)
  -v : Displays the OpenISA instructions from the compiled regions
```

### LICENSE

This project is being developed at the Institute of Computing - Unicamp as part of @vandersonmr doctoral thesis. You are free to contact him and use this code under the MIT LICENSE.

The OI-DBT is part of The OpenISA Project: an infrastructure for a new, open source and fast emulated architecture.
