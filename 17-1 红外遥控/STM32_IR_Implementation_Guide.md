# STM32F1红外遥控解码实现指南 (基于HAL库)

本文档详细介绍了如何利用 STM32F1 系列单片机的定时器输入捕获 (Input Capture) 功能，高效、精确地解码红外遥控信号（以常见的 NEC 协议为例）。

---

### 一、核心原理：输入捕获 (Input Capture)

输入捕获模式是 STM32 定时器的强大功能之一。它允许定时器在检测到指定 GPIO 引脚上的电平变化时，自动地将那一瞬间定时器的计数值“锁存”到一个专用的寄存器（`CCR` - 捕获/比较寄存器）中，并触发一次中断。

**工作流程：**
1.  **硬件自动计时**：定时器以恒定频率（如1MHz）不停计数。当红外信号的边沿到来时，硬件自动记录下当前的时间戳（计数值）。
2.  **软件处理结果**：硬件完成计时后，通过中断通知 CPU。CPU 在中断服务程序中只需读取已经被硬件记录好的时间戳，计算两次时间戳的差值，即可得到精确的脉冲宽度。

这种“硬件负责测量，软件负责决策”的模式，是 STM32 高性能的体现。

---

### 二、重要：关于 STM32F1 系列的硬件限制

与 STM32F4/L4 等后续系列不同，**STM32F1 系列的通用定时器不支持在同一个通道上直接配置“双边沿触发 (Both Edges)”**。每个通道只能单独配置为捕获上升沿或下降沿。

因此，我们需要采用特定的方法来模拟双边沿捕获。本文将提供两种成熟的解决方案。

---

### 三、硬件连接

| 红外接收模块 | STM32F103C8T6 |
| :--- | :--- |
| VCC | 5V / 3.3V |
| GND | GND |
| DATA / OUT | **PA0** (方案一) / **PA0 & PA1** (方案二) |

---

### 四、解决方案一：单通道软件切换极性 (推荐)

这是最常用、最节省硬件资源的方法。我们只用一个定时器通道，但在每次中断后，用软件代码“翻转”下一次要捕获的边沿类型。

#### 1. CubeMX 配置

*   **定时器配置 (TIM2)**:
    *   `Channel 1`: 选择 **`Input Capture direct mode`**。
    *   `Prescaler (PSC)`: 设置为 **`71`** (假设时钟为72MHz，得到1MHz计数频率)。
    *   `Counter Period (ARR)`: 设置为 **`65535`**。
*   **通道配置 (Channel 1)**:
    *   `Polarity Selection`: 设置为 **`Falling Edge`** (下降沿)，作为起始捕获边沿。
    *   `Input Filter (ICFilter)`: 设置为 **`8`**。
*   **中断配置 (NVIC)**:
    *   勾选 **`TIM2 global interrupt`**。

#### 2. 代码实现

*   **启动捕获**: 在 `main()` 函数的 `while(1)` 之前，添加：
    ```c
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    ```

*   **中断回调函数**: 在 `main.c` 中实现 `HAL_TIM_IC_CaptureCallback`：
    ```c
    /* USER CODE BEGIN 4 */
    // --- 红外解码相关的全局变量 ---
    volatile uint32_t ir_last_capture = 0;
    // ... (其他 ir_state, ir_data_byte, ir_bit_count 等变量) ...

    void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
    {
        if (htim->Instance != TIM2 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1) {
            return;
        }

        // 1. 计算脉冲宽度
        uint32_t current_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        uint32_t pulse_width;
        if (current_capture > ir_last_capture) {
            pulse_width = current_capture - ir_last_capture;
        } else { // 处理定时器溢出
            pulse_width = (65535 - ir_last_capture) + current_capture + 1;
        }
        ir_last_capture = current_capture;

        // 2. 核心：根据当前捕获的边沿类型，执行逻辑并切换到下一种边沿
        if (__HAL_TIM_GET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1) == TIM_INPUTCHANNELPOLARITY_FALLING)
        {
            // == 当前是下降沿中断，意味着一个高电平刚刚结束 ==
            // 在这里根据 pulse_width (高电平宽度) 解码
            // 例如：判断是4.5ms的引导码高电平，还是1.69ms的数据'1'，或是560us的数据'0'
            
            // 切换到上升沿捕获，准备测量下一个低电平的宽度
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
        }
        else // TIM_INPUTCHANNELPOLARITY_RISING
        {
            // == 当前是上升沿中断，意味着一个低电平刚刚结束 ==
            // 在这里根据 pulse_width (低电平宽度) 解码
            // 例如：判断是9ms的引导码低电平，还是560us的数据位低电平
            
            // 切换回下降沿捕获，准备测量下一个高电平的宽度
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
        }
        
        // 将完整的状态机解码逻辑填充到上述判断中
    }
    /* USER CODE END 4 */
    ```

