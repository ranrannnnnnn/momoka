# 11.5 项目 Code Review 报告

- **评审范围**：`user/` 全部业务代码 + `Core/Src` 中的 `main.c` / `stm32f1xx_it.c` / `stm32f1xx_hal_msp.c` + `CMakeLists.txt` / `cmake/` / `11.5.ioc`
- **评审方式**：静态通读 + 跨文件调用关系核对 + 对 math/bit 宏做主机侧编译实测（GCC 13.3 / `-std=gnu11`）
- **评审时间**：2025-11-13
- **结论**：代码处于**重构半成品状态**。模块划分和命名体系（`_method`/`_tool`/`getter`/`setter`）已成型，但工程**当前无法编译通过**；即使修好编译，业务入口、电机闭环、状态机三块仍是断的。整体属于"骨架正确、关节未接"。

---

## 0. 总体评价

**做得好的地方：**

1. 目录分层清晰：`config / common / hardware / connectivity / interrupt`，模块边界基本合理。
2. 统一命名风格：模块私有函数 `xxx_method`、通用工具 `xxx_tool`、属性访问 `xxx_getter/setter`，可读性好。
3. 软 I2C 抽象（`I2C` 结构体携带 7 位地址）比旧版硬编码 `0x78` 前进了一大步。
4. OLED 驱动已完成从"直写 GPIO"到"依赖 `I2C` 抽象层"的迁移，`String` 结构体 + 显示字符串/数字接口成型。
5. PID 采用增量式 + 微分先行 + 微分低通，算法结构合理；采样周期分频（`ts` ms）设计正确。
6. 工程化意识好：CMakePresets、自动查找 arm-none-eabi 工具链、链接后自动生成 hex/bin、OpenOCD 烧录目标。

**主要问题（按严重程度）：**

| 等级 | 数量 | 典型问题 |
|---|---|---|
| 🔴 P0 致命 | 7 | 程序入口没接、编译/链接错误、编码器未启动、状态机缺失、PWM 换向直通风险、中断内阻塞 |
| 🟠 P1 严重 | 12 | KEYNUMBER 与按键数不符、齿轮表顺序反了、TIM3 中断没接、OLED 在 ISR 里刷、显示越界、I2C 无 ACK/时序超速、初始化函数大量未调用 |
| 🟡 P2 建议 | 15+ | 头文件规范、死代码、魔数、命名拼写、宏边界、仓库换行符噪音等 |

---

## 1. 🔴 P0 致命问题（编译不过 / 上电没反应 / 有炸机风险）

### P0-1 程序入口断裂：`appMain` / `app_main` 名字对不上，主循环为空

