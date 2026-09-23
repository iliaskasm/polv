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
 * POLV Test interface
 */
#ifndef POLV_TEST_H
#define POLV_TEST_H

#include <stddef.h>
#include <stdint.h>

#if defined(USE_POLV)
	#include <polv.h>
	#define RESULT_TYPE    POLVResult
	#define PSTATUS        polvStatus
	#define RESULT_SUCCESS POLV_SUCCESS
#elif defined(USE_POLV_CORE)
	#include <polv_core.h>
	#define RESULT_TYPE    POLVCoreResult
	#define PSTATUS        polvCoreStatus
	#define RESULT_SUCCESS POLV_CORE_SUCCESS
#elif defined(__INTELLISENSE__) // IntelliSense-only defs
	#include <polv.h>
	#include <polv_core.h>
	#define RESULT_TYPE     int
	#define PSTATUS(status) ""
	#define RESULT_SUCCESS  1
#else
	#error "Either USE_POLV or USE_POLV_CORE must be defined"
#endif

#ifndef POLV_TEST_KERNEL_DIR
	#define POLV_TEST_KERNEL_DIR "build/kernels"
#endif

/* 
 * Typedefs 
 */
typedef struct TestContext_
{
	const char *name;
	int failures;
} TestContext;

typedef int (*TestFunction)(TestContext *test, void *state);
typedef void (*TestCleanupFunction)(void *state);

/*
 * Helper defs 
 */
#define TEST_SHADER(name) POLV_TEST_KERNEL_DIR "/" name ".spv"
#define TEST_EXPECT(test, expression) \
	test_expect((test), !!(expression), #expression, __FILE__, __LINE__)

#define TEST_EXPECT_RESULT(test, expression, expected) \
	test_expect_result((test), (expression), (expected), #expression, __FILE__, __LINE__)

#define TEST_REQUIRE(test, expression) \
	do { \
		if (!TEST_EXPECT((test), (expression))) \
			return 0; \
	} while (0)

#define TEST_REQUIRE_RESULT(test, expression, expected) \
	do { \
		if (!TEST_EXPECT_RESULT((test), (expression), (expected))) \
			return 0; \
	} while (0)

#define TEST_REQUIRE_SUCCESS(test, expression) \
	TEST_REQUIRE_RESULT(test, expression, RESULT_SUCCESS)

#define TEST_COMPARE_FLOAT(test, actual, expected, count, epsilon) \
	test_compare_float((test), (actual), (expected), (count), (epsilon), __FILE__, __LINE__)

/* 
 * Functions 
 */
// Runs a test function and creates a test context
int test_run(const char *name, TestFunction function,
             TestCleanupFunction cleanup, void *state);

// Expects `condition == 1`.
// If not, it prints the error and adds +1 to the test context's failures.
int test_expect(TestContext *test, int condition, const char *expression,
                const char *file, int line);

// Expects `actual == expected`, used for POLV function calls.
// If not, it prints the error and adds +1 to the test context's failures.
int test_expect_result(TestContext *test, RESULT_TYPE actual, RESULT_TYPE expected,
                       const char *expression, const char *file, int line);

// Expects `actual == expected`, used for floats and usually during verification.
// If the condition is not true, it prints the error and adds +1 to the test
// context's failures.
int test_compare_float(TestContext *test, const float *actual, const float *expected,
                       size_t count, float epsilon, const char *file, int line);

// Fills a float array with values.
void test_fill_float(float *data, size_t count, float scale, float bias);

// Frees POLV or POLV Core memory
void test_free_polv(void **ptrs, size_t count);

// Frees up HOST memory
void test_free_host(void **ptrs, size_t count);

#endif /* POLV_TEST_H */