---

### 五、解决方案二：双通道主从模式 (纯硬件，较复杂)

此方案利用同一定时器的两个通道，一个捕获上升沿，一个捕获下降沿，并通过主从模式实现硬件自动复位计时器，从而直接得到脉冲宽度。

#### 1. CubeMX 配置

*   **引脚连接**: **需要用杜邦线在外部将 `PA0` 和 `PA1` 短接**，并连接到红外接收头的 `DATA` 引脚。
*   **定时器配置 (TIM2)**:
    *   `Slave Mode`: 选择 **`Reset Mode`**。
    *   `Trigger Source`: 选择 **`TI1FP1`** (通道1的滤波后输入)。
    *   `Prescaler (PSC)`: `71`。
    *   `Counter Period (ARR)`: `65535`。
*   **通道配置**:
    *   `Channel 1`: `Input Capture direct mode`, 极性 **`Falling Edge`**。
    *   `Channel 2`: `Input Capture direct mode`, 极性 **`Rising Edge`**。
*   **中断配置 (NVIC)**:
    *   勾选 **`TIM2 global interrupt`**。

#### 2. 代码实现

*   **启动捕获**: 需要同时启动两个通道：
    ```c
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
    ```

*   **中断回调函数**: 在回调函数中判断是哪个通道触发了中断：
    ```c
    /* USER CODE BEGIN 4 */
    void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
    {
        if (htim->Instance == TIM2)
        {
            if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) // CH1, 下降沿触发
            {
                // 硬件自动复位了CNT，所以CCR1的值就是上一个高电平的宽度
                uint32_t high_pulse_width = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
                // 在这里根据 high_pulse_width 解码
            }
            else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) // CH2, 上升沿触发
            {
                // 此时，CCR2的值就是上一个低电平的宽度
                uint32_t low_pulse_width = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
                // 在这里根据 low_pulse_width 解码
            }
        }
    }
    /* USER CODE END 4 */
    ```

---

### 六、结论与推荐

| 对比项 | 方案一 (软件切换) | 方案二 (主从模式) |
| :--- | :--- | :--- |
| **硬件资源** | 1个定时器通道, 1个GPIO | 2个定时器通道, 2个GPIO (需短接) |
| **实现复杂度** | 较低，逻辑集中 | 较高，需理解主从模式 |
| **代码逻辑** | 中断内需计算差值，并切换极性 | 中断内直接读取净脉宽，代码更简洁 |
| **可靠性** | 非常可靠，社区标准方案 | 非常可靠，但配置略复杂 |

对于绝大多数应用场景，**方案一 (单通道软件切换极性) 是首选推荐方案**。它在功能、资源占用和代码可维护性上达到了最佳平衡，是解决 STM32F1 系列红外解码问题的标准做法。


好的，我们来深入、详细地剖析一下方案二：**利用STM32定时器的“输入捕获”功能**。

这个方案是STM32处理这类带有时序测量需求任务的“教科书”式方法。

### 一、 核心原理：为什么是“输入捕获”？

想象一下，你需要用秒表测量很多人跑100米的时间。

*   **51的方案（外部中断+手动读定时器）**：
    *   你在起点，看到运动员起跑，就低头看一下秒表，记下时间。
    *   然后你跑到终点，看到运动员冲线，再低头看一下秒表，记下时间。
    *   最后你用终点时间减去起点时间，得出用时。
    *   **问题**：你从“看到”到“低头看表”有延迟，而且在你低头看表的时候，你干不了别的事。如果运动员太多（中断太频繁），你就手忙脚乱了。

