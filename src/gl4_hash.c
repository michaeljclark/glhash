/*
 * gl4_hash
 */

#undef NDEBUG
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

#include <stdbool.h>
#include <inttypes.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "sha256.h"
#include "gl2_util.h"

typedef unsigned char u8;

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

static const char* comp_shader_glsl_filename = SRC_PATH PATH_SEP "sha256.comp";
static const char* comp_shader_spir_filename = BIN_PATH PATH_SEP "sha256.comp.spv";

static bool help = 0;
static int use_spir = 0;
static GLuint program;
static GLuint ssbo;

// test_001 - e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
static const u8 test_001_input[] =
    "";
static const u8 test_001_sha256[32] = {
    0xe3, 0xb0, 0xc4, 0x42,  0x98, 0xfc, 0x1c, 0x14,  0x9a, 0xfb, 0xf4, 0xc8,
    0x99, 0x6f, 0xb9, 0x24,  0x27, 0xae, 0x41, 0xe4,  0x64, 0x9b, 0x93, 0x4c,
    0xa4, 0x95, 0x99, 0x1b,  0x78, 0x52, 0xb8, 0x55 };

// test_002 - ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
static const u8 test_002_input[] =
    "abc";
static const u8 test_002_sha256[32] = {
    0xba, 0x78, 0x16, 0xbf,  0x8f, 0x01, 0xcf, 0xea,  0x41, 0x41, 0x40, 0xde,
    0x5d, 0xae, 0x22, 0x23,  0xb0, 0x03, 0x61, 0xa3,  0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61,  0xf2, 0x00, 0x15, 0xad };

// test_003 - 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1
static const u8 test_003_input[] =
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
static const u8 test_003_sha256[32] = {
    0x24, 0x8d, 0x6a, 0x61,  0xd2, 0x06, 0x38, 0xb8,  0xe5, 0xc0, 0x26, 0x93,
    0x0c, 0x3e, 0x60, 0x39,  0xa3, 0x3c, 0xe4, 0x59,  0x64, 0xff, 0x21, 0x67,
    0xf6, 0xec, 0xed, 0xd4,  0x19, 0xdb, 0x06, 0xc1 };

// test_004 - cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1
static const u8 test_004_input[] =
    "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
    "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
static const u8 test_004_sha256[32] = {
    0xcf, 0x5b, 0x16, 0xa7,  0x78, 0xaf, 0x83, 0x80,  0x03, 0x6c, 0xe5, 0x9e,
    0x7b, 0x04, 0x92, 0x37,  0x0b, 0x24, 0x9b, 0x11,  0xe8, 0xf0, 0x7a, 0x51,
    0xaf, 0xac, 0x45, 0x03,  0x7a, 0xfe, 0xe9, 0xd1 };

#define array_items(arr) (sizeof(arr)/sizeof(arr[0]))

struct { const u8 *input; const u8 *exemplar; }
tests[] = {
    { test_001_input, test_001_sha256 },
    { test_002_input, test_002_sha256 },
    { test_003_input, test_003_sha256 },
    { test_004_input, test_004_sha256 },
};

typedef struct {
    uint chain[8];
    uint block[16];
    uint offset;
    uint length;
    uint data[];
} SSBO_t;

void test_sha256_gpu(u8 *result, const u8 *input, size_t length)
{
    SSBO_t *ssbo_ptr, *ssbo_map;
    size_t ssbo_len;

    ssbo_len = sizeof(SSBO_t) + ((length+3)&~3);
    ssbo_ptr = calloc(1, ssbo_len);
    ssbo_ptr->length = length;
    memcpy(ssbo_ptr->data, input, length);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo_len, ssbo_ptr, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glUseProgram(program);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
    glDispatchCompute(1, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    ssbo_map = (SSBO_t *)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    memcpy(result, ssbo_map->chain, 32);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    free(ssbo_ptr);
}

void test_sha256_cpu(u8 *result, const u8 *input, size_t len)
{
    sha256_ctx ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, input, len);
    sha256_final(&ctx, result);
}

static u8 result1[32], result2[32], tbuf[256];

static char* to_hex(u8 *buf, size_t buf_len, const u8 *in, size_t in_len)
{
    for (intptr_t i = 0, o = 0; i < in_len; i++) {
        o+= snprintf(buf+o, buf_len - o, "%02" PRIx8, in[i]);
    }
    return buf;
}

void test_sha256(size_t i, const u8 *input, size_t len, const u8 exemplar[32])
{
    printf("test_%zu.exp = %s\n", i, to_hex(tbuf, sizeof(tbuf), exemplar, 32));
    test_sha256_cpu(result1, input, len);
    printf("test_%zu.cpu = %s\n", i, to_hex(tbuf, sizeof(tbuf), result1, 32));
    test_sha256_gpu(result2, input, len);
    printf("test_%zu.gpu = %s\n", i, to_hex(tbuf, sizeof(tbuf), result2, 32));
    assert(memcmp(result1, exemplar, 32) == 0);
    assert(memcmp(result2, exemplar, 32) == 0);
}

static void run_all_tests()
{
    for (size_t i = 0; i < array_items(tests); i++) {
        test_sha256(i, tests[i].input, strlen(tests[i].input), tests[i].exemplar);
    }
}

static void init()
{
    GLuint csh;

    if (use_spir) {
        csh = compile_shader(GL_COMPUTE_SHADER, comp_shader_spir_filename);
    } else {
        csh = compile_shader(GL_COMPUTE_SHADER, comp_shader_glsl_filename);
    }
    program = link_program(&csh, 1, NULL);

    glGenBuffers(1, &ssbo);
}

static void print_help(int argc, char **argv)
{
    fprintf(stderr,
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  -s, --spir                         use SPIR-V shaders\n"
        "  -h, --help                         command line help\n",
        argv[0]);
}

static int match_opt(const char *arg, const char *opt, const char *longopt)
{
    return strcmp(arg, opt) == 0 || strcmp(arg, longopt) == 0;
}

static void parse_options(int argc, char **argv)
{
    int i = 1;
    while (i < argc) {
        if (match_opt(argv[i], "-s", "--spir")) { use_spir++;  i++; }
        else if (match_opt(argv[i], "-h", "--help")) { help++; i++; }
        else {
            fprintf(stderr, "error: unknown option: %s\n", argv[i]);
            help++;
            break;
        }
    }

    if (help) {
        print_help(argc, argv);
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    GLFWwindow* window;

    parse_options(argc, argv);
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(64, 64, argv[0], NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL();
    init();
    run_all_tests();
    glfwTerminate();

    exit(EXIT_SUCCESS);
}

