# POLV-C Tests

This directory contains the test suite for POLV. Currently, the tests cover only the high-level POLV API.

The available test families are:

* `api`: tests core POLV API functionality
* `kernels`: tests common compute kernels such as vector addition and matrix multiplication
* `lifecycle`: tests repeated use of POLV functions and resources
* `memory`: tests memory transfers

The `common` directory contains shared testing utilities and shaders used across the test families.

The `Template` directory provides a starting point for implementing new tests.

# Usage

To build and run all tests:

```sh
make test
```

To run a specific test family:

```sh
make test_<family>
```

For example:

```sh
make test_memory
```

These commands can also be executed from the root directory of the project.

Test binaries and intermediate files are generated in the `build/` directory. To remove them, run:

```sh
make clean
```
