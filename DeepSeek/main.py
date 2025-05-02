import json
import os
import time
import threading
import pandas as pd
from LSTM import LSTMPredictor
import Comment
from uart import SerialDataLogger
from HTTPS import HTTPSClient

class _SafeFileAccess:
    """内部类：线程安全的文件访问上下文管理器"""

    def __init__(self, lock, filepath):
        self.lock = lock
        self.filepath = filepath

    def __enter__(self):
        self.lock.acquire()
        try:
            if not os.path.exists(self.filepath):
                raise FileNotFoundError(f"文件 {self.filepath} 不存在")

            # 读取CSV并确保包含所需列
            required_cols = [
                'time',
                'pitch_1 (°)', 'roll_1 (°)', 'range_1 (mm)',
                'pitch_2 (°)', 'roll_2 (°)', 'range_2 (mm)'
            ]

            df = pd.read_csv(
                self.filepath,
                usecols=required_cols,
                dtype={'time': 'object'}  # 先作为字符串读取
            )

            # 验证列是否存在
            missing_cols = [col for col in required_cols if col not in df.columns]
            if missing_cols:
                raise ValueError(f"缺少必要列: {missing_cols}")

            return df

        except Exception as e:
            print(f"[文件访问] 错误: {str(e)}")
            self.lock.release()
            raise

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.lock.release()

