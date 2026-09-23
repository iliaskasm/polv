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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "test.h"

static void test_fail(TestContext *test, const char *file, int line,
                      const char *format, ...)
{
	va_list args;

	test->failures++;
	fprintf(stderr, "    %s:%d: ", file, line);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputc('\n', stderr);
}


int test_run(const char *name, TestFunction function,
             TestCleanupFunction cleanup, void *state)
{
	TestContext test = { name, 0 };
	int completed;
	int passed;

	printf("[ RUN  ] %s\n", name);
	completed = function(&test, state);

	if (cleanup)
		cleanup(state);

	passed = completed && test.failures == 0;
	printf("[ %s ] %s\n", passed ? "PASS" : "FAIL", name);

	return passed ? 0 : 1;
}


int test_expect(TestContext *test, int condition, const char *expression,
                const char *file, int line)
{
	if (condition)
		return 1;

	test_fail(test, file, line, "expected: %s", expression);
	return 0;
}


int test_expect_result(TestContext *test, RESULT_TYPE actual, RESULT_TYPE expected,
                       const char *expression, const char *file, int line)
{
	if (actual == expected)
		return 1;

	test_fail(test, file, line,
	          "%s returned %d (%s), expected %d (%s)",
	          expression, (int) actual, PSTATUS(actual),
	          (int) expected, PSTATUS(expected));
	return 0;
}


int test_compare_float(TestContext *test, const float *actual, const float *expected,
                       size_t count, float epsilon, const char *file, int line)
{
	size_t i;

	for (i = 0; i < count; i++)
	{
		float a = actual[i];
		float e = expected[i];

		if (isnan(a) || isnan(e) || fabsf(a - e) > epsilon)
		{
			test_fail(test, file, line,
			          "mismatch at %zu: got %.9g, expected %.9g (epsilon %.9g)",
			          i, (double) a, (double) e, (double) epsilon);
			return 0;
		}
	}

	return 1;
}


void test_fill_float(float *data, size_t count, float scale, float bias)
{
	size_t i;
	for (i = 0; i < count; i++)
		data[i] = (float) (i % 251) * scale + bias;
}


void test_free_polv(void **ptrs, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++)
	{
		if (ptrs[i])
		{
#if defined(USE_POLV_CORE)
			polvCoreMemoryFree((POLVCoreMemory *) ptrs[i]);
#elif defined(USE_POLV)
			polvFree(ptrs[i]);
#endif
		}
	}
}


void test_free_host(void **ptrs, size_t count)
{
	size_t i;
	for (i = 0; i < count; i++)
	{
		free(ptrs[i]);
		ptrs[i] = NULL;
	}
}
