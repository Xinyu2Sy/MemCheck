# Memory Leak Detector 🔍

一个轻量级、高效的内存泄漏检测工具，通过拦截内存操作函数（`malloc/free`、`new/delete`）实现实时追踪，适用于C/C++项目的内存问题定位。

![Demo](https://via.placeholder.com/800x300?text=内存泄漏检测流程图) <!-- 建议替换实际截图 -->

## 特性亮点 ✨

- ​**全内存操作覆盖**：支持 `malloc/free` 及 C++ 的 `new/delete` 系列操作
- ​**智能模式过滤**：通过 `REPORT_TRAINING_ONLY` 模式区分训练代码与系统内存操作
- ​**泄露详情记录**：精确到字节级的内存分配大小追踪
- ​**零代码侵入**：通过预加载库实现无痕检测
- ​**高性能设计**：反向遍历算法实现 O(1) 释放匹配

## 实现原理 🛠️

### 核心机制
通过 `dlsym(RTLD_NEXT)` 动态劫持内存管理函数：
```c
// 获取原始函数指针
real_malloc = dlsym(RTLD_NEXT, "malloc"); 

// 自定义 malloc 包装器
void* malloc(size_t size) {
    void* ptr = real_malloc(size);
    record_allocation(ptr, size);  // 记录分配信息
    return ptr;
}
```

### 关键技术
1. ​**内存登记簿**​  
   使用静态数组存储内存记录，避免动态分配开销：
   ```c
   struct MemRecord {
       void* ptr;
       size_t size;
       bool in_training;
   };
   ```

2. ​**反向遍历优化**​  
   释放时反向扫描数组实现快速匹配：
   ```c
   for (size_t i = leak_count; i-- > 0; ) {
       if (leaks[i].ptr == ptr) {
           leaks[i] = leaks[--leak_count];
           break;
       }
   }
   ```

3. ​**上下文感知**​  
   通过全局标记过滤非关键代码：
   ```c
   extern "C" {
       void start_check_log();  // 开启检测模式
       void stop_check_log();   // 关闭检测模式
   }
   ```

## 快速开始 🚀

### 编译与运行
```bash
# 编译动态库及测试程序
make -j

# 运行测试（显示完整报告）
LD_PRELOAD=./libmymalloc.so ./memory_leak
```

### 示例输出
```text
=== 内存泄漏报告 ===
地址: [0x7F8A1B002E50]  大小: 128 bytes
地址: [0x7F8A1B045D20]  大小: 2048 bytes
```

## 高级配置 ⚙️

### 编译选项
| 宏定义                 | 描述                     | 默认值   |
|-----------------------|-------------------------|---------|
| `REPORT_TRAINING_ONLY` | 仅显示指定函数内存泄漏   | `false` |
| `MAX_LEAKS`           | 最大追踪数量             | 100000  |

## 架构图解 📐
```c
+---------------------+      +---------------------+
|   Your Application  |      |   Original malloc   |
|                     |      |      (libc.so)      |
+----------+----------+      +----------+----------+
           |                            ^
           | 1. malloc(size)            |
           +----------------------------+
           |
           v
+----------+----------+
|  Wrapped malloc     |
|  - 记录分配地址/大小  |
|  - 条件打印调试信息  |
+---------------------+
```

## 许可协议 📄
[MIT License](LICENSE) © 2024 [Xinyu Lv]