*   **STM32的方案（输入捕获）**：
    *   你在起点和终点各安装了一个红外触发器，都连接到一个中央计时系统（定时器）。
    *   运动员起跑时，触发起点设备，计时系统**自动记录**下当前时间（存入`CCR`寄存器），并“嘀”一声通知你（触发中断）。
    *   运动员冲线时，触发终点设备，计时系统**再次自动记录**下当前时间，并再“嘀”一声通知你。
    *   **优势**：你不需要低头看表了，计时是硬件自动完成的，精准无延迟。你只需要在听到“嘀”声时，去读取那个已经被硬件记录好的时间即可。在这期间，你可以做别的事情（CPU空闲）。

“输入捕获”就是这个自动化的中央计时系统。它把“测量脉冲宽度”这个任务从CPU的软件工作中剥离出来，交给了定时器硬件，极大地提升了效率和精度。

### 二、 详细实现步骤

#### 1. 硬件连接
*   将红外接收模块的 `DATA` 或 `OUT` 引脚连接到 STM32 的一个支持输入捕获的引脚上。例如，`TIM2_CH1` 的默认引脚是 `PA0`。我们就用 `PA0`。

#### 2. CubeMX 图形化配置

这是最直观的一步：

1.  **选择定时器并开启通道**
    *   在左侧 `Pinout & Configuration` 视图中，点击 `Timers` -> `TIM2`。
    *   在 `Channel 1` 的下拉菜单中，选择 `Input Capture direct mode`。
    *   你会看到右侧芯片引脚图上的 `PA0` 变成了绿色的 `TIM2_CH1`。

2.  **配置定时器核心参数 (Parameter Settings)**
    *   **`Prescaler (PSC)`**: 关键参数，用于设置定时器的计数频率。我们的目标是1MHz（即1µs计一次数）。
        *   假设你的 APB1 Timer Clock 是 72MHz（在 `Clock Configuration` 标签页可以查看和设置）。
        *   公式：`计数频率 = 定时器时钟 / (PSC + 1)`
        *   所以：`1MHz = 72MHz / (PSC + 1)` -> `PSC = 71`。
        *   **将 `Prescaler` 设置为 `71`**。
    *   **`Counter Period (ARR)`**: 自动重装载值。因为我们是用来计时的，希望它能计得越久越好，防止中途溢出。所以设置为最大值。
        *   **将 `Counter Period` 设置为 `65535`** (这是一个16位定时器的最大值)。

3.  **配置输入捕获通道参数**
    *   **`Polarity Selection`**: **捕获边沿选择，这是最重要的设置**。
        *   NEC协议的逻辑“0”和“1”是通过测量两个**下降沿**之间的总时间来区分的。但为了解码引导码（一个长低电平+一个长高电平），我们需要知道每个电平的宽度。
        *   **设置为 `Both Edges` (上升沿和下降沿都捕获)**。这样，信号的每一次跳变都会触发一次捕获和中断，我们可以精确测量出每一个高电平和低电平的持续时间。
    *   **`Input Filter (ICFilter)`**: 输入滤波器。红外信号可能带有毛刺。设置一个小的滤波值可以增加稳定性。
        *   **可以设置为 `4` 或 `8`**。这意味着连续采样4或8个时钟周期，电平都稳定，才认为是一次有效的边沿。

4.  **开启中断 (NVIC Settings)**
    *   点击 `NVIC` 标签页。
    *   **勾选 `TIM2 global interrupt`**。

5.  **生成代码**
    *   点击右上角的 `GENERATE CODE`。

#### 3. 代码编写

CubeMX 已经帮你完成了所有底层的初始化配置。你只需要在 main.c (或新建一个 ir.c 文件) 中编写应用逻辑。

1.  **在 `main()` 函数中启动输入捕获**
    *   在 `while(1)` 循环之前，找到 `MX_TIM2_Init();` 这行代码。
    *   在它下面，添加一行代码来启动定时器和输入捕获中断：

    ```c
    /* USER CODE BEGIN 2 */
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1); // 启动TIM2的通道1，并使能输入捕获中断
    /* USER CODE END 2 */
    ```

