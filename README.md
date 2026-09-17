# POLV-C

POLV-C (Portable Offloading Library for Vulkan devices) is a C library for
running compute workloads on Vulkan-capable devices without requiring
applications to manage Vulkan objects directly.

The project provides two interfaces:

- **POLV**: a high-level API for device selection, memory allocation, data
  transfers, and SPIR-V compute shader execution.
- **POLV Core**: a lower-level API for applications that need explicit control
  over devices, contexts, memory, and kernels.

The motivation behind POLV-C is to provide a small, portable offloading
interface on top of Vulkan while keeping the programming model familiar to C
applications.

## Dependencies

Building POLV-C requires:

- a C compiler (e.g., GCC or Clang)
- GNU Make
- Vulkan headers and loader development files
- a Vulkan-capable device with a working Vulkan driver

Building the samples also requires:

- `glslangValidator`, used to compile GLSL compute shaders to SPIR-V

On Debian or Ubuntu, the development tools can be installed with:

```sh
sudo apt install build-essential libvulkan-dev glslang-tools vulkan-tools
```

A working Vulkan installation can be checked with:

```sh
vulkaninfo
```

## Installation

Build POLV-C with:

```sh
make
```

Optionally run the compile-time checks:

```sh
make check
```

### Local installation

By default, POLV-C installs into the `install` directory inside the repository:

```sh
make install
```

This creates:

```text
install/
	include/
		polv.h
		polv_core.h
		polv_version.h
	lib/
		libpolv.so
		libpolvcore.so
```

The samples are configured to use this local installation:

```sh
make -C samples
make -C samples run
```


### Installation under a different directory

POLV-C can be installed into its own directory, e.g., under `/usr/local`:

```sh
sudo make install INSTALL_DIR=/usr/local/polv
```

Add the POLV-C include and library directories to the compiler, linker, and
runtime library search paths:

```sh
export CPATH="/usr/local/polv/include${CPATH:+:$CPATH}"
export LIBRARY_PATH="/usr/local/polv/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"
export LD_LIBRARY_PATH="/usr/local/polv/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
```

To make these settings persistent for Bash, add them to `~/.bashrc`.
 
After configuring these paths, an application using the POLV API can be
compiled directly, without any `-I` or `-L` options, with:

```sh
gcc program.c -lpolv -o program
```

...or using the POLV Core API:

```sh
gcc program.c -lpolvcore -o program
```

## Device information

After initialization, the POLV API can query portable information about
any compute device without exposing Vulkan types:

```c
POLVDeviceInfo info;

if (polvGetDeviceInfo(0, &info) == POLV_SUCCESS) 
{
    printf("%s (%s)\n", info.name, polvDeviceTypeName(info.type));
    printf("device-local memory: %llu bytes\n",
           (unsigned long long) info.device_local_memory_bytes);
}
```

`POLVDeviceInfo` includes the device name and type, vendor/device identifiers,
Vulkan API version, and other information (see polv.h).

POLV Core also exposes the cached Vulkan device data directly:

```c
POLVCoreDevice *device = polvCoreGetDevice(0);
VkPhysicalDeviceProperties properties;
VkPhysicalDeviceFeatures features;
VkPhysicalDeviceMemoryProperties memory_properties;

polvCoreDeviceGetProperties(device, &properties);
polvCoreDeviceGetFeatures(device, &features);
polvCoreDeviceGetMemoryProperties(device, &memory_properties);
```

Additional Core helpers return the POLV device ID, total device-local memory,
and selected compute queue-family index (see polv_core.h).

## Samples

Both interfaces include matching examples under `samples/`:

| Sample | POLV | POLV Core | Info |
| - | - | - | - |
| `01_device_info` | yes | yes | prints information about installed Vulkan-capable devices |
| `02_vecadd` | yes | yes | vector addition using device local memory |
| `03_vecadd_unified` | yes | yes | vector addition using host-coherent memory |
| `04_memcpy` | yes | yes | tests H2D, D2H and D2D memory copies |
| `05_saxpy` | yes | yes | performs single-precision ax + y |
| `06_matadd` | yes | yes | matrix addition |
| `07_bandwidth` | yes | yes | tests bandwidth of H2D, D2H and D2D copies |
| `08_matmul` | yes | yes | matrix multiplication (simple) |

Build and run the complete sample set with:

```sh
make install
make -C samples
make -C samples run
```

The bandwidth sample defaults to 64 MiB transfers and 20 timed iterations.
The transfer size (in MiB) and iteration count can be overridden on the command
line, for example:

```sh
./samples/polv/07_bandwidth/bandwidth 256 50
./samples/polv_core/07_bandwidth/bandwidth_core 256 50
```

The reported H2D, D2H, and D2D rates are end-to-end synchronous POLV copy
throughput, including the current command submission/wait behavior and any
staging work performed by the memory-copy implementation. They should not be
interpreted as raw PCIe or VRAM bandwidth.

## Example

