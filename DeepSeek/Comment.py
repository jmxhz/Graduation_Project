import pandas as pd
from openai import OpenAI
import os
import time
from typing import Optional

RESULTS_DIR = "./results"
STORAGE_FORMAT = "json"  # 可选 json/txt


def save_result(content: str, file_path: Optional[str] = None) -> str:
    """
    保存结果到本地文件
    返回实际存储路径
    """
    # 自动创建存储目录
    os.makedirs(RESULTS_DIR, exist_ok=True)

    # 生成带时间戳的文件名
    filename = file_path or f"result_{int(time.time())}.{STORAGE_FORMAT}"
    full_path = os.path.join(RESULTS_DIR, filename)

    try:
        with open(full_path, 'w', encoding='utf-8') as f:
            if STORAGE_FORMAT == "json":
                import json
                json.dump({"analysis_result": content, "timestamp": time.time()}, f)
            else:
                f.write(content)
        return full_path
    except Exception as e:
        print(f"结果存储失败: {e}")
        return ""

def analyze_settlement_risk(
        file_path: str,
        api_key: str,
        base_url: str = "https://api.deepseek.com/v1",
        model: str = "deepseek-chat",
        temperature: float = 1.0,
        max_tokens: int = 1280
) -> str:
    """
    改进版沉降风险分析函数

    新增功能：
    1. 时间列名改为time
    2. 仅读取最后120行数据
    """
    # 读取CSV并优化数据处理
    try:
        # 先读取全部数据再取最后120行
        df = pd.read_csv(file_path, usecols=lambda col: col.strip()).tail(30)
    except Exception as e:
        raise ValueError(f"文件读取失败: {e}")

    # 更新列名检查方式
    required_columns = [
        "time",
        "pitch_1 (°)", "roll_1 (°)", "range_1 (mm)",
        "pitch_2 (°)", "roll_2 (°)", "range_2 (mm)"
    ]

    # 规范化列名
    df.columns = df.columns.str.strip()

    # 转换为集合比较
    missing = set(required_columns) - set(df.columns.str.strip())
    if missing:
        raise ValueError(f"缺少必要列：{missing}")

    # 高效构建数据描述
    data_desc = "【最新测量数据】\n" + '\n'.join(
        f"📅 测量时间： {row['time']} ："  # 改用time列
        f"墙体1[倾角={row['pitch_1 (°)']}°/侧角={row['roll_1 (°)']}° 距地={row['range_1 (mm)']}mm] "
        f"墙体2[倾角={row['pitch_2 (°)']}°/侧角={row['roll_2 (°)']}° 距地={row['range_2 (mm)']}mm]"
        for _, row in df.iterrows()
    )

    # Prompt
    prompt = f"""基于给出的数据进行评价：
            {data_desc}
            
            注意：
            1. 传感器测量精度不高，会有几度或者几毫米的偏差（不要攻击这一条）
            2. 可以使用emoji
            3. 禁用markdown，合理换行！
            4. 幽默风趣的语气评价该建筑物沉降监测的数据
            5. 数据中的墙体1和2是该建筑物的对向墙，请合理分析
            """

    # API调用保持不变
    client = OpenAI(api_key=api_key, base_url=base_url)

    # 流式响应处理并保存结果
    result_content = ''.join(
        chunk.choices[0].delta.content
        for chunk in client.chat.completions.create(
            model=model,
            messages=[
                {"role": "system", "content": "毒舌建筑安全师，擅长用梗解读数据"},
                {"role": "user", "content": prompt}
            ],
            temperature=temperature,
            max_tokens=max_tokens,
            stream=True
        )
        if chunk.choices[0].delta.content
    )

    # 新增存储功能
    stored_path = save_result(result_content)
    print(f"分析结果已存储至：{stored_path}")  # 调试信息

    return result_content

if __name__ == "__main__":
    data_file = "data.csv"
    API_KEY = "sk-fae2918fe3e844bbb7cf921173d33e68"  # 示例密钥，需替换

    try:
        print(analyze_settlement_risk(data_file, API_KEY))
    except Exception as e:
        print(f"执行错误: {e}")
