# 毕业设计项目：建筑沉降监测与可视化预警系统

本项目是一个“嵌入式采集 + 上位机智能分析 + Unity 可视化展示”的完整闭环系统，面向建筑沉降监测场景。

系统核心目标：
- 实时采集两面墙体的姿态与位移数据；
- 基于时序模型进行趋势预测；
- 结合大模型输出风险解读文本；
- 在移动端/客户端中完成图表与三维模型联动展示。

## 1. 系统架构

系统由 4 个子模块组成：
- 采集子节点（STM32）：采集 MPU6050 与 VL6180 数据并发送；
- 主节点（STM32 + LCD）：接收并本地显示双墙体状态；
- AI 分析端（Python）：串口落盘、LSTM 预测、文本分析、HTTP 上传；
- Unity 客户端：登录、拉取数据、曲线展示、三维建筑模型联动。

## 2. 项目目录

```text
Graduation_Project/
├─ Demo/                  # STM32 子节点工程（采集端）
├─ Main_Node_LCD/         # STM32 主节点工程（LCD 显示端）
├─ DeepSeek/              # Python 上位机与 AI 处理
│  ├─ main.py             # 总调度入口
│  ├─ uart.py             # 串口采集与 CSV 写入
│  ├─ LSTM.py             # 预测模型
│  ├─ Comment.py          # 文本风险分析（大模型）
│  └─ HTTPS.py            # 数据上传
├─ Scripts/               # Unity C# 核心脚本
└─ README.md
```

## 3. 环境要求

### 3.1 硬件端
- MCU：STM32F103 系列（工程中包含 F103C8/F103RC 相关配置）；
- 传感器：MPU6050、VL6180；
- 通信：DL_LN3X 模块（子节点地址示例：`0x2025` / `0x2026`，主节点地址：`0x2003`）；
- 串口桥：CH340（Python 端会自动查找 CH340 端口）；
- 显示：ILI9341 LCD（主节点工程）。

### 3.2 软件端
- Python：建议 3.10+；
- Unity：建议 2021+（需 TMP、Newtonsoft.Json、XCharts）；
- ARM 工具链：Keil MDK-ARM（工程已包含 `MDK-ARM`），并附 IAR 工程文件。

### 3.3 Python 依赖

建议在 `DeepSeek/` 下安装：

```bash
pip install pandas numpy matplotlib scikit-learn tensorflow keras pyserial requests openai
```

## 4. 快速开始

### 4.1 拉取项目

```bash
git clone https://github.com/jmxhz/Graduation_Project.git
cd Graduation_Project
```

### 4.2 编译并下载嵌入式程序

1. 使用 Keil 打开 `Demo/MDK-ARM/Demo.uvprojx`，烧录到采集子节点；
2. 使用 Keil 打开 `Main_Node_LCD/MDK-ARM/Main_Node_LCD.uvprojx`，烧录到主节点；
3. 按实际硬件连接传感器与通信模块，确认主从地址配置一致。

### 4.3 启动 Python 上位机与 AI 工作流

```bash
cd DeepSeek
python main.py
```

运行后将自动执行：
- 串口采集并写入 `data_raw.csv`；
- 定时触发分析任务；
- 进行预测并输出 `predictions/latest_prediction.csv`；
- 上传到服务端接口（`/clientA`）。

### 4.4 Unity 客户端展示

1. 在 Unity 项目中挂载 `Scripts/` 下脚本；
2. 确认场景中已配置：
- 登录页面（`LoginController`）；
- 数据管理器（`DataManager`）；
- 曲线页面（`DataVisualization`）；
- 三维模型页面（`BuildingWallVisualizer`）。
3. 运行后客户端会从 `/clientB` 拉取 `raw/predict/display` 三类数据。

## 5. 关键实现说明

### 5.1 采集与通信
- 子节点周期采集 `pitch/roll/range`；
- 数据打包后通过 DL 模块发送至主节点；
- 主节点在 LCD 上实时显示双节点数据及时间信息。

### 5.2 数据与预测
- `uart.py` 记录 CSV 列：
  - `time`
  - `pitch_1 (°), roll_1 (°), range_1 (mm)`
  - `pitch_2 (°), roll_2 (°), range_2 (mm)`
- `LSTM.py` 使用多特征时序建模，支持模型持久化与自动重训；
- `main.py` 中默认每 3 分钟调度一次任务，训练间隔约 6 小时（代码参数可改）。

### 5.3 风险分析文本
- `Comment.py` 基于最新数据构建提示词，调用大模型返回风险描述；
- 分析结果保存到 `DeepSeek/results/` 目录。

## 6. 服务接口约定（按当前代码）

- Python 上传：`POST https://zhouyulin.online/clientA`
  - Header：`X-API-Key`
  - Body：`raw_data` / `predict_data` / `display_data`
- Unity 拉取：`GET https://zhouyulin.online/clientB?data_type=raw|predict|display`
  - Header：`Authorization: Bearer ...`
- Unity 登录：`POST https://zhouyulin.online/login`

## 7. 安全与交付建议

当前仓库中存在硬编码密钥与令牌（如 API Key / Bearer Token），建议在答辩前完成以下处理：
- 改为环境变量或本地配置文件读取；
- 仓库中移除真实密钥并更换为占位值；
- 为接口增加密钥轮换与最小权限控制。

## 8. 可作为论文/答辩的亮点

- 多端协同：嵌入式 + AI + 可视化；
- 实时采集与预测结合；
- 原始数据、预测结果、文本诊断三通道统一展示；
- 可扩展到更多监测点与更多传感器类型。

## 9. 作者信息

- 项目作者：Julian Zhou（代码注释内署名）
- 用途：毕业设计项目