The following POLV example adds two arrays using a Vulkan compute shader and host-coherent memory.
The variant that utilizes device-local memory can be found under `samples/polv/02_vecadd/vecadd.c`.

### Compute shader

Create `vecadd.comp`:

```glsl
#version 450

layout(local_size_x_id = 0,
       local_size_y_id = 1,
       local_size_z_id = 2) in;

layout(set = 0, binding = 0) readonly buffer A
{
    float a[];
};

layout(set = 0, binding = 1) readonly buffer B
{
    float b[];
};

layout(set = 0, binding = 2) writeonly buffer C
{
    float c[];
};

void main()
{
    uint i = gl_GlobalInvocationID.x;

    if (i >= c.length())
        return;

    c[i] = a[i] + b[i];
}
```

Compile it to SPIR-V:

```sh
glslangValidator -V vecadd.comp -o vecadd.spv
```

### C program

Create `vecadd.c`:

```c
#include <stdint.h>
#include <stdio.h>
#include <polv.h>

static void cleanup(void *d_a, void *d_b, void *d_c)
{
	polvFree(d_a);
	polvFree(d_b);
	polvFree(d_c);
	polvFinalize();
}

int main(void)
{
	const size_t n = 1024, threads = 128;
	const size_t bytes = n * sizeof(float);

	void *d_a = NULL, *d_b = NULL, *d_c = NULL;
	float *a = NULL, *b = NULL, *c = NULL;
	size_t i;

	if (polvInit() != POLV_SUCCESS)
		return 1;

	if (polvSetDevice(0) != POLV_SUCCESS)
	{
		cleanup(d_a, d_b, d_c);
		return 1;
	}

	d_a = polvAlloc(bytes, polvMemHostCoherent);
	d_b = polvAlloc(bytes, polvMemHostCoherent);
	d_c = polvAlloc(bytes, polvMemHostCoherent);
	if (!d_a || !d_b || !d_c)
	{
		cleanup(d_a, d_b, d_c);
		return 1;
	}

	a = polvGetHostPointer(d_a);
	b = polvGetHostPointer(d_b);
	c = polvGetHostPointer(d_c);
	if (!a || !b || !c)
	{
		cleanup(d_a, d_b, d_c);
		return 1;
	}

	for (i = 0; i < n; i++)
	{
		a[i] = (float) i;
		b[i] = (float) (2 * i);
		c[i] = 0.0f;
	}

	void *args[] = { d_a, d_b, d_c };
	POLVDim group = { threads, 1, 1 };
	POLVDim grid = { ((n + threads - 1) / threads), 1, 1 };

	POLVResult result = polvKernelLaunch("vecadd.spv", args, 3, grid, group);

	if (result != POLV_SUCCESS)
	{
		fprintf(stderr, "kernel launch failed: %s\n", polvStatus(result));
		cleanup(d_a, d_b, d_c);
		return 1;
	}

	for (i = 0; i < n; i++)
	{
		float expected = a[i] + b[i];

		if (c[i] != expected)
		{
			fprintf(stderr, "incorrect result at index %zu\n", i);
			cleanup(d_a, d_b, d_c);
			return 1;
		}
	}

	printf("OK\n");
	cleanup(d_a, d_b, d_c);

	return 0;
}
```

Compile and run it after installing POLV-C under `/usr/local/polv` and
configuring the environment variables above:

```sh
gcc vecadd.c -lpolv -o vecadd
./vecadd
```

Expected output:

```text
OK
```

## Requirements
* POLV requires a Vulkan-capable device and a Vulkan 1.0-compatible driver.
* Compute shaders must be provided as SPIR-V binaries.
* Shader workgroup dimensions are supplied through specialization constants with IDs 0, 1, and 2:

```glsl
layout(local_size_x_id = 0,
       local_size_y_id = 1,
       local_size_z_id = 2) in;
```

These correspond to the `x`, `y`, and `z` workgroup dimensions passed to the POLV kernel launch API.

## Current limitations
* POLV currently supports buffer-based kernel arguments only; each kernel argument is a Vulkan storage buffer.
* User-defined push constants and additional specialization constants are not currently exposed through the POLV API. Specialization constant IDs 0, 1, and 2 are reserved for workgroup dimensions.
* POLV does not currently expose configuration for enabling optional Vulkan device features.
* The current runtime uses a global state for device and context management. Concurrent access from multiple host threads is not currently guaranteed to be thread-safe.
* POLV does not currently provide explicit multi-queue scheduling or concurrent command-stream management.

## General notes
* Memory returned by `polvAlloc()` is a POLV allocation handle. Use `polvGetHostPointer()` with host-coherent allocations when direct CPU access is required.
* Kernel workgroup sizes must respect the limits reported by the underlying Vulkan device.
* POLV Core exposes lower-level Vulkan-oriented functionality, while the POLV API provides the simpler high-level offloading interface.
* Applications using POLV from multiple host threads should externally serialize access to the POLV runtime (e.g., through mutexes).
