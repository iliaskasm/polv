/* 
 * MIT License
 * 
 * Copyright (c) 2026 Ilias K. Kasmeridis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
 * Matrix multiplication (using JIT shader compilation)
 */
#include <stdint.h>
#include <stdlib.h>
#include "test.h"

typedef struct 
{ 
	uint32_t m;
	uint32_t n;
	uint32_t k; 
} Shape;

typedef struct {
	int initialized;
	float *a;
	float *b;
	float *c;
	float *expected;
	void *da;
	void *db;
	void *dc;
	void *shape;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const uint32_t m = 19, n = 17, k = 13,
	               tx = 8, ty = 8;
	const size_t a_count = (size_t) m * k,
	             b_count = (size_t) k * n,
	             c_count = (size_t) m * n;
	size_t i;
	uint32_t row, col;
	Shape *shape;
	void *args[4];
	POLVDim group = { tx, ty, 1 },
	        grid = { (n + tx - 1) / tx, (m + ty - 1) / ty, 1 };

	// Initialize host
	state->a = malloc(a_count * sizeof(float));
	state->b = malloc(b_count * sizeof(float));
	state->c = malloc(c_count * sizeof(float));
	state->expected = malloc(c_count * sizeof(float));
	TEST_REQUIRE(test, state->a && state->b && state->c && state->expected);

	for (i = 0; i < a_count; i++)
		state->a[i] = (float) ((int) (i % 7) - 3) * 0.25f;
	for (i = 0; i < b_count; i++)
		state->b[i] = (float) ((int) (i % 5) - 2) * 0.5f;
	for (row = 0; row < m; row++)
		for (col = 0; col < n; col++)
		{
			uint32_t p;
			float sum = 0.0f;
			for (p = 0; p < k; p++)
				sum += state->a[(size_t) row * k + p] * state->b[(size_t) p * n + col];
			state->expected[(size_t) row * n + col] = sum;
		}

	// Initialize POLV
	TEST_REQUIRE_SUCCESS(test, polvInit());
	state->initialized = 1;

	// Allocate device local memory for a, b and c
	state->da = polvAlloc(a_count * sizeof(float), polvMemDeviceLocal);
	state->db = polvAlloc(b_count * sizeof(float), polvMemDeviceLocal);
	state->dc = polvAlloc(c_count * sizeof(float), polvMemDeviceLocal);

	// Allocate host-coherent memory for shape
	state->shape = polvAlloc(sizeof(Shape), polvMemHostCoherent);

	TEST_REQUIRE(test, state->da && state->db && state->dc && state->shape);

	// Get host pointer for shape
	shape = polvGetHostPointer(state->shape);
	TEST_REQUIRE(test, shape != NULL);
	shape->m = m; 
	shape->n = n; 
	shape->k = k;

	// HOST a, b -> DEVICE a, b
	TEST_REQUIRE_SUCCESS(test, polvMemcpy(state->a, 0, state->da, 0, a_count * sizeof(float), polvHostToDevice));
	TEST_REQUIRE_SUCCESS(test, polvMemcpy(state->b, 0, state->db, 0, b_count * sizeof(float), polvHostToDevice));

	args[0] = state->da;
	args[1] = state->db;
	args[2] = state->dc;
	args[3] = state->shape;

	// Kernel launch
	TEST_REQUIRE_SUCCESS(test, polvKernelLaunchFromGLSL(TEST_SHADER_SRC("matmul"), args, 4, grid, group));

	// DEVICE c -> HOST c
	TEST_REQUIRE_SUCCESS(test, polvMemcpy(state->dc, 0, state->c, 0, c_count * sizeof(float), polvDeviceToHost));
	TEST_REQUIRE(test, TEST_COMPARE_FLOAT(test, state->c, state->expected, c_count, 1.0e-5f));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *host_ptrs[] = { state->a, state->b, state->c, state->expected };
	void *polv_ptrs[] = { state->da, state->db, state->dc, state->shape };

	if (state->initialized) 
	{ 
		test_free_polv(polv_ptrs, 4); 
		polvFinalize(); 
	}
	test_free_host(host_ptrs, 4);
}


int main(void)
{
	State state = { 0 };
	return test_run("kernels/matmul_jit", run, cleanup, &state);
}
