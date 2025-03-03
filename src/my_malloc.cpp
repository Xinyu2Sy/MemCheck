#include <dlfcn.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>

#define MAX_LEAKS 100000

// 控制宏：定义则只显示训练函数中的泄漏，注释则显示全部
//#define REPORT_TRAINING_ONLY

// 增强的内存记录结构（新增size字段）
struct MemRecord {
    void* ptr;
    size_t size;    // 记录分配大小
    bool in_training;
};

// 线程局部训练模式标记
static bool check_mode = false;

// 原始函数指针
static void* (*real_malloc)(size_t) = nullptr;
static void (*real_free)(void*) = nullptr;
static void* (*real_new)(size_t) = nullptr;
static void* (*real_new_arr)(size_t) = nullptr;
static void (*real_delete)(void*) = nullptr;
static void (*real_delete_arr)(void*) = nullptr;

// 泄漏记录数组
static MemRecord leaks[MAX_LEAKS];
static size_t leak_count = 0;

// 外部控制接口
extern "C" {
    void start_check_log() { check_mode = true; }
    void start_check_log()  { check_mode = false; }
}

// 初始化原始函数
__attribute__((constructor)) static void init() {
    real_malloc = reinterpret_cast<void*(*)(size_t)>(dlsym(RTLD_NEXT, "malloc"));
    real_free = reinterpret_cast<void(*)(void*)>(dlsym(RTLD_NEXT, "free"));
    real_new = reinterpret_cast<void*(*)(size_t)>(dlsym(RTLD_NEXT, "_Znwm"));     // operator new
    real_new_arr = reinterpret_cast<void*(*)(size_t)>(dlsym(RTLD_NEXT, "_Znam")); // operator new[]
    real_delete = reinterpret_cast<void(*)(void*)>(dlsym(RTLD_NEXT, "_ZdlPv"));   // operator delete
    real_delete_arr = reinterpret_cast<void(*)(void*)>(dlsym(RTLD_NEXT, "_ZdaPv")); // operator delete[]
}

// 增强的内存记录函数（增加size参数）
static void record_allocation(void* ptr, size_t size) {
    if (leak_count < MAX_LEAKS) {
        leaks[leak_count].ptr = ptr;
        leaks[leak_count].size = size;
        leaks[leak_count].in_training = check_mode;
        leak_count++;
    }
}

/**********************​ 完整的内存操作包装器 ​**********************/

// malloc包装器
extern "C" void* malloc(size_t size) {
    if (!real_malloc) return nullptr;
    void* ptr = real_malloc(size);
    
    record_allocation(ptr, size);
    if (check_mode) {
        fprintf(stderr, "[Training] malloc(%zu) => %p\n", size, ptr);
    }
    return ptr;
}

// free包装器
extern "C" void free(void* ptr) {
    if (!ptr || !real_free) return;
    
    if (check_mode) {
        fprintf(stderr, "[Training] free(%p)\n", ptr);
    }
    
    // 反向遍历提高删除效率
    for (size_t i = leak_count; i-- > 0; ) {
        if (leaks[i].ptr == ptr) {
            leaks[i] = leaks[--leak_count];
            break;
        }
    }
    real_free(ptr);
}

// operator new包装器
void* operator new(size_t size) {
    if (!real_new) return nullptr;
    void* ptr = real_new(size);
    
    record_allocation(ptr, size);
    if (check_mode) {
        fprintf(stderr, "[Training] new(%zu) => %p\n", size, ptr);
    }
    return ptr;
}

// operator new[]包装器
void* operator new[](size_t size) {
    if (!real_new_arr) return nullptr;
    void* ptr = real_new_arr(size);
    
    record_allocation(ptr, size);
    if (check_mode) {
        fprintf(stderr, "[Training] new[](%zu) => %p\n", size, ptr);
    }
    return ptr;
}

// operator delete包装器
void operator delete(void* ptr) noexcept {
    if (!ptr || !real_delete) return;
    
    if (check_mode) {
        fprintf(stderr, "[Training] delete(%p)\n", ptr);
    }
    
    for (size_t i = leak_count; i-- > 0; ) {
        if (leaks[i].ptr == ptr) {
            leaks[i] = leaks[--leak_count];
            break;
        }
    }
    real_delete(ptr);
}

// operator delete[]包装器
void operator delete[](void* ptr) noexcept {
    if (!ptr || !real_delete_arr) return;
    
    if (check_mode) {
        fprintf(stderr, "[Training] delete[](%p)\n", ptr);
    }
    
    for (size_t i = leak_count; i-- > 0; ) {
        if (leaks[i].ptr == ptr) {
            leaks[i] = leaks[--leak_count];
            break;
        }
    }
    real_delete_arr(ptr);
}

/**********************​ 完整的泄漏报告生成 ​**********************/
void report() {
#ifdef REPORT_TRAINING_ONLY
    const char* header = "\n=== 训练函数内存泄漏 ===\n";
#else
    const char* header = "\n=== 全部内存泄漏 ===\n";
#endif
    write(STDERR_FILENO, header, 30);
    
    char buf[256];
    for (size_t i = 0; i < leak_count; ++i) {
#ifdef REPORT_TRAINING_ONLY
        if (!leaks[i].in_training) continue;
#endif
        // 生成带地址和大小的报告条目
        int len = snprintf(
            buf, 
            sizeof(buf), 
            "地址: [0x%016lX]\t大小: %6zu bytes\n",  // 对齐大小显示
            reinterpret_cast<uintptr_t>(leaks[i].ptr),
            leaks[i].size
        );
        
        if (len > 0) {
            write(STDERR_FILENO, buf, len);
        }
    }
}