2.  **编写中断回调函数**
    *   当捕获事件发生时，HAL库会调用一个弱函数 `HAL_TIM_IC_CaptureCallback()`。我们需要在 main.c 的 `USER CODE` 区域重新定义这个函数，写入我们的解码逻辑。

    ```c
    /* USER CODE BEGIN 4 */

    // --- 红外解码相关的全局变量 ---
    #define IR_STATE_IDLE       0  // 空闲，等待引导码的下降沿
    #define IR_STATE_LEADER     1  // 已收到引导码下降沿，等待引导码高电平结束
    #define IR_STATE_DATA       2  // 正在接收数据
    #define IR_STATE_STOP       3  // 接收完成

    volatile uint8_t  ir_state = IR_STATE_IDLE;
    volatile uint32_t ir_capture_value = 0;
    volatile uint32_t ir_pulse_width = 0;
    
    // ... 其他如 ir_data, ir_pdata 等变量 ...

    void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
    {
        if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
            // 1. 计算脉冲宽度
            uint32_t current_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            if (current_capture > ir_capture_value) {
                ir_pulse_width = current_capture - ir_capture_value;
            } else { // 处理定时器溢出
                ir_pulse_width = (65535 - ir_capture_value) + current_capture;
            }
            ir_capture_value = current_capture;

            // 2. 根据当前引脚电平 和 脉冲宽度 进行状态机解码
            GPIO_PinState pin_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

            switch (ir_state)
            {
                case IR_STATE_IDLE:
                    // 空闲状态只关心下降沿，它标志着引导码的开始
                    if (pin_state == GPIO_PIN_RESET) {
                        ir_state = IR_STATE_LEADER; // 进入下一个状态
                    }
                    break;

                case IR_STATE_LEADER:
                    // 引导码是一个约9ms的低电平和一个约4.5ms的高电平
                    if (pin_state == GPIO_PIN_SET) { // 刚结束一个低电平
                        if (ir_pulse_width > 8500 && ir_pulse_width < 9500) {
                            // 9ms低电平正确，状态不变，继续等待4.5ms高电平
                        } else {
                            ir_state = IR_STATE_IDLE; // 脉宽错误，返回空闲
                        }
                    } else { // 刚结束一个高电平
                        if (ir_pulse_width > 4000 && ir_pulse_width < 5000) {
                            // 4.5ms高电平正确，开始接收数据
                            ir_state = IR_STATE_DATA;
                            // ir_pdata = 0; // 复位数据位计数器
                        } else if (ir_pulse_width > 2000 && ir_pulse_width < 2500) {
                            // 这是重复码的2.25ms高电平
                            // TriggerRepeatCallback();
                            ir_state = IR_STATE_IDLE;
                        } else {
                            ir_state = IR_STATE_IDLE; // 脉宽错误，返回空闲
                        }
                    }
                    break;

                case IR_STATE_DATA:
                    // 数据位都是一个560us的低电平，然后跟一个高电平
                    if (pin_state == GPIO_PIN_SET) { // 刚结束一个低电平
                        if (ir_pulse_width < 500 || ir_pulse_width > 700) {
                            // 560us低电平脉宽错误
                            ir_state = IR_STATE_IDLE;
                        }
                    } else { // 刚结束一个高电平，高电平的宽度决定了是0还是1
                        if (ir_pulse_width > 500 && ir_pulse_width < 700) {
                            // 记录一个 '0'
                            // IR_Data[ir_pdata/8] &= ~(1 << (ir_pdata%8));
                            // ir_pdata++;
                        } else if (ir_pulse_width > 1600 && ir_pulse_width < 1800) {
                            // 记录一个 '1'
                            // IR_Data[ir_pdata/8] |= (1 << (ir_pdata%8));
                            // ir_pdata++;
                        } else {
                            ir_state = IR_STATE_IDLE; // 脉宽错误
                        }

                        if (/* ir_pdata >= 32 */) {
                            // 32位接收完毕，进行校验
                            // if (校验成功) { TriggerDataCallback(); }
                            ir_state = IR_STATE_IDLE;
                        }
                    }
                    break;
            }
        }
    }

    /* USER CODE END 4 */
    ```

