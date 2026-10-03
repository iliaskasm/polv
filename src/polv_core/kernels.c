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
 * POLV Core kernel helpers (internal)
 */
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "kernels.h"
#include "polv_core_internal.h"

static char *dup_string(const char *s)
{
	size_t n;
	char *copy;

	if (!s)
		return NULL;
	n = strlen(s) + 1;
	copy = (char *) smalloc(n);
	if (copy)
		memcpy(copy, s, n);
	return copy;
}


/* Reads an entire file into memory. */
static POLVCoreResult read_file(const char *path, void **out, size_t *out_size, unsigned long typesize)
{
	FILE *fp;
	long n;

	if (!path || !out || !out_size)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	*out = NULL;
	*out_size = 0;

	fp = fopen(path, "rb");
	if (!fp)
		return POLV_CORE_ERROR_SHADER;

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return POLV_CORE_ERROR_SHADER;
	}

	n = ftell(fp);
	if (n <= 0 || ((size_t) n % typesize) != 0)
	{
		fclose(fp);
		return POLV_CORE_ERROR_SHADER;
	}

	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		fclose(fp);
		return POLV_CORE_ERROR_SHADER;
	}

	*out = smalloc((size_t) n);
	if (!*out)
	{
		fclose(fp);
		return POLV_CORE_ERROR_OUT_OF_MEMORY;
	}

	if (fread(*out, 1, (size_t) n, fp) != (size_t) n)
	{
		free(*out);
		*out = NULL;
		fclose(fp);
		return POLV_CORE_ERROR_SHADER;
	}

	fclose(fp);
	*out_size = (size_t) n;
	return POLV_CORE_SUCCESS;
}


static POLVCoreResult read_file_uint32(const char *path, uint32_t **out, size_t *out_size)
{
	return read_file(path, (void **) out, out_size, sizeof(uint32_t));
}


static POLVCoreResult read_file_str(const char *path, char **out, size_t *out_size)
{
	return read_file(path, (void **) out, out_size, sizeof(char));
}


/* Makes a shader cache key, for a given filename.
 * Key is in the following form:
 *
 *   <path>|dev=<storage_device>|ino=<inode>|size=<filesize>|mtime=<modification_time>
 */
static char *make_shader_cache_key(const char *filename)
{
	char resolved[PATH_MAX];
	struct stat st;
	char *key;
	int n;

	if (!filename || realpath(filename, resolved) == NULL)
		return NULL;
	if (stat(resolved, &st) != 0)
		return NULL;

	n = snprintf(NULL, 0, "%s|dev=%ld|ino=%ld|size=%ld|mtime=%ld",
	                      resolved, (long) st.st_dev, (long) st.st_ino,
	                      (long) st.st_size, (long) st.st_mtime);
	if (n < 0)
		return NULL;

	key = (char *) smalloc((size_t) n + 1);
	if (!key)
		return NULL;

	snprintf(key, (size_t) n + 1, "%s|dev=%ld|ino=%ld|size=%ld|mtime=%ld",
	                             resolved, (long) st.st_dev, (long) st.st_ino,
	                             (long) st.st_size, (long) st.st_mtime);
	return key;
}


/* Destroys a shader module. */
static void destroy_shader_module(POLVCoreDevice *dev, POLVCoreShader *s)
{
	if (!dev || !s || dev->device == VK_NULL_HANDLE)
		return;

	if (s->compute_shader_module != VK_NULL_HANDLE)
		vkDestroyShaderModule(dev->device, s->compute_shader_module, NULL);

	s->compute_shader_module = VK_NULL_HANDLE;
}


/* Destroys a shader given its ID. */
void polvc_kernels_destroy_shader(POLVCoreDevice *dev, int shader_id)
{
	POLVCoreShader *s;

	if (!dev || shader_id < 0 || shader_id >= POLV_SHADER_CACHE_SIZE)
		return;

	s = &dev->shader_cache[shader_id];
	destroy_shader_module(dev, s);
	free(s->filename);
	free(s->cache_key);
	memset(s, 0, sizeof(*s));
}