class PredictionSystem:
    def __init__(self):
        # 初始化数据采集系统
        self.logger = SerialDataLogger(
            filename='data_raw.csv',
            interval=3,
            baudrate=115200
        )

        # 模型相关配置
        self.model_lock = threading.Lock()
        self.last_operation_time = 0
        self.interval = 180  # 3分钟检查间隔
        self.train_interval = 3600 * 6  # 6小时训练间隔
        self.model_path = "best_model.h5"
        self.min_train_samples = 1000  # 最小训练样本数

        # 文件访问锁
        self.file_lock = threading.Lock()

        self.uploader = HTTPSClient()
        self.last_analysis_result = None
        self.last_prediction = None

        # 初始化预测器
        self.predictor = self._init_predictor()

    def _init_predictor(self):
        """改进的初始化方法"""
        max_retries = 3
        for attempt in range(max_retries):
            try:
                predictor = LSTMPredictor(
                    data_path=self.logger.filename,
                    model_path=self.model_path
                )

                # 验证时间戳有效性
                if predictor.last_train_time <= 0:
                    print(f"[Model] 第{attempt + 1}次尝试: 检测到无效训练时间")
                    predictor.last_train_time = time.time()
                    predictor._save_model_metadata()
                    continue

                return predictor
            except Exception as e:
                print(f"[Model] 初始化失败(尝试{attempt + 1}): {str(e)}")
                if attempt == max_retries - 1:
                    raise
                time.sleep(2)

    def _train_model(self, predictor, force=False):
        """改进的训练逻辑"""
        with self.model_lock:
            try:
                current_time = time.time()

                # 检查训练必要性
                needs_train = (
                        force or
                        predictor.model is None or
                        (current_time - predictor.last_train_time >= self.train_interval)
                )

                if needs_train:
                    print("[Model] 准备训练数据...")
                    predictor.prepare_data(retrain=True)

                    # 检查数据量是否足够
                    if not hasattr(predictor, 'X_train') or len(predictor.X_train) < self.min_train_samples:
                        print(
                            f"[Model] 数据不足 ({len(predictor.X_train) if hasattr(predictor, 'X_train') else 0}样本)，等待更多数据")
                        return False

                    if predictor.model is None:
                        predictor.build_model()

                    print(f"[Model] 开始训练，数据量: {len(predictor.X_train)}")
                    predictor.train(force_retrain=True)
                    predictor.last_train_time = time.time()  # 更新训练时间
                    print("[Model] 训练完成")
                    return True
                else:
                    print("[Model] 跳过训练，使用现有模型")
                    return False

            except Exception as e:
                print(f"[Model] 训练异常: {str(e)}")
                return False

    def _generate_prediction(self):
        """生成预测数据（修复时间列问题）"""
        with self.model_lock:
            try:
                print("\n[Prediction] 开始生成预测...")

                # 确保数据准备就绪
                if not hasattr(self.predictor, 'X_test'):
                    self.predictor.prepare_data()

                # 生成预测
                self.predictor.predict_future(steps=1200)

                pred_path = f'predictions/latest_prediction.csv'

                return pred_path

            except Exception as e:
                print(f"[Prediction] 预测异常: {str(e)}")
                return None

    def _perform_analysis(self):
        """执行大模型分析"""
        try:
            print("\n[Analysis] 启动大模型分析...")

            # 使用最新数据文件
            API_KEY = "sk-fae2918fe3e844bbb7cf921173d33e68"
            result = Comment.analyze_settlement_risk(
                "data_raw.csv",
                API_KEY
            )
            self.last_analysis_result = result
            print(f"[Analysis] 分析完成")

            return result

        except Exception as e:
            print(f"[Analysis] 分析异常: {str(e)}")
            return None

    def _upload_results(self, analysis_result, prediction_path):
        """最终兼容版上传方法"""
        try:
            print("\n[Upload] 准备上传数据...")

            # 1. 准备预测数据
            predict_df = None
            if prediction_path and os.path.exists(prediction_path):
                try:
                    predict_df = pd.read_csv(prediction_path)
                    if 'time' in predict_df.columns:
                        predict_df['time'] = pd.to_datetime(predict_df['time'], errors='coerce')
                        predict_df = predict_df.dropna(subset=['time'])
                except Exception as e:
                    print(f"[Upload] 预测数据处理失败: {str(e)}")

            # 2. 准备原始数据
            raw_df = None
            try:
                with self._safe_file_access("data_raw.csv") as df:
                    if 'time' in df.columns:
                        raw_df = df.copy()
                        raw_df['time'] = pd.to_datetime(raw_df['time'], errors='coerce')
                        raw_df = raw_df.dropna(subset=['time'])
            except Exception as e:
                print(f"[Upload] 原始数据处理失败: {str(e)}")

            # 3. 执行上传
            if raw_df is not None or predict_df is not None or analysis_result:
                success = self.uploader.retry_upload(
                    raw_path="data_raw.csv",  # 明确传递参数
                    predict_df=predict_df,
                    analysis_str=json.dumps(analysis_result) if analysis_result else None
                )
                if success:
                    print("[Upload] 上传成功")
                else:
                    print("[Upload] 上传失败")
            else:
                print("[Upload] 无有效数据可上传")

        except Exception as e:
            print(f"[Upload] 上传流程异常: {str(e)}")
            import traceback
            traceback.print_exc()

    def _execute_workflow(self):
        """改进的工作流程"""
        # 1. 执行分析
        analysis_result = self._perform_analysis()

        # 2. 检查模型状态
        if self.predictor is None:
            print("[System] 预警: 预测器未初始化!")
            self.predictor = self._init_predictor()
            if self.predictor is None:
                return

        # 3. 检查是否需要训练
        current_time = time.time()
        needs_retrain = (
                self.predictor.model is None or
                current_time - self.predictor.last_train_time >= self.train_interval
        )

        if needs_retrain:
            trained = self._train_model(self.predictor, force=True)
            if not trained:
                print("[Model] 训练未完成，跳过本次预测")
                return

        # 4. 生成预测
        prediction_path = self._generate_prediction()

        # 5. 统一上传结果
        if analysis_result or prediction_path:
            self._upload_results(analysis_result, prediction_path)

    def _schedule_tasks(self):
        """任务调度"""
        current_time = time.time()
        if current_time - self.last_operation_time >= self.interval:
            # 启动独立线程执行完整流程
            threading.Thread(
                target=self._execute_workflow,
                daemon=True
            ).start()
            self.last_operation_time = current_time

    def _safe_file_access(self, filepath):
        """线程安全的文件访问入口方法"""
        return _SafeFileAccess(self.file_lock, filepath)

    def run(self):
        """主运行循环"""
        self.logger.start()
        print("数据采集系统已启动...")

        try:
            while True:
                self._schedule_tasks()
                time.sleep(30)  # 降低CPU占用
                print(f"[心跳检测] 系统运行正常: {time.ctime()}")

        except KeyboardInterrupt:
            self.logger.stop()
            print("\n系统安全关闭")


if __name__ == "__main__":
    system = PredictionSystem()
    system.run()
