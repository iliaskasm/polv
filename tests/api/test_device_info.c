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
 * Device information tests
 */
#include <string.h>
#include "test.h"

typedef struct {
	int initialized;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	int devices;

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;

	// Get number of devices
	devices = polvGetNumDevices();
	TEST_REQUIRE(test, devices > 0);

	for (int i = 0; i < devices; i++)
	{
		POLVDeviceInfo info;
		const char *type_name;

		memset(&info, 0, sizeof(info));

		// Get device information
		TEST_REQUIRE_RESULT(test, polvGetDeviceInfo(i, &info), POLV_SUCCESS);
		TEST_REQUIRE(test, info.id == i);
		TEST_REQUIRE(test, info.name[0] != '\0');
		TEST_REQUIRE(test, info.api_version_major > 0);
		TEST_REQUIRE(test, info.max_compute_work_group_invocations > 0);
		TEST_REQUIRE(test, info.max_compute_work_group_size[0] > 0);
		TEST_REQUIRE(test, info.max_compute_work_group_size[1] > 0);
		TEST_REQUIRE(test, info.max_compute_work_group_size[2] > 0);
		TEST_REQUIRE(test, info.max_compute_work_group_count[0] > 0);
		TEST_REQUIRE(test, info.max_compute_work_group_count[1] > 0);
		TEST_REQUIRE(test, info.max_compute_work_group_count[2] > 0);

		type_name = polvDeviceTypeName(info.type);
		TEST_REQUIRE(test, type_name != NULL);
		TEST_REQUIRE(test, type_name[0] != '\0');
	}

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;

	if (state->initialized)
		polvFinalize();
}


int main(void)
{
	State state = { 0 };

	return test_run("api/device_info", run, cleanup, &state);
}