static int _cache_enabled(void)
{
	const char *cache = getenv("POLV_CACHE");

	return !cache || strcmp(cache, "0") != 0;
}


/* Generic function for shader creation */
static int polvc_kernels_shader_create(POLVCoreDevice *dev, const uint32_t *code, size_t code_size,
                                       const char *filename, char *cache_key)
{
	VkShaderModuleCreateInfo ci;
	POLVCoreShader *s;
	int id;

	if (!dev || !code || code_size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	/* Vulkan expects SPIR-V code size in bytes, aligned to uint32_t words. */
	if (code_size % sizeof(uint32_t) != 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (dev->nshaders >= POLV_SHADER_CACHE_SIZE)
		return POLV_CORE_ERROR_SHADER;

	id = dev->nshaders;
	s = &dev->shader_cache[id];

	memset(s, 0, sizeof(*s));

	s->owner = dev;
	s->cache_key = cache_key;

	if (filename)
	{
		s->filename = dup_string(filename);
		if (!s->filename)
		{
			memset(s, 0, sizeof(*s));
			return POLV_CORE_ERROR_OUT_OF_MEMORY;
		}
	}

	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ci.codeSize = code_size;
	ci.pCode = code;

	if (vkCreateShaderModule(dev->device, &ci, NULL,
	                         &s->compute_shader_module) != VK_SUCCESS)
	{
		free(s->filename);
		memset(s, 0, sizeof(*s));
		return POLV_CORE_ERROR_SHADER;
	}

	/* From this point, the shader owns filename and cache_key */
	++dev->nshaders;

	return id;
}


/* Creates a shader from an in-memory SPIR-V binary.
 * code_size is the binary size in bytes.
 */
int polvc_kernels_shader_new_from_spv_raw(POLVCoreDevice *dev, const uint32_t *code, 
                                          size_t code_size)
{
	if (!dev || !code || code_size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	return polvc_kernels_shader_create(dev, code, code_size, NULL, NULL);
}


/* Creates a shader from a SPIR-V binary file. */
int polvc_kernels_shader_new_from_spv_file(POLVCoreDevice *dev,
                                           const char *shader_filename,
                                           int ignore)
{
	uint32_t *code;
	size_t code_size;
	char *new_key;
	POLVCoreResult res;
	int id;

	(void) ignore;

	if (!dev || !shader_filename)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	/* (1) Create cache key */
	new_key = make_shader_cache_key(shader_filename);
	if (!new_key)
		return POLV_CORE_ERROR_SHADER;

	/* (2) Return an already cached shader */
	for (id = 0; id < dev->nshaders; ++id)
	{
		if (dev->shader_cache[id].cache_key &&
		    strcmp(dev->shader_cache[id].cache_key, new_key) == 0)
		{
			free(new_key);
			return id;
		}
	}

	if (dev->nshaders >= POLV_SHADER_CACHE_SIZE)
	{
		free(new_key);
		return POLV_CORE_ERROR_SHADER;
	}

	/* (3) Read SPIR-V binary */
	code = NULL;
	code_size = 0;

	res = read_file_uint32(shader_filename, &code, &code_size);
	if (res != POLV_CORE_SUCCESS)
	{
		free(new_key);
		return res;
	}

	/* (4) Create and cache Vulkan shader module */
	id = polvc_kernels_shader_create(dev, code, code_size, shader_filename, new_key);

	free(code);

	if (id < 0)
	{
		free(new_key);
		return id;
	}

	/* Success: ownership of new_key is now held by shader_cache[id] */
	return id;
}


/* djb33_hash 64-bit */
static uint64_t _hash_update(uint64_t h, const void *data, size_t len)
{
	const unsigned char *p = data;

	while (len--)
	{
		h += h << 5;
		h ^= *p++;
	}

	return h;
}


/* Hashes passed data, and if use_tag==1 it also employs a tag */
static uint64_t _shader_hash(const void *data, size_t len, int use_tag)
{
	static const char cache_tag[] = "polv-1.0|glslangValidator|-V|-S|comp";
	uint64_t h = 5381;

	h = _hash_update(h, data, len);
	if (use_tag)
		h = _hash_update(h, cache_tag, sizeof(cache_tag) - 1);

	return h;
}


static int _mkdir_if_needed(const char *path)
{
	struct stat st;

	if (mkdir(path, 0755) == 0)
		return 1;

	if (errno != EEXIST)
		return 0;

	if (stat(path, &st) != 0)
		return 0;

	return S_ISDIR(st.st_mode);
}


/* Creates ~/.cache/polv */
static int _create_cache_dir(void)
{
	const char *home = getenv("HOME");
	char cache[PATH_MAX], polv_cache[PATH_MAX];

	if (!home)
		return 0;

	if (snprintf(cache, sizeof(cache), "%s/.cache", home) >= (int) sizeof(cache))
		return 0;

	if (snprintf(polv_cache, sizeof(polv_cache), "%s/.cache/polv", home) >= (int) sizeof(polv_cache))
		return 0;

	if (!_mkdir_if_needed(cache))
		return 0;

	if (!_mkdir_if_needed(polv_cache))
		return 0;

	return 1;
}


/* Reads a file and outputs a hash */
static POLVCoreResult _hash_file(const char *filename, uint64_t *hash)
{
	void *data;
	size_t size;
	POLVCoreResult res;

	if (!filename || !hash)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	res = read_file(filename, &data, &size, 1);
	if (res != POLV_CORE_SUCCESS)
		return res;

	*hash = _shader_hash(data, size, 0);
	free(data);

	return POLV_CORE_SUCCESS;
}


static int _make_cache_path(char *path, size_t path_size, const char *home,
                            uint64_t hash, const char *suffix)
{
	int n = snprintf(path, path_size, "%s/.cache/polv/%016" PRIx64 "%s",
	                 home, hash, suffix);
	return n >= 0 && (size_t) n < path_size;
}


static int _read_cache_key(const char *path, uint64_t *hash)
{
	FILE *fp;
	int ret;

	fp = fopen(path, "r");
	if (!fp)
		return 0;

	ret = fscanf(fp, "%16" SCNx64, hash) == 1;
	fclose(fp);

	return ret;
}


static int _write_cache_key(const char *path, const char *home, uint64_t hash)
{
	char tmp_path[PATH_MAX];
	FILE *fp;
	int fd, ok, n;

	n = snprintf(tmp_path, sizeof(tmp_path),
	             "%s/.cache/polv/.key-XXXXXX", home);
	if (n < 0 || (size_t) n >= sizeof(tmp_path))
		return 0;

	fd = mkstemp(tmp_path);
	if (fd < 0)
		return 0;

	fp = fdopen(fd, "w");
	if (!fp)
	{
		close(fd);
		unlink(tmp_path);
		return 0;
	}

	ok = fprintf(fp, "%016" PRIx64 "\n", hash) > 0;
	if (fclose(fp) != 0)
		ok = 0;

	if (!ok)
	{
		unlink(tmp_path);
		return 0;
	}

	if (rename(tmp_path, path) != 0)
	{
		unlink(tmp_path);
		return 0;
	}

	return 1;
}


static int _glslang_compile_comp(const char *src, const char *dst)
{
	int pipefd[2];
	pid_t pid;
	char *output = NULL;
	size_t output_len = 0;
	int status;

	/* Create the pipe for writing glslangValidator output */
	if (pipe(pipefd) < 0)
		return 0;

	/* Spawn a child process for executing the compilation command */
	pid = fork();
	if (pid == 0)
	{
		close(pipefd[0]); // child doesn't read

		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);

		execlp("glslangValidator", "glslangValidator", "-V", "-S", "comp",
		       src,  "-o", dst, (char *) NULL);
		_exit(127);
	}

	if (pid < 0)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return 0;
	}

	close(pipefd[1]); // parent doesn't write

	/* I am the parent, read child info */
	int output_failed = 0;

	for (;;)
	{
		char buf[1024], *tmp;
		ssize_t n = read(pipefd[0], buf, sizeof(buf));

		if (n <= 0)
			break;

		if (output_failed)
			continue;

		tmp = realloc(output, output_len + (size_t) n + 1);
		if (!tmp)
		{
			free(output);
			output = NULL;
			output_len = 0;
			output_failed = 1;
			continue;
		}

		output = tmp;
		memcpy(output + output_len, buf, (size_t) n);
		output_len += (size_t) n;
		output[output_len] = '\0';
	}

	close(pipefd[0]); // Parent has read, close the fd

	if (waitpid(pid, &status, 0) < 0)
	{
		free(output);
		return 0;
	}

	/* Print the error only if status != 0 */
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
	{
		if (output && output_len)
			fprintf(stderr, "[polv_core] error:%s: compilation failed:\n%s", src, output);
		free(output);
		unlink(dst);
		return 0;
	}

	free(output);

	return 1;
}


/* 
 * This creates a shader given its source filename (*.comp).
 * If from_string == 1, it means that we are called by 
 * polvc_kernel_shader_new_from_string, therefore we should
 * delete the .comp file after the compilation.
 */
int polvc_kernels_shader_new_from_glsl(POLVCoreDevice *dev,
                                       const char *shader_source_filename,
                                       int from_string)
{
	const char *home;
	char *source;
	char key_path[PATH_MAX], spv_path[PATH_MAX], tmp_path[PATH_MAX];
	size_t source_size;
	uint64_t source_hash, spv_hash, cached_spv_hash, existing_hash;
	POLVCoreResult res;
	int fd, ret;

	if (!dev || !shader_source_filename)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	res = read_file_str(shader_source_filename, &source, &source_size);
	if (res != POLV_CORE_SUCCESS)
		return res;

	/*
	 * Source hash is used only as a lookup key.
	 * Include POLV/glslang compilation configuration in this hash.
	 */
	source_hash = _shader_hash(source, source_size, 1);
	free(source);

	/*
	 * Cache is turned off (POLV_CACHE=0)
	 * GLSL -> /tmp/polv-XXXXXX.spv -> create shader -> delete
	 */
	if (!_cache_enabled())
	{
		strcpy(tmp_path, "/tmp/polv-XXXXXX.spv");

		fd = mkstemps(tmp_path, 4);
		if (fd < 0)
			return POLV_CORE_ERROR_SHADER;

		close(fd);

		if (!_glslang_compile_comp(shader_source_filename, tmp_path))
		{
			unlink(tmp_path);
			return POLV_CORE_ERROR_SHADER;
		}

		ret = polvc_kernels_shader_new_from_spv_file(dev, tmp_path, 0);
		unlink(tmp_path);

		return ret;
	}

	/*
	 * Cache is turned on:
	 * <source hash>.key -> <SPIR-V hash>
	 * <SPIR-V hash>.spv -> actual binary
	 */
	home = getenv("HOME");
	if (!home)
		return POLV_CORE_ERROR_SHADER;

	if (!_create_cache_dir())
		return POLV_CORE_ERROR_SHADER;

	if (!_make_cache_path(key_path, sizeof(key_path), home, source_hash, ".key"))
		return POLV_CORE_ERROR_SHADER;

	/*
	 * First try the source -> SPIR-V mapping.
	 * Identical source -> skip compilation.
	 */
	if (_read_cache_key(key_path, &cached_spv_hash))
	{
		if (_make_cache_path(spv_path, sizeof(spv_path), home,
		                     cached_spv_hash, ".spv") &&
		    access(spv_path, F_OK) == 0)
			return polvc_kernels_shader_new_from_spv_file(dev, spv_path, 0);
	}

	/*
	 * No source-cache hit. Compile into a temporary SPIR-V file
	 * inside ~/.cache/polv.
	 */
	if (snprintf(tmp_path, sizeof(tmp_path),
	             "%s/.cache/polv/.tmp-XXXXXX.spv", home) >= (int) sizeof(tmp_path))
		return POLV_CORE_ERROR_SHADER;

	fd = mkstemps(tmp_path, 4);
	if (fd < 0)
		return POLV_CORE_ERROR_SHADER;
	close(fd);

	if (!_glslang_compile_comp(shader_source_filename, tmp_path))
	{
		unlink(tmp_path); // delete temporary file
		return POLV_CORE_ERROR_SHADER;
	}

	/*
	 * Hash the generated SPIR-V. Different GLSL sources which generate
	 * identical SPIR-V now resolve to exactly the same cached file.
	 */
	res = _hash_file(tmp_path, &spv_hash);
	if (res != POLV_CORE_SUCCESS)
	{
		unlink(tmp_path);
		return res;
	}

	if (!_make_cache_path(spv_path, sizeof(spv_path), home, spv_hash, ".spv"))
	{
		unlink(tmp_path);
		return POLV_CORE_ERROR_SHADER;
	}

	/*
	 * If an identical cached SPIR-V already exists, discard our temporary
	 * copy. Also verify its contents in case the cache file was corrupted.
	 */
	if (access(spv_path, F_OK) == 0 &&
	    _hash_file(spv_path, &existing_hash) == POLV_CORE_SUCCESS &&
	    existing_hash == spv_hash)
	{
		unlink(tmp_path);
	}
	else
	{
		/*
		 * tmp_path and spv_path are on the same filesystem, so rename()
		 * is atomic and cannot fail with EXDEV.
		 */
		if (rename(tmp_path, spv_path) != 0)
		{
			unlink(tmp_path);
			return POLV_CORE_ERROR_SHADER;
		}
	}

	/*
	 * source hash -> SPIR-V hash
	 *
	 * Failure to write this mapping does not make the shader unusable;
	 * it only means we'll have to compile it again next time.
	 */
	_write_cache_key(key_path, home, spv_hash);

	ret = polvc_kernels_shader_new_from_spv_file(dev, spv_path, 0);

	if (from_string)
		unlink(shader_source_filename);

	return ret;
}


/* Creates a shader given its contents. */
int polvc_kernels_shader_new_from_string(POLVCoreDevice *dev,
                                         const char *shader_str,
                                         int ignore)
{
	char shader_path[] = "/tmp/polv-XXXXXX.comp"; // template
	size_t len;
	FILE *fp;
	int fd, ret;

	(void) ignore;

	if (!dev || !shader_str)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	fd = mkstemps(shader_path, 5);
	if (fd < 0)
		return POLV_CORE_ERROR_SHADER;

	fp = fdopen(fd, "wb");
	if (!fp)
	{
		close(fd);
		unlink(shader_path);
		return POLV_CORE_ERROR_SHADER;
	}

	len = strlen(shader_str);

	if (fwrite(shader_str, 1, len, fp) != len)
	{
		fclose(fp);
		unlink(shader_path);
		return POLV_CORE_ERROR_SHADER;
	}

	if (fclose(fp) != 0)
	{
		unlink(shader_path);
		return POLV_CORE_ERROR_SHADER;
	}

	ret = polvc_kernels_shader_new_from_glsl(dev, shader_path, 1);

	return ret;
}


/* Adds a kernel to the list. */
void polvc_kernels_link(POLVCoreContext *context, POLVCoreKernel *kernel)
{
	kernel->prev = NULL;
	kernel->next = context->kernels;

	if (context->kernels)
		context->kernels->prev = kernel;

	context->kernels = kernel;
}


/* Removes a kernel from the list. */
static void kernel_unlink(POLVCoreKernel *kernel)
{
	POLVCoreContext *context;

	if (!kernel || !kernel->owner)
		return;

	context = kernel->owner;

	if (kernel->prev)
		kernel->prev->next = kernel->next;
	else if (context->kernels == kernel)
		context->kernels = kernel->next;

	if (kernel->next)
		kernel->next->prev = kernel->prev;

	kernel->prev = NULL;
	kernel->next = NULL;
}


/* Destroys a specific kernel; removes it from the list if unlink == 1. */
void polvc_kernels_destroy_kernel(POLVCoreKernel *kernel, int unlink)
{
	POLVCoreContext *context;
	POLVCoreDevice *dev;
	struct POLVCorePipeline_ *pipeline, *next;

	if (!kernel || !kernel->owner)
		return;

	context = kernel->owner;
	dev = context->p_device;

	if (unlink)
		kernel_unlink(kernel);

	/* (1) Destroy all the pipelines */
	pipeline = kernel->pipelines;
	while (pipeline)
	{
		next = pipeline->next;

		if (pipeline->compute_pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(dev->device, pipeline->compute_pipeline, NULL);

		free(pipeline);
		pipeline = next;
	}
	kernel->pipelines = NULL;

	/* (2) Destroy PL, DP and DSL */
	if (kernel->pipeline_layout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(dev->device, kernel->pipeline_layout, NULL);

	if (kernel->descriptor_pool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(dev->device, kernel->descriptor_pool, NULL);

	if (kernel->descriptor_set_layout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(dev->device, kernel->descriptor_set_layout, NULL);

	kernel->owner = NULL;
	kernel->shader = NULL;
	free(kernel);
}


/* Creates a descriptor set layout for a given kernel. */
POLVCoreResult polvc_kernels_create_descriptor_set_layout(POLVCoreKernel *kernel)
{
	POLVCoreDevice *dev;
	VkDescriptorSetLayoutBinding *bindings;
	VkDescriptorSetLayoutCreateInfo ci;
	int i;

	if (!kernel || !kernel->owner || kernel->nargs <= 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	bindings = (VkDescriptorSetLayoutBinding *) 
		calloc((size_t) kernel->nargs, sizeof(*bindings));
	if (!bindings)
		return POLV_CORE_ERROR_OUT_OF_MEMORY;

	for (i = 0; i < kernel->nargs; ++i)
	{
		bindings[i].binding = (uint32_t) i;
		bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[i].descriptorCount = 1;
		bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	}

	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.bindingCount = (uint32_t) kernel->nargs;
	ci.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(dev->device, &ci, NULL,
	                                &kernel->descriptor_set_layout) != VK_SUCCESS)
	{
		free(bindings);
		return POLV_CORE_ERROR_VULKAN;
	}

	free(bindings);
	return POLV_CORE_SUCCESS;
}


/* Creates a pipeline layout for a given kernel. */
POLVCoreResult polvc_kernels_create_pipeline_layout(POLVCoreKernel *kernel)
{
	POLVCoreDevice *dev;
	VkPipelineLayoutCreateInfo ci;

	if (!kernel || !kernel->owner)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	ci.setLayoutCount = 1;
	ci.pSetLayouts = &kernel->descriptor_set_layout;

	if (vkCreatePipelineLayout(dev->device, &ci, NULL,
	                           &kernel->pipeline_layout) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}


/* Creates a descriptor pool for a given kernel. */
POLVCoreResult polvc_kernels_create_descriptor_pool(POLVCoreKernel *kernel)
{
	POLVCoreDevice *dev;
	VkDescriptorPoolSize pool_size;
	VkDescriptorPoolCreateInfo ci;

	if (!kernel || !kernel->owner || kernel->nargs <= 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	memset(&pool_size, 0, sizeof(pool_size));
	pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size.descriptorCount = (uint32_t) kernel->nargs;

	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = 1;
	ci.poolSizeCount = 1;
	ci.pPoolSizes = &pool_size;

	if (vkCreateDescriptorPool(dev->device, &ci, NULL,
	                           &kernel->descriptor_pool) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}


/* Allocates a descriptor set for a given kernel. */
POLVCoreResult polvc_kernels_allocate_descriptor_set(POLVCoreKernel *kernel)
{
	POLVCoreDevice *dev;
	VkDescriptorSetAllocateInfo ai;

	if (!kernel || !kernel->owner)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	memset(&ai, 0, sizeof(ai));
	ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	ai.descriptorPool = kernel->descriptor_pool;
	ai.descriptorSetCount = 1;
	ai.pSetLayouts = &kernel->descriptor_set_layout;

	if (vkAllocateDescriptorSets(dev->device, &ai,
	                             &kernel->descriptor_set) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}


/* Creates a compute pipeline for a given kernel and its group dimensions. */
static POLVCoreResult kernel_create_pipeline(POLVCoreKernel *kernel, uint32_t group_x, 
                                             uint32_t group_y, uint32_t group_z,
                                             VkPipeline *compute_pipeline)
{
	POLVCoreDevice *dev;
	struct POLVCorePipeline_ *pipeline;
	VkSpecializationMapEntry spec_entries[3];
	VkSpecializationInfo spec_info;
	VkPipelineShaderStageCreateInfo stage;
	VkComputePipelineCreateInfo ci;
	uint32_t group_size[3];
	int i, nsizes = 3;

	if (!kernel || !kernel->owner || !kernel->shader || !compute_pipeline)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	pipeline = (struct POLVCorePipeline_ *) calloc(1, sizeof(*pipeline));
	if (!pipeline)
		return POLV_CORE_ERROR_OUT_OF_MEMORY;

	/* local_size_x_id, local_size_y_id and local_size_z_id */
	group_size[0] = group_x;
	group_size[1] = group_y;
	group_size[2] = group_z;

	for (i = 0; i < nsizes; i++)
	{
		spec_entries[i].constantID = i;
		spec_entries[i].offset = i * sizeof(uint32_t);
		spec_entries[i].size = sizeof(uint32_t);
	}

	memset(&spec_info, 0, sizeof(spec_info));
	spec_info.mapEntryCount = (uint32_t) nsizes;
	spec_info.pMapEntries = spec_entries;
	spec_info.dataSize = sizeof(group_size);
	spec_info.pData = group_size;

	memset(&stage, 0, sizeof(stage));
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module = kernel->shader->compute_shader_module;
	stage.pName = "main";
	stage.pSpecializationInfo = &spec_info;

	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	ci.stage = stage;
	ci.layout = kernel->pipeline_layout;

	if (vkCreateComputePipelines(dev->device, VK_NULL_HANDLE, 1, &ci, NULL,
	                             &pipeline->compute_pipeline) != VK_SUCCESS)
	{
		free(pipeline);
		return POLV_CORE_ERROR_VULKAN;
	}

	pipeline->group_x = group_x;
	pipeline->group_y = group_y;
	pipeline->group_z = group_z;
	pipeline->next = kernel->pipelines;
	kernel->pipelines = pipeline;

	*compute_pipeline = pipeline->compute_pipeline;

	return POLV_CORE_SUCCESS;
}


/* Returns the compute pipeline of a given kernel, if its dimensions match,
 * otherwise it creates one.
 */
POLVCoreResult polvc_kernels_get_pipeline(POLVCoreKernel *kernel, uint32_t group_x, 
                                          uint32_t group_y, uint32_t group_z,
                                          VkPipeline *compute_pipeline)
{
	struct POLVCorePipeline_ *pipeline;

	if (!kernel || !compute_pipeline)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	for (pipeline = kernel->pipelines; pipeline; pipeline = pipeline->next)
	{
		if (pipeline->group_x == group_x &&
			pipeline->group_y == group_y &&
			pipeline->group_z == group_z)
		{
			*compute_pipeline = pipeline->compute_pipeline;
			return POLV_CORE_SUCCESS;
		}
	}

	/* Pipeline not found, create one */
	return kernel_create_pipeline(kernel, group_x, group_y, group_z,
	                              compute_pipeline);
}
