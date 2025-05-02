# HTTPS.py
import os

import requests
import pandas as pd
import json
from typing import Dict, Optional
import time


class HTTPSClient:
    def __init__(self,
                 base_url: str = "https://zhouyulin.online",
                 api_key: str = "SECRET_2025zylKEY_A"):
        # 强制转换为字符串类型
        self.base_url = str(base_url)
        self.api_key = str(api_key)

        self.session = requests.Session()
        self.session.headers.update({
            "X-API-Key": self.api_key,
            "Content-Type": "application/json"
        })
        self.session.verify = True  # 启用SSL验证
        self.timeout = 10  # 请求超时时间

    def _construct_payload(
            self,
            raw_path: str,
            raw_df: Optional[pd.DataFrame] = None,
            predict_df: Optional[pd.DataFrame] = None,
            analysis_str: Optional[str] = None
    ) -> Dict:
        """构造符合服务器要求的载荷"""
        payload = {}

        # 添加路径类型检查
        if not isinstance(raw_path, str):
            raise TypeError(f"无效路径类型: {type(raw_path)}")

        # 验证文件存在性
        if not os.path.exists(raw_path):
            raise FileNotFoundError(f"文件不存在: {raw_path}")

        # 转换原始数据
        if raw_df is not None:
            # 保留最后1小时数据（3秒间隔*1200=1小时）
            payload['raw_data'] = raw_df.tail(1200).to_dict(orient='records')

        # 转换预测数据
        if predict_df is not None:
            # 转换时间列为字符串
            predict_df = predict_df.copy()
            predict_df['time'] = predict_df['time'].dt.strftime('%Y-%m-%d %H:%M:%S')
            payload['predict_data'] = predict_df.to_dict(orient='records')

        # 转换分析结果
        if analysis_str:
            try:
                payload['display_data'] = json.loads(analysis_str)
            except json.JSONDecodeError:
                payload['display_data'] = {"text": analysis_str}

        return payload

    def upload_data(
            self,
            raw_path: str = "data_raw.csv",
            predict_df: Optional[pd.DataFrame] = None,
            analysis_str: Optional[str] = None
    ) -> bool:
        """执行数据上传"""
        try:
            # 新增请求头验证
            if "X-API-Key" not in self.session.headers:
                raise ValueError("API Key未配置")
            # 读取原始数据
            raw_df = pd.read_csv(raw_path)

            # 构造载荷
            payload = self._construct_payload(
                raw_path=raw_path,  # 必须显式传递
                raw_df=raw_df,
                predict_df=predict_df,
                analysis_str=analysis_str
            )

            # 发送请求
            response = self.session.post(
                f"{self.base_url}/clientA",
                json=payload,
                timeout=self.timeout
            )

            # 处理响应
            if response.status_code == 201:
                print(f"[Upload] 数据上传成功 ID:{response.json()['id']}")
                return True
            # 新增密钥错误处理
            if response.status_code == 401:
                print("[Upload] 认证失败，请检查API Key")
                return False
            else:
                print(f"[Upload] 上传失败 Code:{response.status_code} Msg:{response.text}")
                return False

        except Exception as e:
            print(f"[Upload] 上传异常: {str(e)}")
            return False

    def retry_upload(self, max_retries=3, **kwargs):
        """带重试的上传"""
        for i in range(max_retries):
            if self.upload_data(**kwargs):
                return True
            time.sleep(2 ** i)  # 指数退避
        return False