- [Core/Src/main.c:60](../Core/Src/main.c#L60) 只声明了 `void app_main(void);`，**全工程没有任何地方调用它**；[Core/Src/main.c:107-112](../Core/Src/main.c#L107-L112) 的 `while(1)` 是空的。
- [user/main.c:34](../user/main.c#L34) 定义的是 `int appMain(void)`（`A` 大写、返回 int），与 `app_main` 不是同一个符号。

**后果**：烧录后只完成 CubeMX 外设初始化，`hardware_init()` 根本不会执行——OLED 不亮、按键不响应、电机无输出，看起来像"死机"。

**修复**：

```c
/* Core/Src/main.c  USER CODE BEGIN 2 */
app_main();          // 或把 user/main.c 改名 app_main 后调用

/* 注意：user/main.c 里 appMain 自带 while(1)，main() 里就不要再空转；
   也可以让 appMain 只做初始化、main 的 while(1) 里调 appMain_loop()。 */
```

```c
// user/main.c 建议改成
void app_main(void)              // 与 Core/main.c 声明一致
{
    hardware_init();
    hardware_initFinalize();
    initComplete();
    while (1) { /* 主循环：按键事件、OLED 刷新、状态监控都放这里 */ }
}
```

### P0-2 编译/链接直接失败（3 处）

1. **`motor.c` 里有一个字面量 `...`**：[motor.c:23](../user/hardware/motor/motor.c#L23)

   ```c
   Pid_init(&motor.pid, 1, 0, 2, 6, 0.05f, 3059, 539, 50, Motor_pwmOutput_method, ..., Motor_motorPwm_getter);
   ```
   `...` 不是 C 表达式 → 语法错误。`sampleFunc`（编码器采样函数）还没写，可先传 `NULL` 占位，但 PID ISR 在调用前必须判空（见 P0-3）。

2. **`oled.h` 与 `oled.c` 参数类型冲突**：[oled.h:6](../user/hardware/oled/oled.h#L6) 声明 `String* string`，[oled.c:132](../user/hardware/oled/oled.c#L132) 定义 `const String* string` —— 两者是**不同的指针类型**，不是顶层 const 可以忽略的情况 → `conflicting types for 'OLED_showString_method'`。
   - 修复：头文件改成 `const String* string;`（推荐），或定义去掉 `const`。

3. **CMake 源文件列表缺 `oled.c`**：[CMakeLists.txt:35-62](../CMakeLists.txt#L35-L62) 没有 `user/hardware/oled/oled.c`。
   - 而 [user/main.c:19](../user/main.c#L19) 调 `OLED_Init()`、[args_listener.c:27](../user/common/args_listener/args_listener.c#L27) 调 `OLED_showString_method()` → 链接时 `undefined reference`。
   - 修复：`add_executable` 列表补上 `user/hardware/oled/oled.c`、`user/hardware/oled/oled.h`（以及 `fortLib.h` 仅作为头文件不必须列出）。

> 当前 `build/Debug` 里的 elf/hex 是 **11-12 12:52** 的旧产物（那时还没有 oled/string/i2c 模块），不能作为"能编译"的依据。

### P0-3 电机闭环三大关键环节缺失：编码器没启动、采样函数没写、初始化没调用

- **编码器从未启动**：全工程搜索不到 `HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL)`；`__HAL_TIM_GET_COUNTER(&htim2)` 也没有任何读取代码。[motor_encoder.c](../user/hardware/motor/motor_encoder.c) 只有一行 `#include`，是空文件。
- **PID 反馈函数缺失**：`Pid_init` 的 `sampleFunc` 参数就是 P0-2 里那个 `...` 占位。
- **初始化函数全部没被调用**（grep 验证）：

| 函数 | 定义位置 | 是否被调用 |
|---|---|---|
| `Motor_init` | [motor.c:20](../user/hardware/motor/motor.c#L20) | ❌ 从未调用 |
| `Motor_harwareInit` | [motor.c:30](../user/hardware/motor/motor.c#L30) | ❌ 从未调用 |
| `Motor_hardwareFinalize` | [motor.c:38](../user/hardware/motor/motor.c#L38) | ❌ 从未调用 |
| `Motor_statusCheck_method` | [motor.c:45](../user/hardware/motor/motor.c#L45) | ❌ 从未调用 |
| `Pid_hardwareInit` | [pid.c:16](../user/common/pid/pid.c#L16) | ❌ 从未调用 |
| `Pid_hardwareInitFinalize` | [pid.c:28](../user/common/pid/pid.c#L28) | ❌ 从未调用 |
| `ArgListener_init` | [args_listener.c:20](../user/common/args_listener/args_listener.c#L20) | ❌ 从未调用 |
| `Key_hardwareInit` | [key.c:45](../user/hardware/key/key.c#L45) | ✅ [user/main.c:15](../user/main.c#L15) |
| `OLED_Init` | [oled.c:17](../user/hardware/oled/oled.c#L17) | ✅ [user/main.c:19](../user/main.c#L19) |

**后果**：即使入口接通，电机也无法闭环——TIM1 的 OC 通道没启动（`Motor_hardwareFinalize`），TIM3 没按 0.5 s 配置（`Motor_harwareInit`），PWM 比较值永远为 0。

**建议的初始化顺序**（user/main.c）：

```c
void hardware_init(void) {
    LED_on_method(LED0);

    Key_init(&key0, KeyType_KEY0);          // 先注册，再开中断（现在是反的）
    Key_init(&key1, KeyType_KEY1);
    Key_init(&key2, KeyType_KEY2);

    OLED_Init();                            // 内部会 I2C_init(&i2c, 0x3C)
    Motor_init();                           // Pid_init(...) 需要先补 sampleFunc
    Motor_harwareInit();                    // 关 IR2104 + TIM3 0.5s
    Pid_hardwareInit();                     // 启动 TIM4 1ms 节拍（或统一由 BSP 启动）
    ArgListener_init();

    Key_hardwareInit();                     // 最后再开 TIM4 中断
}

void hardware_initFinalize(void) {
    Motor_hardwareFinalize();               // 启动 TIM1 PWM 输出
    // Pid_hardwareInitFinalize();          // 视设计决定是否需要
}
```

### P0-4 电机状态机整体缺失 + 多处死等循环

- `MotorStatus` 只有 `Motor_motorStatus_setter()`（[motor.c:149](../user/hardware/motor/motor.c#L149)）能改，而**这个 setter 从未被调用**（grep 验证）。`Motor_start_method()` 启动后也不置 `RUNNING`。
- 于是 `motor.status` 永远停在 `STOPPED`：
  - [key.c:146](../user/hardware/key/key.c#L146) 的"再按一次停止"分支永远进不去（永远走 start）；
  - `Motor_statusCheck_method()` 里 `while (motor.status != MotorStatus_RUNNING)` 等不到目标状态，只能等 TIM3 喂 `watchDogFlag`。
- [motor.c:173-177](../user/hardware/motor/motor.c#L173-L177) 的 `Motor_TIM3_interruptCallFunction()` 只做 `watchDogFlag = 1`，**没有任何状态迁移/去抖逻辑**；而且 `watchDogFlag` 置位后从不清零 → 第二次进入时立即"超时"。
- [motor.c:45-94](../user/hardware/motor/motor.c#L45-L94) 的整体流程是"启动 TIM3 → 等 RUNNING → 停 TIM3 → 等 STOPPED → 换向 → 再来一遍"。由于状态机不存在，这段代码一旦被调用，最终必然走进 `while(1){}` 卡死（[motor.c:63-67](../user/hardware/motor/motor.c#L63-L67)）。
- [motor.c:155](../user/hardware/motor/motor.c#L155) 在 `Motor_motorStatus_setter` 内部调用 `Motor_stop_method()`，而 `Motor_stop_method()` 又在等 `motor.status == STOPPED`——该赋值发生在 setter 末尾 → **潜在自锁**（不过目前 setter 无人调用，属埋雷）。

**建议**：明确"谁负责状态机"——推荐放在 **TIM3 0.5 s 中断**里（见 P1），主循环只读状态：

```c
void Motor_TIM3_interruptCallFunction(void) {
    if (!MotorStatusCheckInterruptFlag) { return; }

    switch (motor.status) {
    case MotorStatus_STOPPED:
        motor.status = MotorStatus_RUNNING_DEBOUNCE;   // 采样编码器→方向判断
        break;
    case MotorStatus_RUNNING_DEBOUNCE:
    case ...
    default: break;
    }
    watchDogFlag = 0;      // 每拍清零，由主循环侧按超时计数判断
}
```

（状态迁移应按你的实际需求设计：编码器有脉冲/无脉冲/方向判定→RUNNING / LOCKED / BRAKE / STOPPED，且**所有去抖计数都要有上限**。）

### P0-5 PWM 换向只写一个通道，另一个通道残留 → 桥臂直通/失控风险

[motor.c:121-129](../user/hardware/motor/motor.c#L121-L129)：

```c
static void Motor_pwmOutput_method(uint16_t pwm) {
    if (motor.direction == MotorDirection_Right) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm);     // CH2 没清零！
    } else {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwm);     // CH1 没清零！
    }
    motor.pwm = pwm;
}
```

换向瞬间 CH1 保留旧占空比，CH2 又写入新占空比 → 两个 IR2104 输入同时为高电平，电机可能全桥导通/失控。

**修复**：

```c
static void Motor_pwmOutput_method(uint16_t pwm) {
    if (motor.direction == MotorDirection_Right) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm);
    } else {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwm);
    }
    motor.pwm = pwm;
}
```

另外**换向时编码器反馈符号必须同步取反**（否则 PID 用反了的反馈会正反馈发散），这一步目前完全没有——建议在 `Motor_changeDirection_method()` 里同时翻转采样符号，或由采样函数读取 `motor.direction` 决定正负。

### P0-6 中断里执行阻塞/长耗时操作（三个地方）

1. **按键回调在 TIM4 中断里直接跑电机流程**：[key.c:142-157](../user/hardware/key/key.c#L142-L157) 的回调由 [key.c:130](../user/hardware/key/key.c#L130) 在 `Key_TIM4_interruptCallFunction()`（TIM4 1 ms 中断）里调用，而回调会调用 `Motor_start_method()` / `Motor_stop_method()` / `Motor_changeDirection_method()` —— 这些函数内部有 `while (motor.status != ...)` 死等。一旦状态机行为改变，**整个系统会在中断里锁死**（中断优先级 0，连 SysTick 都不再运行）。
2. **OLED 刷新在 TIM4 中断里**：[args_listener.c:49-63](../user/common/args_listener/args_listener.c#L49-L63) 每 21 ms 调一次 `OLED_showString/Num`，而软 I2C 每写一个字节都是一次完整 Start+3 字节+Stop 事务（约 30 个 GPIO 翻转），一次刷新 >200 µs，占 1 ms 中断的 20%+ → 按键/PID 时序抖动。
3. **`Motor_statusCheck_method` 在主循环里忙等**（可接受但不优雅，且没有喂狗）。

**修复原则：中断只置标志/入队，主循环处理。**

```c
/* key.c 回调 */
static void Key_pressedHandle_key0(void) {
    g_motorCmd = MOTOR_CMD_TOGGLE;      /* volatile 全局事件标志，ISR 里只写这一行 */
}

/* user/main.c 主循环 */
while (1) {
    if (g_motorCmd == MOTOR_CMD_TOGGLE) { g_motorCmd = 0; Motor_keyToggle_method(); }
    ArgListener_display_method();       /* OLED 刷新挪出 ISR，500ms 一次即可 */
}
```

### P0-7 TIM3 中断服务函数没接业务回调（0.5 s 定时器"配了但白配"）

- [stm32f1xx_it.c:205-214](../Core/Src/stm32f1xx_it.c#L205-L214) 的 `TIM3_IRQHandler` 只调了 `HAL_TIM_IRQHandler(&htim3)`，**没有调用 `Motor_TIM3_interruptCallFunction()`**（对比 TIM4 是有 `TIM4_Interrupt()` 的）。
- [interrupt_TIM.c](../user/interrupt/interrupt_TIM.c) 里也没有 TIM3 分发函数，`Motor_TIM3_interruptCallFunction` 是孤岛函数。
- 同时 [interrupt_TIM.c:6-9](../user/interrupt/interrupt_TIM.c#L6-L9) 的 `TIM4_Interrupt()` 漏了 `ArgListener_TIM4_interruptCallFunction()`——所以 args listener 的 OLED 刷新永远不会执行。

**修复**：

```c
/* interrupt_TIM.c */
void TIM3_Interrupt(void) {
    Motor_TIM3_interruptCallFunction();
}

void TIM4_Interrupt(void) {
    Key_TIM4_interruptCallFunction();
    Pid_TIM4_interruptCallFunction();
    /* ArgListener 建议改为置标志，在主循环刷新 */
}
```

```c
/* stm32f1xx_it.c  USER CODE BEGIN TIM3_IRQn 1 */
TIM3_Interrupt();
```

---

## 2. 🟠 P1 严重问题（功能错误 / 明显隐患）

### P1-1 按键数量配置错误，且注册逻辑有 bug

- [keyConfig.h:4](../user/config/keyConfig.h#L4) `KEYNUMBER 2`，但代码里注册 **3 个**按键（KEY0/1/2），`GPIO_PIN_X[]` 也是 3 项 → `keyManagerList[2]` 数组只有 2 个槽位。
- [key.c:38-42](../user/hardware/key/key.c#L38-L42) 的注册循环**没有 `break`**：

  ```c
  for (uint8_t i = 0; i < KEYNUMBER; i++)
      if (keyManagerList[i] == NULL) keyManagerList[i] = self;   // 一次 Key_init 把两个槽都填成同一个 key！
  ```
  结果：第一次 `Key_init(&key0)` 就把 `keyManagerList[0]` 和 `[1]` 都占成 key0；`key1/key2` 永远注册不进去 → **两个按键完全失灵**。

**修复**：

```c
#define KEYNUMBER 3     // keyConfig.h

void Key_init(Key* self, KeyType_enum keyType) {
    ...
    for (uint8_t i = 0; i < KEYNUMBER; i++) {
        if (keyManagerList[i] == NULL) { keyManagerList[i] = self; break; }   // 加 break
    }
}
```

### P1-2 齿轮目标转速表顺序与枚举反了

- [motor.h:14-18](../user/hardware/motor/motor.h#L14-L18)：`MotorGear_GEAR3 = 0, MotorGear_GEAR2 = 1, MotorGear_GEAR1 = 2`
- [motor.c:13](../user/hardware/motor/motor.c#L13)：`static uint16_t gearAimRpmList[3] = {150, 200, 300};`
- 即 `GEAR3 → 150 rpm`、`GEAR1 → 300 rpm`，与命名完全相反；且 [motor.c:99](../user/hardware/motor/motor.c#L99) `Motor_start_method()` 每次启动都强制 `motor.gear = MotorGear_GEAR3`，**换向/重启会把用户选的挡位重置掉**。

**修复**：`{300, 200, 150}`（与枚举顺序一致），`Motor_start_method()` 不要再覆盖 `motor.gear`（保留当前挡位），只 `Pid_r_setter`。

### P1-3 `motor.status` / 状态标志的并发与生命周期问题

- `motor` 结构体被主循环（getter/setter、按键回调）和中断（PID 调整、未来的状态机）同时访问，**未加 `volatile`，也无临界区**。虽然 Cortex-M3 上 8/16 位访问是原子的，但"读-改-写"与复合状态仍有竞态。
- `watchDogFlag` 只置不清（[motor.c:175](../user/hardware/motor/motor.c#L175)），第二次使用必定立即误判超时。
- `motorLockedFlag` 非 volatile，且只在 `Motor_motorStatus_setter()` 内使用（该 setter 无人调用）→ 整段逻辑是死代码。
- 建议：`static volatile Motor motor;` 或更规范地用 `__disable_irq()/__enable_irq()` 保护复合字段；状态机集中在一处更新。

### P1-4 TIM3 的 0.5 s 配置在运行时改，与 .ioc 不一致

- [11.5.ioc](../11.5.ioc) 里 TIM3 没有配置 `Prescaler/Period`，生成代码 [main.c:301-304](../Core/Src/main.c#L301-L304) 是 `PSC=0, ARR=65535` → 实际 **≈0.91 ms**。
- [motor.c:31-33](../user/hardware/motor/motor.c#L31-L33) 在 `Motor_harwareInit()` 里运行时改成 `PSC=7199, ARR=4999`（0.5 s）再 `HAL_TIM_Base_Init()`。功能上能跑，但：
  1. CubeMX 重新生成后容易被覆盖/阅读混乱；
  2. `HAL_TIM_Base_Init` 之后再 `HAL_TIM_Base_Start_IT` 的行为依赖 NVIC 已使能（MSP 里已开 ✓）。
- 建议直接在 CubeMX 里把 TIM3 配成 `PSC=7199 / ARR=4999`，删掉运行时修改。

### P1-5 OLED 显示：0 不显示、列号 0 越界、字符越界

1. **数字 0 显示为空**：[math_base.h:42-51](../user/common/math/math_base.h#L42-L51) 的 `MathBase_numberLength_tool(0)` 返回 **0**（已实测），而 [oled.c:165-169](../user/hardware/oled/oled.c#L165-L169) 用它决定循环次数 → `OLED_showNum_method` 显示 0 时一个字符都不画。
   修复：`if (number == 0) { length = 1; }`。
2. **列号 0 越界**：[oled.c:113](../user/hardware/oled/oled.c#L113) `(column - 1) * 8` 是 **1-based** 列号（1~16），而 [args_listener.c:27](../user/common/args_listener/args_listener.c#L27) 传的是 `column = 0` → `(0-1)*8` 在 uint8_t 运算中回绕成 248 → 写出 GDDRAM 列地址 >127，显示错位/无效。
   修复：`OLED_showString_method(i + 1, 1, ...)`（列从 1 开始），或把 API 统一改成 0-based。
3. **字模数组无边界检查**：[oled.c:116](../user/hardware/oled/oled.c#L116) `OLED_F8x16[_char - ' ']`，`_char` 若为 `\0`、中文多字节（>0x7E）会越界读 flash（95 个字模，0x20~0x7E）。建议检查 `_char >= ' ' && _char <= '~'`，否则跳过或显示空格。
4. `OLED_showNum_method` 没有列上限检查，`column + length > 16` 时 SSD1306 列地址会回卷，可能覆盖行首内容。

### P1-6 args_listener 的多处问题

- [args_listener.c:12-14](../user/common/args_listener/args_listener.c#L12-L14)：`argName_pwm` 定义未使用（`-Wall` 会警告）；`argName_direction` 内容 `{'D','I','T'}` 拼写错误（应为 DIR）。
- [args_listener.c:21](../user/common/args_listener/args_listener.c#L21)：`argName_rpm` 标签配的却是 `Motor_motorPwm_getter`（PWM 值），名不符实。
- [args_listener.c:17](../user/common/args_listener/args_listener.c#L17) 与 [args_listener.c:36-42](../user/common/args_listener/args_listener.c#L36-L42)：`cnt` 既是"注册下标"又是"ISR 分频计数"，`ArgListener_init()` 末尾把 `cnt=0`，之后任何再注册都会覆盖第 0 项；ISR 与 init 之间也无保护。
- [args_listener.c:59](../user/common/args_listener/args_listener.c#L59)：`OLED_showNum_method(i+1, ...)` 在 ISR 里调用软 I2C（见 P0-6）。
- [args_listener.h:7](../user/common/args_listener/args_listener.h#L7) 用 `uint32_t ptr` 存函数指针（已沟通过，建议改成 `getterFunction getter;` 类型化成员）。
- 分频计数 `cnt >= 20` 实际是 **21 ms** 刷新一次，对 OLED 来说太快了（建议 500 ms）。

### P1-7 软 I2C 时序与可靠性

- [i2c.c:6-7](../user/connectivity/i2c/i2c.c#L6-L7) 用 `HAL_GPIO_WritePin` 直接翻转，**没有任何延时**。72 MHz 下一次 HAL 调用约 0.1~0.3 µs，SCL 周期远快于 I2C 标准（100 kHz / 400 kHz）→ 超出 SSD1306 时序规范，属"碰运气能亮"，不同批次 OLED/线长可能失败。
  建议：每个边沿间加 1~2 µs 延时（或 `__NOP()` 若干），明确设计成 100 kHz/400 kHz。
- **从不读取 ACK**（[i2c.c:30-39](../user/connectivity/i2c/i2c.c#L30-L39) 注释也承认），OLED 未接/线松时由"无反馈"变成"静默失败"，无法报错。
- [i2c.c:41](../user/connectivity/i2c/i2c.c#L41) `I2C_writeRegister_method` 名字误导：SSD1306 没有寄存器地址，第二个字节是控制字节（0x00 命令 / 0x40 数据）；建议改名 `I2C_writeControl_method` 并在注释说明。
- GPIO 配置为开漏 + `GPIO_NOPULL`（[main.c:413-418](../Core/Src/main.c#L413-L418)），依赖 OLED 模块板载上拉；若换用无上拉的模块会完全不通信。建议开启内部上拉或确认外部 4.7 kΩ。

### P1-8 `Pid_init` 未初始化全部字段 / PID ISR 缺空指针保护

- [pid.c:47-66](../user/common/pid/pid.c#L47-L66) 只初始化了部分字段；`dState / yBefore / eBefore / uAcc / fristRun` 依赖对象的静态零初始化——目前靠 `static Motor motor;` 侥幸成立，一旦 PID 实例放到栈上就是随机值。
- [pid.c:146](../user/common/pid/pid.c#L146) / [pid.c:155](../user/common/pid/pid.c#L155) 直接调用 `pidList[i]->sampleFunc()`，注册时若传 `NULL` → 中断内 HardFault。
- [pid.c:17](../user/common/pid/pid.c#L17) 注释写"使用 TIM6"，实际用的是 TIM4（注释过期）。
- 建议：`Pid_init` 里补全字段初始化；启动时 `if (sampleFunc == NULL || pidAdjustFunc == NULL) return;` 并考虑返回错误码。

### P1-9 `Motor_statusCheck_method` 的设计问题（即使状态机补上也建议重构）

- 用**阻塞 while + 全局 flag** 实现"超时"，没有 `HAL_GetTick()` 超时兜底；`watchDogFlag` 不复位（P1-3）。
- 内部两次重复的流程（右转/左转各一段）可以抽成函数，减少复制粘贴。
- [motor.c:46](../user/hardware/motor/motor.c#L46) `__HAL_TIM_CLEAR_IT(&htim1, ...)` 应在每次启动前同时清 TIM3 的更新标志，否则可能立即进一次中断。

### P1-10 头文件规范问题

- [oled.h](../user/hardware/oled/oled.h)：用 `uint8_t/uint32_t` 但未 `#include <stdint.h>`（靠 `string.h` 间接带入，属脆弱依赖）；`String*` 与定义 `const String*` 冲突（P0-2）。
- [string.h](../user/common/string.h)：`String_init()` 在 [string.c:8](../user/common/string.c#L8) 有定义但**头文件没声明**——一旦有人调用就是隐式声明。要么补声明，要么删掉这个没人用的函数。
- [fortLib.h](../user/hardware/oled/fortLib.h)：`const uint8_t OLED_F8x16[][16]` **在头文件里定义数据**——目前只有 oled.c 包含所以没炸，一旦第二个 .c 包含就 `multiple definition`。建议移到 `fortLib.c`，头文件只留 `extern const uint8_t OLED_F8x16[][16];`。
- [interrupt_TIM.h](../user/interrupt/) **不存在**：`TIM4_Interrupt()` 靠 [stm32f1xx_it.c:49](../Core/Src/stm32f1xx_it.c#L49) 手写原型。建议建头文件放 `TIM3_Interrupt/TIM4_Interrupt/ArgListener_display` 等原型。
- [motor.h](../user/hardware/motor/motor.h)、[key.h](../user/hardware/key/key.h) 等接口头比较干净，继续保持。

### P1-11 中断优先级与实时性

- [stm32f1xx_hal_msp.c:112-127](../Core/Src/stm32f1xx_hal_msp.c#L112-L127)：TIM3/TIM4 都是 **优先级 0**，与 SysTick(15) 之间没有合理分层。PID 1 ms 中断里做软件浮点运算（Cortex-M3 无 FPU），单次 `Pid_deltaCalc_method` 估算几十 µs，建议：
  - 要么把 PID 周期降到 5~10 ms（控制环不需要 1 kHz）；
  - 要么改定点/`float` 简化；
  - 至少给 TIM4 一个明确的中断优先级并在主循环之外避免长临界区。

### P1-12 中断共享 TIM4 的启停没有单一归属

- `Key_hardwareInit`、`Pid_hardwareInit` 都各自 `HAL_TIM_Base_Start_IT(&htim4)`；[key.c:50-52](../user/hardware/key/key.c#L50-L52) `Key_hardwareFinalize` 又 `HAL_TIM_Base_Start`（无 IT，冗余）。
- 建议：TIM4 的启动/停止集中在一个 BSP/主初始化处，模块只注册自己"挂到节拍上"的处理函数（例如函数指针数组），避免谁都能启停共享外设。

---

## 3. 🟡 P2 建议（正确性隐患 / 可维护性 / 风格）

### 3.1 通用宏库边界问题（已主机实测）

- [math_base.h:14-28](../user/common/math/math_base.h#L14-L28) `MathBase_pow_tool`：`__typeof__` 对字面量取的是 `int`，实测 `MathBase_pow_tool(10, 10)` 触发 **signed integer overflow（UB）**。10 位数（如 4294967295）走 `MathBase_numberLength_tool` 时会算到 10^10 → UB。建议 result 用 `uint32_t`，或提前用除法判断（`while (num >= 10) { num /= 10; len++; }`），别用幂。
- [math_base.h:42-51](../user/common/math/math_base.h#L42-L51) `MathBase_numberLength_tool`：0 返回 0（见 P1-5），且用"减法回绕比较"实现循环判断，可读性差；建议直接循环除以 10。
- [bit.h:51-56](../user/common/bit.h#L51-L56) / [bit.h:75-79](../user/common/bit.h#L75-L79) 多位读写：`size=8` 时 `MathBase_pow_tool(2,8)=256` 存入 `uint8_t temp` → 截断为 0 → `temp-1=255` 逻辑出错；`n+size>8` 时 `temp << n` 溢出。实测 `Bit_writeMultipleBits_tool(&x,4,8,0xAB)` 结果 `0xB0`（错误）。建议改成 `mask = ((1u << size) - 1u) << n;`（`uint32_t` 中间量），并限制 `size<=8, n+size<=8`。
- [bit.h:18-41](../user/common/bit.h#L18-L41) `Bit_*` 未做 `n<8` 边界检查。
- [bit.h:64-66](../user/common/bit.h#L64-L66) `Bit_readBit_tool` 的 `((*a >> n) & 254) ^ (*a >> n)` 虽然结果正确，但过于晦涩，建议 `((*a >> n) & 1u)`。

### 3.2 魔数与配置分散

- IR2104 引脚 `GPIO_PIN_5/6` 硬编码在 [motor_ir2104.c:6](../user/hardware/motor/motor_ir2104.c#L6)；软 I2C 引脚 `GPIO_PIN_8/9` 硬编码在 [i2c.c:6-7](../user/connectivity/i2c/i2c.c#L6-L7)；LED 引脚在 [led.c:5](../user/hardware/led/led.c#L5)；按键引脚在 [key.c:26-28](../user/hardware/key/key.c#L26-L28)。而 [main.h:62-71](../Core/Inc/main.h#L62-L71) 其实已有 `KEY0_Pin/LED0_Pin` 等宏——建议全部改用 `main.h` 的宏或建一个 `board_config.h` 统一管理，CubeMX 重新生成时也不易踩。
- PWM 频率/周期 `3599`、`1500`、`3059`、`539` 等魔数散落各处；建议集中在 `motorConfig.h`（目前是空文件）。

### 3.3 死代码 / 未使用项

- [motor_encoder.c/h](../user/hardware/motor/motor_encoder.c) 为空（只有 include），要么补实现要么删除。
- [string.c:8](../user/common/string.c#L8) `String_init` 无声明且无人调用。
- [args_define.h:10-12](../user/common/args_define.h#L10-L12) `FuncStatus_enum` 无人使用。
- [math_base.h:35-40](../user/common/math/math_base.h#L35-L40) `MathBase_max_tool` 无人使用。
- `Pid` 结构体 [pid.h:23](../user/common/pid/pid.h#L23) 的 `eiMax/eiMin` 未使用（积分限幅实际只对 `uAcc` 整体限幅）。
- [motor.c:12-17](../user/hardware/motor/motor.c#L12-L17) 部分标志/变量只写不读或只读不写，建议清理。
- `MotorStatusCheckInterruptFlag` 在中段设置后立刻清 0，但 `watchDogFlag` 从不清零。

### 3.4 命名 / 拼写

- `Motor_harwareInit` → `Motor_hardwareInit`（少 t），头文件与 .c 一起改。
- `fristRun`（[pid.h:33](../user/common/pid/pid.h#L33)）→ `firstRun`。
- `agrsManagerList` → `argsManagerList`（[args_listener.c:16](../user/common/args_listener/args_listener.c#L16)）。
- `_method` 后缀用于私有实现，但 `Motor_statusCheck_method`、`Key_updateKeyStatus_method` 等是公开 API，建议公开接口去掉 `_method`（或在头文件注释区分 public/private）。
- `Pid_hardwareInitFinalize` 与 `Key_hardwareFinalize`、`Motor_hardwareFinalize` 命名不统一（`_method` / `_tool` / `_getter` 混用）。
- 建议加 `.clang-format`（LLVM 或自定义风格）统一排版（当前缩进有 tab/空格混用，如 [motor.c](../user/hardware/motor/motor.c) 的 tab 与 [args_listener.c](../user/common/args_listener/args_listener.c) 的 4 空格）。

### 3.5 工程/仓库卫生

- `git status` 显示 **62 个文件变更、61697 行增删**，其中 27 个 `Drivers/` 文件 33710 行增删是**换行符 CRLF↔LF 噪音**（在 WSL 下打开导致）。建议：
  - 添加 `.gitattributes`：`* text=auto eol=lf`（或对 `Drivers/**` 用 `-text`），然后 `git add --renormalize .`；
  - 别把 Drivers 的换行符变更混进业务提交。
- `.gitignore` 已有 `.run/ build/ .idea/` ✓；但 `build/` 目录里仍有旧 elf/hex（4.5 MB map 等）——确认没被提交即可。
- [README.md](../README.md) 只有一句指向 [document/plan.md](../document/plan.md)，而 plan.md 是 0 字节——补一份一页纸的工程说明（硬件连接、外设分配、控制流程、构建烧录命令）。
- 没有 `.clang-tidy` / `.clang-format` / `.editorconfig`；既然在用 clangd + clang-tidy，建议提交配置，免得每个人告警口径不一致。
- Debug 编译 `-Wall` 但没有 `-Wextra`（[cmake/gcc-arm-none-eabi.cmake:89](../cmake/gcc-arm-none-eabi.cmake#L89)）；建议加 `-Wextra -Wshadow -Wdouble-promotion -Wconversion`（嵌入式场景 `-Wconversion` 可能噪音多，可先只开 `-Wextra`）。

### 3.6 安全 / 健壮性（可选增强）

- 无看门狗（IWDG/WWDG），主循环卡死无法自恢复；建议加 IWDG（约 1~2 s）+ 主循环喂狗。
- 无堵转/过流保护：堵转时 PID 会把占空比推到上限；建议用编码器"指令 vs 实际转速偏差持续超限"做堵转检测并强制停机（`MotorLocker` 状态）。
- 方向切换时建议：
  1. 先 PWM 清零 → 2. 等待编码器速度 ≈ 0（超时保护）→ 3. 关闭 IR2104 SD → 4. 换向 → 5. 再使能。
  目前 `Motor_changeDirection_method()` 缺 2/3 两步。
- [motor.c:119](../user/hardware/motor/motor.c#L119) `Motor_start_method()` 每次都把挡位重置为 GEAR3（见 P1-2）。
- `Motor_start_method` 打开 IR2104 后立即 15% 占空比起步，建议从 0 斜坡爬升，避免大电流冲击。

### 3.7 主机实测发现（供参考）

| 测试 | 结果 |
|---|---|
| `MathBase_numberLength_tool(0)` | 0（应为 1）→ OLED 显示 0 为空 |
| `MathBase_pow_tool(10,10)` | UBSan 报 signed integer overflow |
| `Bit_writeMultipleBits_tool(&x, 4, 8, 0xAB)` | 输出 `0xB0`（应为 0xAB 或明确的截断错误） |
| `Bit_readBit_tool` | 结果正确（实现晦涩） |

---

## 4. 修复优先级建议（建议按此顺序）

**第 1 步：让它能编译、能跑起来（半天内）**

1. [ ] 修 `motor.c:23` 的 `...`（`sampleFunc` 先传 `NULL`，PID ISR 判空）。
2. [ ] `oled.h:6` 改成 `const String*`。
3. [ ] `CMakeLists.txt` 加入 `user/hardware/oled/oled.c`。
4. [ ] 接通入口：`Core/Src/main.c` 的 USER CODE 2 调用 `app_main()`，`user/main.c` 的 `appMain` 改名 `app_main`（void）。
5. [ ] `hardware_init` 里补齐 `Motor_init / Motor_harwareInit / Pid_hardwareInit / ArgListener_init`，并把 `Key_hardwareInit` 挪到最后。

**第 2 步：电机基本能动（1~2 天）**

6. [ ] `Motor_pwmOutput_method` 补"另一个通道清零"。
7. [ ] `KEYNUMBER=3` + `Key_init` 加 `break`。
8. [ ] `gearAimRpmList` 改 `{300,200,150}`，`Motor_start_method` 不再覆盖挡位。
9. [ ] 实现编码器启动 + `sampleFunc`（TIM2 计数差分 → rpm，含方向符号与 16 位回绕处理）。
10. [ ] TIM3 中断接上 `Motor_TIM3_interruptCallFunction()`，补全状态机与超时。
11. [ ] 按键回调改成"置事件标志 + 主循环处理"，去掉 ISR 里的阻塞调用。

**第 3 步：OLED / 参数显示（1 天）**

12. [ ] OLED 刷新移出 ISR（主循环 500 ms 一次）。
13. [ ] `OLED_showNum_method` 修 0 长度；`args_listener` 列号从 1 开始；字模越界检查。
14. [ ] args_listener 的 `cnt` 拆分（注册下标 / 刷新分频），标签修正（RPM/DIR/PWM）。
15. [ ] 软 I2C 加时序延时、读 ACK（或至少加超时/错误返回）。

**第 4 步：收尾与加固（持续）**

16. [ ] 头文件规范（stdint 自包含、fortLib.h 数据外移、String_init 声明、interrupt_TIM.h）。
17. [ ] 宏边界修复（`MathBase_pow/numberLength`、`Bit_*MultipleBits`）。
18. [ ] 命名拼写统一 + `.clang-format` + `.gitattributes` 清理换行符。
19. [ ] 看门狗 + 堵转保护 + 初始化自检（OLED/编码器/按键）。
20. [ ] 补 `document/plan.md`（硬件连线、外设分配表、控制流程、构建烧录说明）。

---

## 附录 A：当前初始化调用图

```
main()                       Core/Src/main.c
 ├─ HAL_Init / SystemClock_Config / MX_GPIO_Init
 ├─ MX_TIM1_Init (PWM 20kHz) / MX_TIM2_Init (编码器, 未 Start)
 ├─ MX_TIM4_Init (1ms 节拍, 未 Start)
 ├─ MX_TIM3_Init (0.91ms, 未 Start)
 └─ while(1) { }            ← 空循环，app_main 未调用

user/main.c appMain()        ← 从未被调用
 ├─ hardware_init()
 │   ├─ LED_on / Key_hardwareInit (启动 TIM4 中断) / Key_init ×3 / OLED_Init
 ├─ hardware_initFinalize() (仅 Key_hardwareFinalize)
 └─ initComplete()
    └─ while(1) { }         ← 全空

中断:
 TIM4_IRQHandler → HAL_TIM_IRQHandler → TIM4_Interrupt()
     ├─ Key_TIM4_interruptCallFunction → 按键去抖 → 回调 → 电机阻塞操作 ❌
     └─ Pid_TIM4_interruptCallFunction → pidList 为空(未初始化) → 空转
 TIM3_IRQHandler → HAL_TIM_IRQHandler → (无业务回调) ❌
```

## 附录 B：评审中未能验证的部分

- 本环境没有 `arm-none-eabi-gcc` 工具链，无法实际编译整个工程；编译类结论基于 C 语言规则和跨文件接口比对（`...` 语法错误、`const String*` 冲突、`oled.c` 未参与链接等均为确定性问题）。
- `build/Debug` 中的产物为 2025-11-12 12:52 生成，早于当前代码，不能代表当前可构建状态。
- 硬件时序（IR2104 死区、OLED 上拉电阻、编码器电平）需上板用示波器/逻辑分析仪确认，本报告只做代码层判断。
