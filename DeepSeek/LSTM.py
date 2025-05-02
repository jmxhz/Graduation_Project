import shutil
import time
import numpy as np
import pandas as pd
import os
import json
from keras import Input
from matplotlib import pyplot as plt
from sklearn.preprocessing import MinMaxScaler
from tensorflow.keras.models import load_model, Sequential, save_model
from tensorflow.keras.layers import LSTM, Dense
import tensorflow as tf
from sklearn.metrics import mean_absolute_error, mean_squared_error


class LSTMPredictor:
    def __init__(self, data_path, look_back=16, model_path='best_model.h5'):
        # TensorFlow线程配置
        self.y_train = None
        self.X_train = None
        tf.config.threading.set_intra_op_parallelism_threads(12)
        tf.config.threading.set_inter_op_parallelism_threads(4)
        tf.config.set_soft_device_placement(True)

        # 添加初始化状态追踪
        self._is_initialized = False

        # 强制路径类型转换
        self.data_path = str(data_path)
        self.model_path = str(model_path)

        self.look_back = int(look_back)
        self.scaler = MinMaxScaler(feature_range=(0, 1))
        self.model = None
        self.history = None
        self.X_test = None
        self.y_test = None
        self.last_prediction = None
        self.timestamps = None

        # 修改last_train_time初始化
        self.last_train_time = time.time()  # 改为当前时间而非0
        if not self._try_load_model():
            self.build_model()
            self._is_initialized = True
            print("[Model] 新建模型初始化完成")
            # 新模型立即保存
            self._save_model_metadata()  # 新增方法

    def _save_model_metadata(self):
        """专门处理元数据保存"""
        metadata = {
            'look_back': self.look_back,
            'last_train_time': self.last_train_time,
            'data_path': self.data_path,
            'saved_at': time.time()  # 保存时间戳
        }
        metadata_path = f"{self.model_path}.metadata"

        try:
            with open(metadata_path, 'w') as f:
                json.dump(metadata, f, indent=2)
            print(f"[Model] 元数据已保存: {metadata_path}")
        except Exception as e:
            print(f"[Model] 元数据保存失败: {str(e)}")

    def _try_load_model(self):
        """
        安全加载模型及其元数据，包含完整的错误处理和验证逻辑

        返回:
            bool: 是否成功加载有效模型
        """
        if not os.path.exists(self.model_path):
            print("[Model] 模型文件不存在")
            return False

        metadata_path = f"{self.model_path}.metadata"
        loaded_metadata = {}

        try:
            # 1. 首先加载并验证元数据
            if os.path.exists(metadata_path):
                with open(metadata_path, 'r') as f:
                    loaded_metadata = json.load(f)

                # 验证元数据关键字段
                required_keys = ['look_back', 'last_train_time', 'data_path']
                if not all(k in loaded_metadata for k in required_keys):
                    raise ValueError("元数据缺少必要字段")

                # 验证时间戳有效性 (不能是未来时间或早于2020年)
                current_time = time.time()
                last_train_time = float(loaded_metadata['last_train_time'])
                if last_train_time > current_time or last_train_time < 1577836800:  # 2020-01-01
                    raise ValueError(f"无效训练时间: {last_train_time}")

                self.look_back = int(loaded_metadata['look_back'])
                self.last_train_time = last_train_time

            # 2. 加载模型文件
            self.model = tf.keras.models.load_model(
                self.model_path,
                custom_objects={
                    'MeanSquaredError': tf.keras.losses.MeanSquaredError(),
                    'mae': tf.keras.metrics.MeanAbsoluteError()
                },
                compile=False
            )

            # 3. 验证模型结构是否匹配元数据
            input_shape = self.model.input_shape
            if input_shape[1] != self.look_back:  # 检查look_back参数
                raise ValueError(
                    f"模型结构不匹配: 元数据look_back={self.look_back} "
                    f"但模型需要{input_shape[1]}"
                )

            # 4. 编译模型
            self.model.compile(
                optimizer='adam',
                loss='mse',
                metrics=['mae']
            )

            # 5. 验证数据路径是否匹配
            if 'data_path' in loaded_metadata:
                if loaded_metadata['data_path'] != self.data_path:
                    print(f"[Model] 警告: 数据路径变更 {loaded_metadata['data_path']} -> {self.data_path}")

            print(
                f"[Model] 成功加载模型 (最后训练: {time.strftime('%Y-%m-%d %H:%M', time.localtime(self.last_train_time))})")
            return True

        except Exception as e:
            print(f"[Model] 加载失败: {str(e)}")

            # 清理可能损坏的文件
            for filepath in [self.model_path, metadata_path]:
                try:
                    if os.path.exists(filepath):
                        os.remove(filepath)
                        print(f"[Model] 已删除损坏文件: {filepath}")
                except Exception as cleanup_error:
                    print(f"[Model] 清理文件失败: {cleanup_error}")

            # 重置状态
            self.model = None
            self.last_train_time = time.time()
            return False

    def _enhance_features(self, data):
        diff = np.diff(data, axis=0)
        diff = np.concatenate([np.zeros((1, data.shape[1])), diff], axis=0)
        return np.concatenate([data, diff], axis=1)

    def _create_sequences(self, data):
        X, y = [], []
        for i in range(len(data) - self.look_back):
            X.append(data[i:i + self.look_back, :])
            y.append(data[i + self.look_back, :data.shape[1] // 2])
        return np.array(X), np.array(y)

    def prepare_data(self, retrain=False):
        try:
            # 强制数据文件存在性验证
            if not os.path.exists(self.data_path):
                raise FileNotFoundError(f"数据文件不存在: {self.data_path}")

            # 数据量最低要求
            if os.path.getsize(self.data_path) < 1024:
                raise ValueError("数据量不足（至少需要1KB原始数据）")

            df = pd.read_csv(self.data_path, parse_dates=['time'])
            # 分离时间戳和数值数据
            self.timestamps = df['time'].values  # 保存时间戳供后续使用
            numeric_data = df.drop(columns=['time'])  # ✅ 移除时间列

            # 数据标准化（仅处理数值列）
            scaled_data = self.scaler.fit_transform(numeric_data.values)  # ✅ 仅数值列

            # 特征增强（保持原有逻辑）
            enhanced_data = self._enhance_features(scaled_data)

            # 序列创建验证
            X, y = self._create_sequences(enhanced_data)
            if len(X) == 0:
                raise ValueError("无法创建训练序列：数据量不足或look_back参数过大")

            # 数据分割
            split_index = int(len(X) * 0.8)
            self.X_train, self.X_test = X[:split_index], X[split_index:]
            self.y_train, self.y_test = y[:split_index], y[split_index:]

            # 必须重建模型的场景
            if self.model is None or retrain:
                self.build_model()

            return True  # 明确返回成功状态

        except Exception as e:
            print(f"[Model] 数据准备失败: {str(e)}")
            # 清除无效数据
            self.X_train = self.y_train = None
            raise

    def build_model(self):
        self.model = Sequential()
        self.model.add(Input(shape=(self.look_back, 12)))
        self.model.add(LSTM(128, return_sequences=True))
        self.model.add(LSTM(64))
        self.model.add(Dense(32, activation='relu'))
        self.model.add(Dense(6))
        self.model.compile(loss='mse', optimizer='adam', metrics=['mae'])
        return self.model

    def train(self, epochs=500, batch_size=128, force_retrain=False):
        if not force_retrain and self.model and time.time() - self.last_train_time < 86400:
            print("[Model] 跳过训练，使用现有模型")
            return None

        # 确保数据已准备
        if self.X_train is None:
            self.prepare_data()

        callbacks = [
            tf.keras.callbacks.EarlyStopping(patience=80, restore_best_weights=True),
            tf.keras.callbacks.ModelCheckpoint(
                self.model_path,
                save_best_only=True,
                monitor='val_loss',
                mode='min'
            )
        ]

        self.history = self.model.fit(
            self.X_train, self.y_train,
            epochs=epochs,
            batch_size=batch_size,
            validation_split=0.2,
            callbacks=callbacks,
            verbose=1,
            shuffle=False
        )

        # 确保模型保存
        save_model(self.model, self.model_path, overwrite=True)
        self.last_train_time = time.time()

        # 保存元数据
        metadata = {
            'look_back': self.look_back,
            'last_train_time': self.last_train_time,
            'data_path': self.data_path
        }
        metadata_path = f"{self.model_path}.metadata"
        with open(metadata_path, 'w') as f:
            json.dump(metadata, f)
        print(f"[Model] 模型和元数据已保存")
        return self.history

    def evaluate(self):
        if self.model is None:
            raise ValueError("模型未初始化，请先训练或加载模型")
        if self.X_test is None:
            raise ValueError("测试数据未准备，请先调用prepare_data()")

        test_predict = self.model.predict(self.X_test)
        test_predict_actual = self.scaler.inverse_transform(np.concatenate([
            test_predict,
            np.zeros((test_predict.shape[0], 6))  # 补全特征数
        ], axis=1))[:, :6]  # 只取前6个特征

        y_test_actual = self.scaler.inverse_transform(np.concatenate([
            self.y_test,
            np.zeros((self.y_test.shape[0], 6))
        ], axis=1))[:, :6]

        print("整体指标：")
        self._print_metrics(y_test_actual, test_predict_actual)
        return test_predict_actual, y_test_actual

    def _print_metrics(self, actual, predicted):
        print("MAE:", mean_absolute_error(actual, predicted))
        print("RMSE:", np.sqrt(mean_squared_error(actual, predicted)))

    def plot_results(self, actual, predicted):
        plt.figure(figsize=(18, 12))
        for i in range(6):
            plt.subplot(3, 2, i + 1)
            plt.plot(actual[:, i], label='True')
            plt.plot(predicted[:, i], label='Predicted')
            plt.title(f"Feature {i + 1} Comparison")
            plt.xlabel("Time Step")
            plt.ylabel("Value")
            plt.grid(alpha=0.3)
            plt.legend()
        plt.tight_layout()
        plt.savefig('predictions.png')
        plt.close()

    def predict_future(self, steps=100):
        """改进的预测方法，确保时间序列连续性"""
        # 检查前置条件
        if self.model is None:
            raise ValueError("模型未初始化，请先训练或加载模型")
        if not os.path.exists(self.data_path):
            raise FileNotFoundError(f"数据文件不存在: {self.data_path}")

        try:
            # 读取数据并处理时间列
            df = pd.read_csv(self.data_path, parse_dates=['time'])
            if df['time'].isnull().any():
                raise ValueError("时间列包含无效值，请检查数据文件")

            # 获取原始特征列名（排除时间列）
            feature_columns = [col for col in df.columns if col != 'time']
            num_features = len(feature_columns)

            # 确保有足够数据
            if len(df) < self.look_back:
                raise ValueError(f"数据不足，至少需要{self.look_back}条历史数据")

            # 1. 计算时间间隔（动态获取而非硬编码）
            if len(df) >= 2:
                time_diff = df['time'].iloc[-1] - df['time'].iloc[-2]
                freq = f"{time_diff.total_seconds()}s"
            else:
                freq = '3s'  # 默认值

            # 2. 获取最后一个有效时间戳
            last_timestamp = df['time'].iloc[-1]
            if pd.isnull(last_timestamp):
                last_timestamp = pd.Timestamp.now()
                print("[Warning] 使用当前时间作为最后时间戳")

            # 3. 生成连续时间序列
            time_index = pd.date_range(
                start=last_timestamp + time_diff,  # 从下一个时间点开始
                periods=steps,
                freq=freq,
                tz=last_timestamp.tz if hasattr(last_timestamp, 'tz') else None
            )

            # 确保scaler已经拟合（仅使用原始特征）
            numeric_data = df[feature_columns].values
            if not hasattr(self.scaler, 'scale_'):
                self.scaler.fit(numeric_data)

            # 使用最后look_back个数据点
            last_data = numeric_data[-self.look_back:]

            # 数据标准化（仅对原始特征）
            scaled_data = self.scaler.transform(last_data)

            # 特征增强（原始特征+差分特征）
            enhanced_data = self._enhance_features(scaled_data)

            # 初始化预测序列
            current_sequence = enhanced_data.reshape(1, self.look_back, -1)
            predictions = []

            # 逐步预测
            for _ in range(steps):
                pred = self.model.predict(current_sequence, verbose=0)[0]
                predictions.append(pred)

                # 创建新特征：原始特征 + 差分特征
                new_features = np.concatenate([
                    pred,  # 原始特征预测结果
                    pred - current_sequence[0, -1, :num_features]  # 差分特征
                ])

                # 更新输入序列
                current_sequence = np.concatenate([
                    current_sequence[:, 1:, :],
                    new_features.reshape(1, 1, -1)
                ], axis=1)

            # 反标准化处理
            predictions = np.array(predictions)

            # 处理维度匹配（关键修复）
            if predictions.shape[1] != num_features:
                print(f"[Warning] 预测维度({predictions.shape[1]})与特征数({num_features})不匹配，自动调整")
                dummy = np.zeros((predictions.shape[0], num_features))
                dummy[:, :predictions.shape[1]] = predictions
                predictions = dummy

            denorm_predictions = self.scaler.inverse_transform(predictions)

            # 生成时间序列（确保时区一致）
            last_timestamp = df['time'].iloc[-1]
            if pd.isnull(last_timestamp):
                last_timestamp = pd.Timestamp.now()

            time_index = pd.date_range(
                start=last_timestamp + pd.Timedelta(seconds=3),
                periods=steps,
                freq='3s',
                tz=last_timestamp.tz if hasattr(last_timestamp, 'tz') else None
            )

            # 创建结果DataFrame
            result_df = pd.DataFrame(
                denorm_predictions,
                columns=feature_columns[:denorm_predictions.shape[1]]
            )

            # 4. 精确对齐时间戳
            result_df.insert(0, 'time', time_index)
            self.last_prediction = result_df

            # 保存结果
            os.makedirs('predictions', exist_ok=True)
            result_path = 'predictions/latest_prediction.csv'
            result_df.to_csv(
                result_path,
                index=False,
                date_format='%Y-%m-%d %H:%M:%S.%f',  # 包含毫秒精度
                encoding='utf-8-sig'
            )

            print(f"成功生成{steps}步预测结果，时间连续性已保持")
            return result_df

        except Exception as e:
            print(f"预测失败: {str(e)}")
            import traceback
            traceback.print_exc()
            raise

    def get_last_prediction(self):
        """获取最近一次预测结果"""
        if self.last_prediction is None:
            raise ValueError("没有可用的预测结果，请先调用predict_future()")
        return self.last_prediction.copy()

    @property
    def is_initialized(self):
        return self._is_initialized


if __name__ == "__main__":
    predictor = LSTMPredictor('data_raw.csv', look_back=16)
    # predictor._try_load_model()
    predictor.prepare_data()
    future = predictor.predict_future(steps=120)
    print(future.head())