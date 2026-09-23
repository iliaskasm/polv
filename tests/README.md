# POLV-C Tests

This directory contains the test suites for both the high-level POLV API and the low-level POLV Core API.

Both POLV/POLV Core share the following families:

* `api`: tests public POLV API functionality
* `kernels`: tests common compute kernels such as vector addition and matrix multiplication
* `lifecycle`: tests repeated use of POLV functions and resources
* `memory`: tests memory transfers and memory types

The POLV Core test suite has an extra family:

* `contexts`: tests context creation and current-context handling

The `common` directory contains shared testing utilities and shaders used by both suites.

The `Template` directory provides starting points for implementing new POLV and POLV Core tests.

# Usage

To build and run both test suites:

```sh
make test
```

To run only one test suite from this directory:

```sh
make test_polv
make test_polv_core
```

To run a specific POLV test family:

```sh
make -C polv test_memory
```

To run a specific POLV Core test family:

```sh
make -C polv_core test_contexts
```

Test binaries and intermediate files are generated in each suite's `build/` directory. To remove them, run:

```sh
make clean
```
