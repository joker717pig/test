"""
版本一：ECG（提R波）+ PPG（提脉搏峰）→ 算PTT/波幅等 → 特征输入模型 → 判OH → 串口返回STM32
功能：实时接收串口数据，进行OH判定，返回结果给STM32
使用数据：实时串口数据（HR, SPO2, 心电波形, PPG波形）
OH判定结果：通过串口返回给STM32
"""

import numpy as np
import serial
import serial.tools.list_ports
import time
import threading
from collections import deque
from scipy.signal import find_peaks, butter, filtfilt
import warnings
import struct
import sys
import os

warnings.filterwarnings('ignore')

# -------------------------- 配置参数 --------------------------
SERIAL_PORT = "COM7"  # 串口号，根据实际情况修改
BAUD_RATE = 115200
FS = 100  # 采样频率

# 数据缓冲区大小
HR_BUFFER_SIZE = 100  # 心率缓冲区
SPO2_BUFFER_SIZE = 100  # 血氧缓冲区
ECG_BUFFER_SIZE = 1000  # 心电波形缓冲区
PPG_BUFFER_SIZE = 1000  # PPG波形缓冲区


# -------------------------- 信号处理函数 --------------------------
def butter_bandpass(lowcut, highcut, fs, order=4):
    """巴特沃斯带通滤波器"""
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b, a


def bandpass_filter(data, lowcut, highcut, fs, order=4):
    """应用带通滤波器"""
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = filtfilt(b, a, data)
    return y


def detect_r_peaks_ecg(ecg_signal, fs):
    """检测ECG信号的R波峰值（优化版）"""
    if len(ecg_signal) < fs * 2:  # 至少2秒数据
        return np.array([]), ecg_signal, np.array([])

    # 带通滤波 (0.5-40 Hz)
    ecg_filtered = bandpass_filter(ecg_signal, 0.5, 40, fs)

    # 微分
    ecg_diff = np.diff(ecg_filtered)

    # 平方
    ecg_squared = ecg_diff ** 2

    # 移动平均
    window_size = int(0.15 * fs)
    window = np.ones(window_size) / window_size
    ecg_integrated = np.convolve(ecg_squared, window, mode='same')

    # 自适应阈值
    threshold = np.percentile(ecg_integrated, 85)

    # 寻找R波
    min_distance = int(0.3 * fs)
    r_peaks, _ = find_peaks(
        ecg_integrated,
        distance=min_distance,
        height=threshold,
        prominence=threshold * 0.5
    )

    return r_peaks, ecg_filtered, ecg_integrated


def detect_ppg_peaks(ppg_signal, fs):
    """检测PPG信号的脉搏波峰值"""
    if len(ppg_signal) < fs * 2:  # 至少2秒数据
        return np.array([]), np.array([]), ppg_signal

    # 带通滤波 (0.5-8 Hz) - PPG信号的频率范围
    ppg_filtered = bandpass_filter(ppg_signal, 0.5, 8, fs)

    # 寻找脉搏波峰值
    min_distance = int(0.4 * fs)  # 最小距离，对应心率上限150bpm
    height_threshold = np.percentile(ppg_filtered, 70)

    # 寻找主峰（收缩峰）
    systolic_peaks, _ = find_peaks(
        ppg_filtered,
        distance=min_distance,
        height=height_threshold
    )

    return systolic_peaks, [], ppg_filtered


def calculate_ptt_ecg_ppg(ecg_signal, ppg_signal, fs):
    """计算ECG R波到PPG脉搏波的PTT（脉搏波传导时间）"""
    # 检测R波
    r_peaks, ecg_filtered, _ = detect_r_peaks_ecg(ecg_signal, fs)

    # 检测PPG波
    ppg_peaks, _, ppg_filtered = detect_ppg_peaks(ppg_signal, fs)

    if len(r_peaks) < 3 or len(ppg_peaks) < 3:
        return None, False

    # 计算PTT：R波到下一个PPG波的时间差
    ptt_values = []
    valid_pairs = []

    for r_peak in r_peaks[:20]:  # 限制数量以提高效率
        # 找到该R波之后的所有PPG波
        subsequent_ppg = ppg_peaks[ppg_peaks > r_peak]
        if len(subsequent_ppg) > 0:
            ptt_ms = (subsequent_ppg[0] - r_peak) / fs * 1000

            # 有效性检查
            if 50 < ptt_ms < 500:  # 合理范围
                ptt_values.append(ptt_ms)
                valid_pairs.append((r_peak, subsequent_ppg[0]))

    if len(ptt_values) >= 3:  # 降低要求，实时处理
        ptt_mean = np.mean(ptt_values)
        ptt_std = np.std(ptt_values)
        ptt_cv = ptt_std / ptt_mean if ptt_mean > 0 else 0

        return {
            "ptt_mean": ptt_mean,
            "ptt_std": ptt_std,
            "ptt_cv": ptt_cv,
            "ptt_count": len(ptt_values),
            "valid_pairs": valid_pairs
        }, True
    else:
        return None, False


