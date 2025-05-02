import os
import shutil
import threading

import serial
import serial.tools.list_ports
import pandas as pd
from threading import Thread, Lock
import time
from datetime import datetime
import logging


class SerialDataLogger:
    def __init__(self, filename='data_raw.csv', interval=5, baudrate=115200):
        self.filename = filename
        self.interval = interval
        self.baudrate = baudrate
        self.ser = None
        self.is_running = False
        self.lock = Lock()

        # 初始化数据存储结构
        self.data_dict = {
            2025: {'pitch': None, 'roll': None, 'range': None, 'updated': False},
            2026: {'pitch': None, 'roll': None, 'range': None, 'updated': False}
        }

        # 初始化CSV文件
        self._init_csv()

        # 配置日志
        logging.basicConfig(level=logging.INFO)

    def _init_csv(self):
        """改进的CSV初始化方法（防止文件重置）"""
        columns = ['time', 'pitch_1 (°)', 'roll_1 (°)', 'range_1 (mm)',
                   'pitch_2 (°)', 'roll_2 (°)', 'range_2 (mm)']
        if not os.path.exists(self.filename):  # 仅当文件不存在时创建
            pd.DataFrame(columns=columns).to_csv(self.filename, index=False, encoding='utf-8-sig')

    @staticmethod
    def _find_ch340_port():
        """查找CH340设备"""
        ports = serial.tools.list_ports.comports()
        for port in ports:
            if 'CH340' in port.description:
                return port.device
        raise Exception("CH340 device not found")

    def _connect_serial(self):
        """改进的串口连接方法（解决冷启动问题）"""
        port = self._find_ch340_port()

        # 初始化
        self.ser = serial.Serial(
            port=port,
            baudrate=self.baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1
        )

        # 关键：模拟终端行为的rts控制
        self.ser.dtr = False
        self.ser.rts = True  # 先拉
        time.sleep(1)
        self.ser.rts = False  # 再拉触发复位
        self.ser.reset_input_buffer()

        logging.info(f"Serial initialized (DTR={self.ser.dtr}, RTS={self.ser.rts})")

    def _read_serial(self):
        """可靠的串口读取方法"""
        buffer = b''
        while self.is_running:
            try:
                # 非阻塞读取模式
                data = self.ser.read_all()
                if data:
                    buffer += data
                    # print(f"[HEX DUMP] {data.hex()}")  # 调试输出

                # 处理完整数据帧（假设用\n分割）
                while b'\n' in buffer:
                    line, _, buffer = buffer.partition(b'\n')
                    line = line.decode('utf-8').strip()
                    if line:
                        # print(f"[RECV] {line}")
                        self._process_data(line)

                time.sleep(0.01)  # 防止CPU占用过高

            except Exception as e:
                logging.error(f"Read error: {str(e)}")
                time.sleep(1)

    def _process_data(self, raw_data):
        """处理原始数据（增强健壮性）"""
        try:
            # 强化数据清洗
            clean_data = raw_data.replace(' ', '').strip()
            if not clean_data:
                return

            parts = clean_data.split(',')
            if len(parts) != 4:
                # logging.warning(f"Invalid data length: {raw_data}")
                return

            # 验证地址有效性
            try:
                addr = int(parts[0])
            except ValueError:
                logging.warning(f"Invalid address: {parts[0]}")
                return

            if addr not in [2025, 2026]:
                # logging.debug(f"Ignored address: {addr}")  # 新增调试日志
                return

            # 数值转换验证
            try:
                pitch = float(parts[1])
                roll = float(parts[2])
                rng = float(parts[3])
            except ValueError as e:
                # logging.warning(f"Value error in {raw_data}: {str(e)}")
                return

            # range数值范围验证
            if rng < 0 or rng > 255:
                # logging.warning(f"Invalid range value: {rng} (0-255 allowed)")
                return

            with self.lock:
                self.data_dict[addr]['pitch'] = pitch
                self.data_dict[addr]['roll'] = roll
                self.data_dict[addr]['range'] = rng
                self.data_dict[addr]['updated'] = True
                # print(f"[UPDATE] Addr {addr} updated: {pitch}, {roll}, {rng}")  # 新增更新日志

        except Exception as e:
            logging.error(f"Processing error: {str(e)}")

    def _save_data(self):
        """定时保存数据的线程函数（增加文件复制功能）"""
        while self.is_running:
            time.sleep(self.interval)
            with self.lock:
                # 检查两个传感器数据是否都已更新
                if not (self.data_dict[2025]['updated'] and self.data_dict[2026]['updated']):
                    continue

                # 准备数据行
                new_data = {
                    'time': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                    'pitch_1 (°)': self.data_dict[2025]['pitch'],
                    'roll_1 (°)': self.data_dict[2025]['roll'],
                    'range_1 (mm)': self.data_dict[2025]['range'],
                    'pitch_2 (°)': self.data_dict[2026]['pitch'],
                    'roll_2 (°)': self.data_dict[2026]['roll'],
                    'range_2 (mm)': self.data_dict[2026]['range']
                }

                # 重置更新标志
                self.data_dict[2025]['updated'] = False
                self.data_dict[2026]['updated'] = False

            # 使用pandas追加数据
            try:
                # 保存原始数据文件
                pd.DataFrame([new_data]).to_csv(
                    self.filename,
                    mode='a',
                    header=False,
                    index=False,
                    encoding='utf-8-sig'
                )

                # 新增：复制文件到data.csv
                try:
                    # 先保存再复制确保数据完整性
                    shutil.copy2(self.filename, 'building_data.csv')
                    # print(f"[{datetime.now().strftime('%H:%M:%S')}] Data copied to building_data.csv")
                except Exception as copy_error:
                    logging.error(f"File copy failed: {str(copy_error)}")

                # 打印存储信息
                # print(f"[{datetime.now().strftime('%H:%M:%S')}] Data saved - "
                #       f"P1: {new_data['pitch_1 (°)']:.2f}° "
                #       f"R1: {new_data['roll_1 (°)']:.2f}° | "
                #       f"P2: {new_data['pitch_2 (°)']:.2f}° "
                #       f"R2: {new_data['roll_2 (°)']:.2f}°")

            except Exception as e:
                logging.error(f"CSV save error: {str(e)}")

    def start(self):
        """启动日志记录"""
        if not self.is_running:
            self.is_running = True
            self._connect_serial()

            # 启动读取线程
            read_thread = Thread(target=self._read_serial, daemon=True)
            read_thread.start()

            # 启动保存线程
            save_thread = Thread(target=self._save_data, daemon=True)
            save_thread.start()

            logging.info("Data logging started")

    def stop(self):
        """停止日志记录"""
        if self.is_running:
            self.is_running = False
            if self.ser and self.ser.is_open:
                self.ser.close()
            logging.info("Data logging stopped")



def process_excel_data():
    """预留的数据处理线程"""
    while True:
        # 在这里添加Excel数据处理逻辑
        time.sleep(5)
        print("Processing excel data...")


if __name__ == "__main__":
    logger = SerialDataLogger(interval=5)
    retry_count = 0

    while retry_count < 3:
        try:
            logger.start()
            while True:
                time.sleep(1)
                # 监控线程状态
                if not any(t.is_alive() for t in threading.enumerate() if t.name != "MainThread"):
                    raise RuntimeError("Thread died")

        except (serial.SerialException, RuntimeError) as e:
            logger.stop()
            print(f"Connection failed: {str(e)}, retrying... ({retry_count + 1}/3)")
            retry_count += 1
            time.sleep(2)

        except KeyboardInterrupt:
            logger.stop()
            break

    if retry_count >= 3:
        print("Failed after 3 attempts")