### 总结对比

| 特性 | 51方案 (EXTI + 手动计时) | STM32方案 (输入捕获) |
| :--- | :--- | :--- |
| **核心** | 软件计时 | **硬件计时** |
| **中断源** | 外部中断 (EXTI) | **定时器捕获中断 (TIM_IT_CC)** |
| **中断频率** | 每个下降沿一次 | 每个**边沿**（上升/下降）一次 |
| **CPU负载** | **高** (中断内要读定时器、计算、判断) | **极低** (中断内只需读已算好的值) |
| **精度** | 一般 (软件延迟) | **高** (硬件捕获无延迟) |
| **代码复杂度** | 逻辑集中在EXTI回调中 | 逻辑集中在TIM回调中，但更清晰 |
| **资源利用** | 未充分利用STM32特性 | **充分利用了强大的定时器外设** |

通过这种方式，你不仅能成功移植功能，还能写出性能更优、更符合STM32架构思想的代码。

---

### 附录：如何确定定时器预分频器(PSC)的值

定时器预分频器（Prescaler, PSC）的作用是对输入的时钟源进行分频，以得到我们想要的计数频率。配置它的核心目标是：**让定时器的计数单位尽可能方便我们进行解码**。

对于红外解码，最理想的计数单位就是 **1 微秒 (µs)**，因为 NEC 协议的时序都是基于微秒的（如 560µs, 9000µs 等）。这样，定时器计数值就直接等于时间（单位 µs），无需额外换算。

#### 计算步骤

1.  **确定定时器的时钟源频率 (`TIMx_CLK`)**
    *   首先，你需要在 CubeMX 的 `Clock Configuration` 标签页中找到你要使用的定时器（例如 `TIM2`）的时钟源频率。
    *   对于 STM32F103 系列，通用定时器（TIM2, 3, 4, 5）通常挂载在 **APB1 总线**上。
    *   你需要查看 `APB1 timer clocks` 的值。**注意一个关键规则**：
        *   如果 APB1 的预分频器（`APB1 Prescaler`）设置为 `/1`，那么供给定时器的时钟频率 **等于** APB1 总线频率（PCLK1）。
        *   如果 APB1 的预分频器设置为 `/2` 或更高，那么供给定时器的时钟频率会自动**倍频**，变为 APB1 总线频率的 **2倍**。

2.  **设定目标计数频率 (`Target_Freq`)**
    *   我们的目标是 1MHz，即每秒计数 1,000,000 次。
    *   `Target_Freq = 1,000,000` Hz。

3.  **计算 PSC 的值（以 PCLK1 = 8MHz 为例）**
    *   公式为： `PSC = (TIMx_CLK / Target_Freq) - 1`

    *   **情况 A：APB1 Prescaler 为 `/1`**
        *   `TIMx_CLK` = `PCLK1` = 8,000,000 Hz
        *   `PSC = (8,000,000 / 1,000,000) - 1 = 7`
        *   **此时应将 PSC 设置为 7。**

    *   **情况 B：APB1 Prescaler 为 `/2` 或更高**
        *   `TIMx_CLK` = `PCLK1 * 2` = 16,000,000 Hz
        *   `PSC = (16,000,000 / 1,000,000) - 1 = 15`
        *   **此时应将 PSC 设置为 15。**

#### 总结

*   **第一步**：查阅 CubeMX 的时钟树，确定 `PCLK1` 频率以及 `APB1 Prescaler` 的值。
*   **第二步**：根据 `APB1 Prescaler` 是否为 `/1` 来确定 `TIMx_CLK` 是否需要倍频。
*   **第三步**：套用公式 `PSC = (TIMx_CLK / 1,000,000) - 1` 计算出 PSC 的值。

通过这个方法，你就可以为任何系统时钟配置计算出正确的 PSC 值，从而得到一个便于计算的、以微秒为单位的计时器。