def calculate_hrv_features(r_peaks, fs):
    """计算心率变异性特征"""
    if len(r_peaks) < 4:
        return None

    # RR间期
    rr_intervals = np.diff(r_peaks) / fs * 1000  # 毫秒

    # 基础统计
    mean_rr = np.mean(rr_intervals)
    std_rr = np.std(rr_intervals)
    cv_rr = std_rr / mean_rr if mean_rr > 0 else 0

    # RMSSD（相邻RR间期差值的均方根）
    if len(rr_intervals) > 1:
        rmssd = np.sqrt(np.mean(np.diff(rr_intervals) ** 2))
    else:
        rmssd = 0

    # pNN50（相邻RR间期差值大于50ms的比例）
    if len(rr_intervals) > 1:
        nn50 = np.sum(np.abs(np.diff(rr_intervals)) > 50)
        pnn50 = nn50 / (len(rr_intervals) - 1) * 100 if len(rr_intervals) > 1 else 0
    else:
        pnn50 = 0

    return {
        "mean_rr": mean_rr,
        "std_rr": std_rr,
        "cv_rr": cv_rr,
        "rmssd": rmssd,
        "pnn50": pnn50,
        "heart_rate": 60000 / mean_rr if mean_rr > 0 else 0
    }


def calculate_ppg_features(ppg_signal, peaks, fs):
    """计算PPG波形特征"""
    if len(peaks) < 3:
        return None

    amplitudes = ppg_signal[peaks]
    systolic_mean = np.mean(amplitudes)
    systolic_std = np.std(amplitudes)

    # 计算脉幅（需要找到波谷）
    pulse_amplitudes = []
    for i in range(len(peaks) - 1):
        peak1 = peaks[i]
        peak2 = peaks[i + 1]
        segment = ppg_signal[peak1:peak2]
        if len(segment) > 0:
            valley = np.min(segment)
            pulse_amplitude = ppg_signal[peak1] - valley
            pulse_amplitudes.append(pulse_amplitude)

    pa_mean = np.mean(pulse_amplitudes) if pulse_amplitudes else 0

    # 计算上升时间
    rise_times = []
    for peak in peaks:
        # 寻找波谷
        search_window = int(0.3 * fs)
        start_idx = max(0, peak - search_window)
        segment = ppg_signal[start_idx:peak]
        if len(segment) > 0:
            valley_idx = start_idx + np.argmin(segment)
            rise_time = (peak - valley_idx) / fs * 1000
            if 20 < rise_time < 300:
                rise_times.append(rise_time)

    rise_time_mean = np.mean(rise_times) if rise_times else 0

    return {
        "amplitude_mean": systolic_mean,
        "amplitude_std": systolic_std,
        "pulse_amplitude_mean": pa_mean,
        "rise_time_mean": rise_time_mean,
        "peak_count": len(peaks)
    }


