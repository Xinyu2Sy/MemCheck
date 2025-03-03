#include <stdlib.h>


void report();

extern "C" {
    void start_check_log();
    void start_check_log();
}

void memory_training(int cycles) {
    start_check_log();
    for (int i = 0; i < cycles; ++i) {
        // 每次循环产生 2 次泄漏
        void* p1 = malloc(512);  // 打印
        int* p2 = new int(42);    // 打印
        
        // 正常释放部分
        void* p3 = malloc(64);
        free(p3);                // 不打印
        int* p4 = new int(88);
        delete p4;              // 不打印
    }
    start_check_log();
    report();
}

int main() {
    // 训练前分配（不打印）
    void* p_pre = malloc(1024);
    
    // 执行训练
    memory_training(10);  // 触发打印
    
    // 训练后分配（不打印）
    int* p_post = new int(999);
    
    // 释放部分内存
    free(p_pre);
    
    // 预期泄漏：10次循环 × 2次分配 + 1次new = 21个泄漏
    return 0;
}