# -------------------------- OH判定模型 --------------------------
def oh_judgment_model_v2(features, ppg_features=None):
    """
    OH判定模型：基于ECG+PPG特征
    返回：True（阳性）/ False（阴性）
    """
    if not features:
        return False, "数据不足"

    ptt_info = features.get("ptt_info")
    hrv_features = features.get("hrv_features")
    has_ptt = features.get("has_ptt", False)

    # 综合评分系统
    risk_score = 0
    reasons = []

    # 1. PTT指标（最重要）
    if has_ptt and ptt_info:
        ptt_mean = ptt_info["ptt_mean"]
        ptt_cv = ptt_info.get("ptt_cv", 0)

        if ptt_mean < 140:  # 更严格的阈值
            risk_score += 2
            reasons.append(f"PTT过短 ({ptt_mean:.1f}ms)")
        elif ptt_mean > 280:
            risk_score += 2
            reasons.append(f"PTT过长 ({ptt_mean:.1f}ms)")

        if ptt_cv > 0.25:  # PTT变异性过大
            risk_score += 1
            reasons.append(f"PTT变异性高 (CV:{ptt_cv:.2f})")

    # 2. HRV指标
    if hrv_features:
        hr = hrv_features.get("heart_rate", 0)
        rmssd = hrv_features.get("rmssd", 0)
        pnn50 = hrv_features.get("pnn50", 0)

        # 心率异常
        if hr > 100 or hr < 50:
            risk_score += 1
            reasons.append(f"心率异常 ({hr:.1f}bpm)")

        # 自主神经功能异常（低HRV）
        if rmssd < 20:  # 低RMSSD提示交感神经占优
            risk_score += 1
            reasons.append(f"HRV降低 (RMSSD:{rmssd:.1f}ms)")

        if pnn50 < 5:  # 低pNN50
            risk_score += 1
            reasons.append(f"pNN50降低 ({pnn50:.1f}%)")

    # 3. PPG特征（如果有）
    if ppg_features:
        amplitude_mean = ppg_features.get("amplitude_mean", 0)
        pulse_amplitude = ppg_features.get("pulse_amplitude_mean", 0)

        if amplitude_mean < 0.3:  # 幅值过低（需要根据实际数据调整）
            risk_score += 1
            reasons.append(f"PPG幅值低 ({amplitude_mean:.2f})")

        if pulse_amplitude < 0.15:  # 脉幅过小
            risk_score += 1
            reasons.append(f"脉幅小 ({pulse_amplitude:.2f})")

    # 判定规则
    if risk_score >= 3:
        return True, "; ".join(reasons)
    elif risk_score >= 2 and has_ptt:
        return True, f"中度风险: {'; '.join(reasons)}"
    else:
        return False, "正常" if risk_score == 0 else f"低风险: {risk_score}分"


# -------------------------- 串口数据处理类 --------------------------
class SerialOHDetector:
    def __init__(self, port, baud_rate, fs=100):
        self.port = port
        self.baud_rate = baud_rate
        self.fs = fs

        # 数据缓冲区
        self.hr_buffer = deque(maxlen=HR_BUFFER_SIZE)
        self.spo2_buffer = deque(maxlen=SPO2_BUFFER_SIZE)
        self.ecg_buffer = deque(maxlen=ECG_BUFFER_SIZE)  # 心电波形（通道2）
        self.ppg_buffer = deque(maxlen=PPG_BUFFER_SIZE)  # PPG波形（通道1）

        # 串口对象
        self.ser = None

        # 处理标志
        self.processing = False
        self.last_judgment_time = 0
        self.judgment_interval = 5.0  # 每5秒进行一次OH判定

        # 结果
        self.last_oh_result = False
        self.last_oh_reason = ""
        self.oh_risk_level = 0  # 0:数据不足, 1:正常, 2:轻度, 3:中度, 4:重度

    def connect_serial(self):
        """连接串口"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baud_rate,
                timeout=1
            )
            print(f"✅ 已连接到串口: {self.port} ({self.baud_rate} baud)")
            return True
        except Exception as e:
            print(f"❌ 串口连接失败: {e}")
            return False

    def parse_serial_data(self, line):
        """解析串口数据行"""
        try:
            # 示例格式: "HR:  75 SPO2:  98 HR_AVG:  75.00 SPO2_AVG:  98.00 B:  1234567 A:  2345678 C:  72.50"
            # B: PPG波形数据, A: 心电波形数据
            if "HR:" in line and "SPO2:" in line and "B:" in line and "A:" in line:
                parts = line.split()

                # 提取心率
                hr_idx = parts.index("HR:")
                hr = int(parts[hr_idx + 1])

                # 提取血氧
                spo2_idx = parts.index("SPO2:")
                spo2 = int(parts[spo2_idx + 1])

                # 提取心电波形（A值）
                a_idx = parts.index("A:")
                ecg_value = float(parts[a_idx + 1])

                # 提取PPG波形（B值）
                b_idx = parts.index("B:")
                ppg_value = float(parts[b_idx + 1])

                # 提取心率（C值，备用）
                c_idx = parts.index("C:")
                bpm = float(parts[c_idx + 1])

                # 更新缓冲区
                self.hr_buffer.append(hr)
                self.spo2_buffer.append(spo2)
                self.ecg_buffer.append(ecg_value)
                self.ppg_buffer.append(ppg_value)

                return {
                    "hr": hr,
                    "spo2": spo2,
                    "ecg": ecg_value,
                    "ppg": ppg_value,
                    "bpm": bpm,
                    "timestamp": time.time()
                }
        except Exception as e:
            print(f"⚠️ 数据解析错误: {e}")

        return None

    def extract_features(self):
        """从缓冲区提取特征"""
        if len(self.ecg_buffer) < FS * 3 or len(self.ppg_buffer) < FS * 3:
            return None  # 数据不足

        # 转换为numpy数组
        ecg_signal = np.array(self.ecg_buffer)
        ppg_signal = np.array(self.ppg_buffer)

        features = {}

        # 1. 计算PTT
        ptt_info, has_ptt = calculate_ptt_ecg_ppg(ecg_signal, ppg_signal, self.fs)
        features["ptt_info"] = ptt_info
        features["has_ptt"] = has_ptt

        # 2. 提取ECG特征（HRV）
        r_peaks, _, _ = detect_r_peaks_ecg(ecg_signal, self.fs)
        hrv_features = calculate_hrv_features(r_peaks, self.fs)
        features["hrv_features"] = hrv_features

        # 3. 提取PPG特征
        ppg_peaks, _, _ = detect_ppg_peaks(ppg_signal, self.fs)
        ppg_features = calculate_ppg_features(ppg_signal, ppg_peaks, self.fs)
        features["ppg_features"] = ppg_features

        # 4. 添加平均HR和SPO2
        if self.hr_buffer:
            features["hr_mean"] = np.mean(list(self.hr_buffer))
        if self.spo2_buffer:
            features["spo2_mean"] = np.mean(list(self.spo2_buffer))

        return features

    def judge_oh_status(self, features):
        """判断OH状态并返回风险等级"""
        if not features:
            return 0, "数据不足"

        # 使用模型判断
        is_oh_positive, reason = oh_judgment_model_v2(
            features,
            features.get("ppg_features")
        )

        # 转换为风险等级
        if not features.get("has_ptt"):
            risk_level = 0  # 数据不足
        elif is_oh_positive:
            # 根据PTT值判断严重程度
            ptt_mean = features.get("ptt_info", {}).get("ptt_mean", 0)
            hr_mean = features.get("hr_mean", 0)

            if ptt_mean < 100 or ptt_mean > 300 or hr_mean > 120:
                risk_level = 4  # 重度低血压
            elif ptt_mean < 120 or ptt_mean > 280 or hr_mean > 110:
                risk_level = 3  # 中度低血压
            else:
                risk_level = 2  # 轻度低血压
        else:
            risk_level = 1  # 正常

        return risk_level, reason

    def send_risk_level(self, risk_level):
        """通过串口发送风险等级给STM32"""
        if self.ser and self.ser.is_open:
            try:
                # 发送格式: "RISK:X\n" 其中X为0-4的数字
                message = f"RISK:{risk_level}\n"
                self.ser.write(message.encode('utf-8'))
                print(f"📤 发送风险等级: {risk_level}")
                return True
            except Exception as e:
                print(f"❌ 串口发送失败: {e}")
                return False
        return False

    def process_data(self):
        """处理接收到的数据"""
        if not self.ser or not self.ser.is_open:
            return

        try:
            # 读取一行数据
            line = self.ser.readline().decode('utf-8', errors='ignore').strip()

            if line and len(line) > 20:  # 有效数据行
                # 解析数据
                data = self.parse_serial_data(line)

                if data:
                    current_time = time.time()

                    # 定期进行OH判定（每5秒一次）
                    if current_time - self.last_judgment_time >= self.judgment_interval:
                        # 提取特征
                        features = self.extract_features()

                        if features:
                            # 判断OH状态
                            risk_level, reason = self.judge_oh_status(features)

                            # 保存结果
                            self.oh_risk_level = risk_level
                            self.last_oh_reason = reason
                            self.last_oh_result = risk_level >= 2

                            # 发送给STM32
                            self.send_risk_level(risk_level)

                            # 打印结果
                            risk_level_names = ["数据不足", "正常", "轻度低血压", "中度低血压", "重度低血压"]
                            print(f"📊 OH判定结果: {risk_level_names[risk_level]}")
                            if reason:
                                print(f"📝 原因: {reason}")

                            # 显示特征信息
                            if features.get("has_ptt"):
                                ptt_mean = features.get("ptt_info", {}).get("ptt_mean", 0)
                                print(f"📈 PTT均值: {ptt_mean:.1f}ms")

                            if features.get("hr_mean"):
                                print(f"💓 平均心率: {features['hr_mean']:.1f}bpm")

                            print("-" * 50)

                        self.last_judgment_time = current_time

        except Exception as e:
            print(f"⚠️ 数据处理异常: {e}")

    def start_monitoring(self):
        """开始监测"""
        if not self.connect_serial():
            return

        print("🚀 开始OH监测...")
        print("=" * 60)

        self.processing = True
        self.last_judgment_time = time.time()

        try:
            while self.processing:
                self.process_data()
                time.sleep(0.01)  # 短暂延迟，避免CPU占用过高

        except KeyboardInterrupt:
            print("\n🛑 用户中断监测")
        finally:
            self.stop_monitoring()

    def stop_monitoring(self):
        """停止监测"""
        self.processing = False
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("✅ 串口已关闭")

    def print_status(self):
        """打印当前状态"""
        print("\n" + "=" * 60)
        print("📋 当前监测状态")
        print("=" * 60)

        print(f"📊 数据缓冲区:")
        print(f"  心率数据: {len(self.hr_buffer)}/{HR_BUFFER_SIZE}")
        print(f"  血氧数据: {len(self.spo2_buffer)}/{SPO2_BUFFER_SIZE}")
        print(f"  心电波形: {len(self.ecg_buffer)}/{ECG_BUFFER_SIZE}")
        print(f"  PPG波形: {len(self.ppg_buffer)}/{PPG_BUFFER_SIZE}")

        risk_level_names = ["数据不足", "正常", "轻度低血压", "中度低血压", "重度低血压"]
        print(f"\n🩺 最后判定结果:")
        print(f"  风险等级: {self.oh_risk_level} - {risk_level_names[self.oh_risk_level]}")
        print(f"  判定原因: {self.last_oh_reason}")
        print("=" * 60)


# -------------------------- 主程序 --------------------------
def list_serial_ports():
    """列出可用的串口"""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("⚠️ 未找到可用的串口")
        return []

    print("📡 可用串口列表:")
    for i, port in enumerate(ports):
        print(f"  [{i}] {port.device} - {port.description}")

    return ports


def main():
    """主函数"""
    print("=" * 60)
    print("🚀 版本一：ECG+PPG OH判定系统 - 串口监测版")
    print("=" * 60)

    # 列出串口
    ports = list_serial_ports()

    if not ports:
        # 使用默认串口
        port_name = SERIAL_PORT
    else:
        # 让用户选择串口
        if len(ports) == 1:
            port_name = ports[0].device
            print(f"✅ 自动选择串口: {port_name}")
        else:
            try:
                choice = int(input("🔢 请选择串口号 (输入数字): "))
                if 0 <= choice < len(ports):
                    port_name = ports[choice].device
                else:
                    print("⚠️ 选择无效，使用默认串口")
                    port_name = SERIAL_PORT
            except:
                port_name = SERIAL_PORT

    print(f"🎯 使用串口: {port_name}")

    # 创建检测器
    detector = SerialOHDetector(port_name, BAUD_RATE, FS)

    # 开始监测
    try:
        # 启动监测线程
        monitor_thread = threading.Thread(target=detector.start_monitoring)
        monitor_thread.daemon = True
        monitor_thread.start()

        # 主线程等待
        while True:
            cmd = input("\n📝 输入命令 (status:状态, quit:退出): ").strip().lower()

            if cmd == "status":
                detector.print_status()
            elif cmd == "quit" or cmd == "exit":
                print("👋 正在退出...")
                detector.stop_monitoring()
                time.sleep(1)
                break
            elif cmd == "test":
                # 测试发送风险等级
                test_level = int(input("🔢 输入测试风险等级 (0-4): "))
                if 0 <= test_level <= 4:
                    detector.send_risk_level(test_level)
                else:
                    print("⚠️ 风险等级必须在0-4之间")
            else:
                print("⚠️ 未知命令")

    except KeyboardInterrupt:
        print("\n👋 用户中断程序")
        detector.stop_monitoring()

    print("✅ 程序已退出")


if __name__ == "__main__":
    main()