"""
GUI版：ECG(ADS1292/文本串口A通道) + PPG(文本串口B通道) OH筛查系统
稳定版修复 + 同串口收发功能
"""

import numpy as np
import pandas as pd
from scipy.signal import find_peaks, butter, filtfilt, medfilt
import warnings
import serial
import serial.tools.list_ports
import time
from datetime import datetime
import sqlite3
from collections import deque
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *
import pyqtgraph as pg
import sys
import threading

warnings.filterwarnings('ignore')

FS = 100
SERIAL_PORT = ""
BAUDRATE = 115200
TIMEOUT = 0.1
GUI_REFRESH_MS = 50       # 20 FPS
PARAM_REFRESH_MS = 250    # 4 FPS
VIEW_REFRESH_MS = 300     # 自动缩放节流
MAX_DEBUG_LINES = 200

THRESHOLDS = {
    'ptt_low': 140,
    'ptt_high': 280,
    'hr_low': 50,
    'hr_high': 100,
    'rmssd_low': 20,
    'spo2_low': 94,
    'pi_low': 0.3,
    'rise_time_low': 50,
    'rise_time_high': 180,
    'amp_cv_high': 30,
}


def butter_bandpass(lowcut, highcut, fs, order=4):
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    if low <= 0:
        low = 1e-6
    if high >= 1:
        high = 0.999999
    if low >= high:
        return None, None
    b, a = butter(order, [low, high], btype='band')
    return b, a


def bandpass_filter(data, lowcut, highcut, fs, order=4):
    arr = np.asarray(data, dtype=float)
    if len(arr) < max(16, order * 3):
        return arr
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    if b is None or a is None:
        return arr
    try:
        return filtfilt(b, a, arr)
    except Exception:
        return arr


def detect_r_peaks_ecg(ecg_signal, fs):
    ecg_signal = np.asarray(ecg_signal, dtype=float)
    if len(ecg_signal) < fs * 2:
        return np.array([]), ecg_signal, np.array([])
    ecg_filtered = bandpass_filter(ecg_signal, 0.5, 40, fs)
    ecg_diff = np.diff(ecg_filtered)
    if len(ecg_diff) == 0:
        return np.array([]), ecg_filtered, np.array([])
    ecg_diff = np.append(ecg_diff, ecg_diff[-1])
    ecg_squared = ecg_diff ** 2
    window_size = max(3, int(0.15 * fs))
    window = np.ones(window_size) / window_size
    ecg_integrated = np.convolve(ecg_squared, window, mode='same')
    recent = ecg_integrated[max(0, len(ecg_integrated) - 5 * fs):]
    threshold = np.percentile(recent, 85) if len(recent) else 0
    min_distance = int(0.3 * fs)
    r_peaks, _ = find_peaks(
        ecg_integrated,
        distance=min_distance,
        height=threshold,
        prominence=max(threshold * 0.5, 1e-9)
    )
    return r_peaks, ecg_filtered, ecg_integrated


def detect_ppg_peaks(ppg_signal, fs):
    ppg_signal = np.asarray(ppg_signal, dtype=float)
    if len(ppg_signal) < fs * 2:
        return np.array([]), [], ppg_signal
    ppg_filtered = bandpass_filter(ppg_signal, 0.5, 8, fs)
    ksize = int(fs) if int(fs) % 2 == 1 else int(fs) + 1
    ksize = max(3, ksize)
    if len(ppg_filtered) > ksize:
        baseline = medfilt(ppg_filtered, kernel_size=ksize)
        ppg_corrected = ppg_filtered - baseline
    else:
        ppg_corrected = ppg_filtered
    std = np.std(ppg_corrected)
    if std > 0:
        ppg_norm = (ppg_corrected - np.mean(ppg_corrected)) / std
    else:
        ppg_norm = ppg_corrected
    min_distance = int(0.4 * fs)
    th = np.percentile(ppg_norm, 60) if len(ppg_norm) else 0
    peaks, _ = find_peaks(ppg_norm, distance=min_distance, height=th, prominence=0.2)
    return peaks, [], ppg_norm


def calculate_ptt_ecg_ppg(ecg_signal, ppg_signal, fs):
    r_peaks, _, _ = detect_r_peaks_ecg(ecg_signal, fs)
    ppg_peaks, _, _ = detect_ppg_peaks(ppg_signal, fs)
    if len(r_peaks) < 3 or len(ppg_peaks) < 3:
        return None, False
    ptt_values = []
    valid_pairs = []
    for r_peak in r_peaks[:50]:
        subsequent = ppg_peaks[ppg_peaks > r_peak]
        if len(subsequent) > 0:
            ptt_ms = (subsequent[0] - r_peak) / fs * 1000
            if 50 < ptt_ms < 600:
                ptt_values.append(ptt_ms)
                valid_pairs.append((r_peak, subsequent[0]))
    if len(ptt_values) >= 3:
        ptt_mean = np.mean(ptt_values)
        ptt_std = np.std(ptt_values)
        ptt_cv = ptt_std / ptt_mean if ptt_mean > 0 else 0
        return {
            'ptt_mean': ptt_mean,
            'ptt_std': ptt_std,
            'ptt_cv': ptt_cv,
            'ptt_count': len(ptt_values),
            'valid_pairs': valid_pairs,
        }, True
    return None, False


def calculate_hrv_features(r_peaks, fs):
    if len(r_peaks) < 4:
        return None
    rr_intervals = np.diff(r_peaks) / fs * 1000
    if len(rr_intervals) == 0:
        return None
    mean_rr = np.mean(rr_intervals)
    std_rr = np.std(rr_intervals)
    cv_rr = std_rr / mean_rr if mean_rr > 0 else 0
    rmssd = np.sqrt(np.mean(np.diff(rr_intervals) ** 2)) if len(rr_intervals) > 1 else 0
    if len(rr_intervals) > 1:
        nn50 = np.sum(np.abs(np.diff(rr_intervals)) > 50)
        pnn50 = nn50 / (len(rr_intervals) - 1) * 100
    else:
        pnn50 = 0
    return {
        'mean_rr': mean_rr,
        'std_rr': std_rr,
        'cv_rr': cv_rr,
        'rmssd': rmssd,
        'pnn50': pnn50,
        'heart_rate': 60000 / mean_rr if mean_rr > 0 else 0,
    }


def calculate_ppg_features(ppg_signal, peaks, fs):
    if len(peaks) < 3:
        return None
    ppg_signal = np.asarray(ppg_signal, dtype=float)
    amplitudes = ppg_signal[peaks]
    amp_mean = np.mean(amplitudes)
    amp_std = np.std(amplitudes)
    amp_cv = (amp_std / abs(amp_mean) * 100) if amp_mean != 0 else 0
    rise_times = []
    for peak in peaks:
        search_window = int(0.3 * fs)
        start_idx = max(0, peak - search_window)
        segment = ppg_signal[start_idx:peak]
        if len(segment) > 5:
            valley_idx = start_idx + np.argmin(segment)
            rise_time = (peak - valley_idx) / fs * 1000
            if 20 < rise_time < 300:
                rise_times.append(rise_time)
    rise_time_mean = np.mean(rise_times) if rise_times else 0
    periods = []
    for i in range(len(peaks) - 1):
        period = (peaks[i + 1] - peaks[i]) / fs * 1000
        if 400 < period < 1500:
            periods.append(period)
    period_mean = np.mean(periods) if periods else 0
    pulse_rate = 60000 / period_mean if period_mean > 0 else 0
    return {
        'ppg_amp_mean': amp_mean,
        'ppg_amp_std': amp_std,
        'ppg_amp_cv': amp_cv,
        'rise_time_mean': rise_time_mean,
        'pulse_period_mean': period_mean,
        'pulse_rate': pulse_rate,
        'peak_count': len(peaks),
    }


def calculate_risk_level(features, thresholds=None):
    """计算风险等级 (0-4) 返回等级和原因"""
    if not features:
        return 0, "数据不足"
        
    if thresholds is None:
        thresholds = THRESHOLDS
        
    ptt_info = features.get('ptt_info')
    hrv_features = features.get('hrv_features')
    ppg_features = features.get('ppg_features')
    spo2 = features.get('spo2', 100)
    pi = features.get('pi', 1.0)
    has_ptt = features.get('has_ptt', False)
    
    risk_score = 0
    reasons = []
    
    # 1. PTT指标（最重要）
    if has_ptt and ptt_info:
        ptt_mean = ptt_info['ptt_mean']
        ptt_cv = ptt_info.get('ptt_cv', 0)
        
        if ptt_mean < thresholds['ptt_low']:
            risk_score += 2
            reasons.append(f'PTT过短 ({ptt_mean:.1f}ms)')
        elif ptt_mean > thresholds['ptt_high']:
            risk_score += 2
            reasons.append(f'PTT过长 ({ptt_mean:.1f}ms)')
            
        if ptt_cv > 0.25:
            risk_score += 1
            reasons.append(f'PTT变异性高 (CV:{ptt_cv:.2f})')
    
    # 2. HRV指标
    if hrv_features:
        hr = hrv_features.get('heart_rate', 0)
        rmssd = hrv_features.get('rmssd', 0)
        pnn50 = hrv_features.get('pnn50', 0)
        
        if hr > thresholds['hr_high'] or hr < thresholds['hr_low']:
            risk_score += 1
            reasons.append(f'心率异常 ({hr:.1f}bpm)')
            
        if rmssd < thresholds['rmssd_low']:
            risk_score += 1
            reasons.append(f'HRV降低 (RMSSD:{rmssd:.1f}ms)')
            
        if pnn50 < 5:
            risk_score += 1
            reasons.append(f'pNN50降低 ({pnn50:.1f}%)')
    
    # 3. PPG特征
    if ppg_features:
        amp_cv = ppg_features.get('ppg_amp_cv', 0)
        rise_time = ppg_features.get('rise_time_mean', 0)
        pulse_rate = ppg_features.get('pulse_rate', 0)
        
        if hrv_features:
            hr = hrv_features.get('heart_rate', 0)
            if abs(pulse_rate - hr) > 5:
                risk_score += 1
                reasons.append(f'脉率-心率差 ({abs(pulse_rate - hr):.1f}bpm)')
                
        if amp_cv > thresholds['amp_cv_high']:
            risk_score += 1
            reasons.append(f'PPG幅值不稳定 (CV:{amp_cv:.1f}%)')
            
        if rise_time > thresholds['rise_time_high']:
            risk_score += 1
            reasons.append(f'上升时间延长 ({rise_time:.1f}ms)')
        elif 0 < rise_time < thresholds['rise_time_low']:
            risk_score += 1
            reasons.append(f'上升时间过短 ({rise_time:.1f}ms)')
    
    # 4. 其他指标
    if spo2 < thresholds['spo2_low']:
        risk_score += 2
        reasons.append(f'血氧降低 (SpO₂:{spo2:.1f}%)')
        
    if pi < thresholds['pi_low']:
        risk_score += 1
        reasons.append(f'灌注指数低 (PI:{pi:.2f}%)')
    
    # 转换为风险等级 (0-4)
    if not has_ptt or risk_score == 0:
        risk_level = 1  # 正常
    elif risk_score >= 4:
        risk_level = 4  # 重度
    elif risk_score >= 3:
        risk_level = 3  # 中度
    elif risk_score >= 2:
        risk_level = 2  # 轻度
    else:
        risk_level = 1  # 正常
    
    reason_str = '; '.join(reasons) if reasons else '正常'
    return risk_level, reason_str


def oh_judgment_model_ecg_ppg(features, thresholds=None):
    """OH判定（兼容原有接口）"""
    risk_level, reason = calculate_risk_level(features, thresholds)
    is_oh = risk_level >= 2  # 2级以上为OH阳性
    return is_oh, reason, risk_level


class SerialReceiverWithSend(QThread):
    """串口接收+发送线程（同一个串口）"""
    data_received = pyqtSignal(object)
    connection_status = pyqtSignal(bool)
    debug_message = pyqtSignal(str)
    send_status = pyqtSignal(str)

    def __init__(self, port=SERIAL_PORT, baudrate=BAUDRATE):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        self.running = False
        self.line_count = 0
        self.good_count = 0
        self.last_bad_log = 0.0
        
        # 发送相关
        self.send_queue = deque()
        self.lock = threading.Lock()
        self.last_sent_time = 0
        self.send_interval = 0.1  # 发送间隔，避免冲突

    def connect(self):
        try:
            self.serial = serial.Serial(
                port=self.port, 
                baudrate=self.baudrate, 
                timeout=TIMEOUT
            )
            self.running = True
            self.connection_status.emit(True)
            self.debug_message.emit(f'串口已连接: {self.port} @ {self.baudrate}')
            return True
        except Exception as e:
            self.debug_message.emit(f'串口连接失败: {e}')
            self.connection_status.emit(False)
            return False

    def disconnect(self):
        self.running = False
        if self.serial and self.serial.is_open:
            try:
                self.serial.close()
            except Exception:
                pass
        self.connection_status.emit(False)
        self.debug_message.emit('串口已断开')

    def parse_serial_line(self, line):
        try:
            # 检查是否是发送的指令（避免接收自己发送的数据）
            if line.startswith('RISK:'):
                return None
                
            if 'HR:' not in line or 'SPO2:' not in line or 'B:' not in line or 'A:' not in line:
                return None
                
            parts = line.split()

            def get_value(tag, cast=float, default=None):
                if tag in parts:
                    idx = parts.index(tag)
                    if idx + 1 < len(parts):
                        return cast(parts[idx + 1])
                return default

            hr = get_value('HR:', int, 0)
            spo2 = get_value('SPO2:', int, 0)
            ecg_value = get_value('A:', float, 0.0)
            ppg_value = get_value('B:', float, 0.0)
            bpm = get_value('C:', float, float(hr))
            
            return {
                'timestamp': time.time(),
                'hr': hr,
                'spo2': spo2,
                'ecg': ecg_value,
                'ppg_red': ppg_value,
                'ppg_ir': ppg_value,
                'bpm': bpm,
                'raw_line': line,
                'pi': 1.0,
            }
        except Exception as e:
            self.debug_message.emit(f'解析错误: {e} | {line[:120]}')
            return None

    def send_risk_level(self, risk_level):
        """发送风险等级到队列"""
        with self.lock:
            self.send_queue.append(risk_level)
            self.send_status.emit(f'加入发送队列: RISK:{risk_level}')

    def run(self):
        while self.running:
            try:
                if self.serial and self.serial.is_open:
                    current_time = time.time()
                    
                    # 1. 处理接收数据
                    if self.serial.in_waiting > 0:
                        raw = self.serial.readline()
                        if raw:
                            self.line_count += 1
                            line = raw.decode('utf-8', errors='ignore').strip()
                            if line and not line.startswith('RISK:'):  # 忽略自己发送的数据
                                data = self.parse_serial_line(line)
                                if data is not None:
                                    self.good_count += 1
                                    self.data_received.emit(data)
                                    if self.good_count <= 3 or self.good_count % 500 == 0:
                                        self.debug_message.emit(
                                            f"样本{self.good_count}: ECG={data['ecg']:.1f}, PPG={data['ppg_red']:.1f}, HR={data['hr']}, SPO2={data['spo2']}"
                                        )
                                else:
                                    if current_time - self.last_bad_log > 2.0:
                                        self.debug_message.emit(f'收到未识别行: {line[:120]}')
                                        self.last_bad_log = current_time
                    
                    # 2. 处理发送数据（限制发送频率）
                    if current_time - self.last_sent_time >= self.send_interval:
                        risk_level = None
                        with self.lock:
                            if self.send_queue:
                                risk_level = self.send_queue.popleft()
                        
                        if risk_level is not None:
                            # 发送格式: "RISK:X\n" 其中X为0-4的数字
                            message = f"RISK:{risk_level}\n"
                            self.serial.write(message.encode('utf-8'))
                            self.send_status.emit(f'已发送风险等级: {risk_level}')
                            self.last_sent_time = current_time
                    
                    # 短暂休眠避免CPU占用过高
                    time.sleep(0.01)
                    
            except Exception as e:
                self.debug_message.emit(f'串口操作错误: {e}')
                time.sleep(0.1)


class RealTimeProcessor(QObject):
    features_updated = pyqtSignal(dict)
    oh_alert = pyqtSignal(bool, str, int)
    risk_level_ready = pyqtSignal(int, str)  # 新增：风险等级信号

    def __init__(self, fs=FS):
        super().__init__()
        self.fs = fs
        self.max_buffer_size = fs * 30
        self.display_size = fs * 8
        self.ecg_buffer = deque(maxlen=self.max_buffer_size)
        self.ppg_red_buffer = deque(maxlen=self.max_buffer_size)
        self.ppg_ir_buffer = deque(maxlen=self.max_buffer_size)
        self.timestamps = deque(maxlen=self.max_buffer_size)
        self.hr_buffer = deque(maxlen=self.max_buffer_size)
        self.spo2_buffer = deque(maxlen=self.max_buffer_size)
        self.r_peaks = []
        self.ppg_peaks = []
        self.last_ptt_info = None
        self.last_hrv_features = None
        self.last_ppg_features = None
        self.last_oh_check_time = time.time()
        self.monitoring_enabled = False
        self.last_features = {
            'hrv_features': {'heart_rate': 0, 'rmssd': 0},
            'ptt_info': None,
            'ppg_features': {'pulse_rate': 0, 'rise_time_mean': 0, 'ppg_amp_cv': 0},
            'spo2': 0,
            'pi': 0,
            'has_ptt': False,
            'timestamp': time.time(),
        }
        self.sample_counter = 0
        self.last_feature_process_counter = 0
        self.serial_receiver = None  # 引用串口接收器（用于发送）
        self.last_sent_risk_level = -1  # 上次发送的风险等级

    def set_serial_receiver(self, receiver):
        """设置串口接收器引用（用于发送数据）"""
        self.serial_receiver = receiver

    @pyqtSlot(object)
    def on_data_received(self, data):
        self.ecg_buffer.append(float(data.get('ecg', 0.0)))
        self.ppg_red_buffer.append(float(data.get('ppg_red', 0.0)))
        self.ppg_ir_buffer.append(float(data.get('ppg_ir', data.get('ppg_red', 0.0))))
        self.timestamps.append(float(data.get('timestamp', time.time())))
        self.hr_buffer.append(float(data.get('hr', 0)))
        self.spo2_buffer.append(float(data.get('spo2', 0)))
        self.sample_counter += 1

        self.last_features = {
            'hrv_features': {'heart_rate': data.get('hr', 0), 'rmssd': 0},
            'ptt_info': self.last_ptt_info,
            'ppg_features': {'pulse_rate': data.get('bpm', data.get('hr', 0)), 'rise_time_mean': 0, 'ppg_amp_cv': 0},
            'spo2': data.get('spo2', 0),
            'pi': data.get('pi', 1.0),
            'has_ptt': self.last_ptt_info is not None,
            'timestamp': time.time(),
        }

        if self.monitoring_enabled and len(self.ecg_buffer) >= self.fs * 3:
            if self.sample_counter - self.last_feature_process_counter >= self.fs:
                self.process_features(data)
                self.last_feature_process_counter = self.sample_counter

    def get_display_data(self):
        n = min(len(self.ecg_buffer), self.display_size)
        if n <= 1:
            return None
        times = np.array(list(self.timestamps)[-n:], dtype=float)
        if len(times) <= 1:
            return None
        time_axis = times - times[-1]
        ecg = np.array(list(self.ecg_buffer)[-n:], dtype=float)
        ppg = np.array(list(self.ppg_red_buffer)[-n:], dtype=float)
        return time_axis, ecg, ppg

    def get_peak_spots(self):
        if len(self.timestamps) == 0:
            return [], []
        timestamps = list(self.timestamps)
        ecg_buffer = list(self.ecg_buffer)
        ppg_buffer = list(self.ppg_red_buffer)
        last_ts = timestamps[-1]
        start_index = max(0, len(timestamps) - self.display_size)

        ecg_spots = []
        for p in self.r_peaks[-10:]:
            if start_index <= p < len(timestamps):
                ecg_spots.append({'pos': (timestamps[p] - last_ts, ecg_buffer[p]), 'size': 10, 'brush': 'r'})

        ppg_spots = []
        for p in self.ppg_peaks[-10:]:
            if start_index <= p < len(timestamps):
                ppg_spots.append({'pos': (timestamps[p] - last_ts, ppg_buffer[p]), 'size': 8, 'brush': 'b'})

        return ecg_spots, ppg_spots

    def process_features(self, latest_data=None):
        if len(self.ecg_buffer) < self.fs * 3:
            return
        ecg_array = np.array(list(self.ecg_buffer)[-self.fs * 10:], dtype=float)
        ppg_array = np.array(list(self.ppg_red_buffer)[-self.fs * 10:], dtype=float)

        r_peaks, _, _ = detect_r_peaks_ecg(ecg_array, self.fs)
        if len(r_peaks) > 0:
            offset = len(self.ecg_buffer) - len(ecg_array)
            self.r_peaks = [offset + int(p) for p in r_peaks]

        ppg_peaks, _, _ = detect_ppg_peaks(ppg_array, self.fs)
        if len(ppg_peaks) > 0:
            offset = len(self.ppg_red_buffer) - len(ppg_array)
            self.ppg_peaks = [offset + int(p) for p in ppg_peaks]

        ptt_info, has_ptt = calculate_ptt_ecg_ppg(ecg_array, ppg_array, self.fs)
        if has_ptt:
            self.last_ptt_info = ptt_info

        hrv_features = calculate_hrv_features(r_peaks, self.fs)
        if hrv_features:
            self.last_hrv_features = hrv_features

        ppg_features = calculate_ppg_features(ppg_array, ppg_peaks, self.fs)
        if ppg_features:
            self.last_ppg_features = ppg_features

        spo2 = latest_data.get('spo2', np.mean(list(self.spo2_buffer)[-10:]) if self.spo2_buffer else 0) if latest_data else np.mean(list(self.spo2_buffer)[-10:]) if self.spo2_buffer else 0
        live_hr = latest_data.get('hr', 0) if latest_data else np.mean(list(self.hr_buffer)[-10:]) if self.hr_buffer else 0
        pi = max(np.std(ppg_array) / (abs(np.mean(ppg_array)) + 1e-6) * 100, 0)

        if self.last_hrv_features and live_hr > 0:
            self.last_hrv_features['heart_rate'] = live_hr
        if self.last_ppg_features and latest_data is not None:
            self.last_ppg_features['pulse_rate'] = latest_data.get('bpm', latest_data.get('hr', 0))

        features = {
            'ptt_info': self.last_ptt_info,
            'hrv_features': self.last_hrv_features,
            'ppg_features': self.last_ppg_features,
            'spo2': spo2,
            'pi': pi,
            'has_ptt': has_ptt,
            'timestamp': time.time(),
        }
        self.last_features = features
        self.features_updated.emit(features)

        current_time = time.time()
        if current_time - self.last_oh_check_time >= 3:
            # 计算风险等级
            risk_level, reason = calculate_risk_level(features)
            is_oh = risk_level >= 2
            
            # 发送风险等级信号
            self.risk_level_ready.emit(risk_level, reason)
            
            # 通过同一个串口发送风险等级（如果有变化）
            if self.serial_receiver and risk_level != self.last_sent_risk_level:
                self.serial_receiver.send_risk_level(risk_level)
                self.last_sent_risk_level = risk_level
            
            # 触发OH预警（兼容原有接口）
            self.oh_alert.emit(is_oh, reason, risk_level)
            
            self.last_oh_check_time = current_time


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.processor = None
        self.receiver = None  # 同一个串口，同时负责接收和发送
        self.patient_info = {}
        self.event_records = []
        self.last_view_update_ts = 0.0
        self.init_ui()
        self.init_database()
        self.init_timers()

    def init_ui(self):
        self.setWindowTitle('体位性低血压快速筛查系统 v2.3 同串口收发版 (文本串口兼容-PPG)')
        self.setGeometry(100, 100, 1450, 900)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)

        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)

        self.ecg_plot = pg.PlotWidget(title='ECG波形 (文本串口 A 通道)')
        self.ecg_plot.setLabel('left', '幅值', units='AU')
        self.ecg_plot.setLabel('bottom', '时间', units='s')
        self.ecg_plot.showGrid(x=True, y=True)
        self.ecg_plot.setClipToView(True)
        self.ecg_plot.setDownsampling(mode='peak')
        self.ecg_curve = self.ecg_plot.plot(pen='y')
        self.ecg_r_peaks = pg.ScatterPlotItem(size=10, brush='r')
        self.ecg_plot.addItem(self.ecg_r_peaks)
        left_layout.addWidget(self.ecg_plot)

        self.ppg_plot = pg.PlotWidget(title='PPG波形 (文本串口 B 通道)')
        self.ppg_plot.setLabel('left', '幅值', units='AU')
        self.ppg_plot.setLabel('bottom', '时间', units='s')
        self.ppg_plot.showGrid(x=True, y=True)
        self.ppg_plot.setClipToView(True)
        self.ppg_plot.setDownsampling(mode='peak')
        self.ppg_curve = self.ppg_plot.plot(pen='c')
        self.ppg_peaks = pg.ScatterPlotItem(size=8, brush='b')
        self.ppg_plot.addItem(self.ppg_peaks)
        left_layout.addWidget(self.ppg_plot)

        # 控制面板
        control_layout = QHBoxLayout()
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(220)
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(['115200', '921600'])
        self.baud_combo.setCurrentText(str(BAUDRATE))
        self.btn_refresh_ports = QPushButton('刷新串口')
        self.btn_refresh_ports.clicked.connect(self.refresh_serial_ports)
        self.btn_connect = QPushButton('连接设备')
        self.btn_connect.clicked.connect(self.toggle_connection)
        self.btn_start = QPushButton('开始监测')
        self.btn_start.clicked.connect(self.start_monitoring)
        self.btn_start.setEnabled(False)
        self.btn_save = QPushButton('保存波形')
        self.btn_save.clicked.connect(self.save_waveform)
        self.btn_export = QPushButton('导出报告')
        self.btn_export.clicked.connect(self.export_report)
        
        control_layout.addWidget(QLabel('串口:'))
        control_layout.addWidget(self.port_combo)
        control_layout.addWidget(QLabel('波特率:'))
        control_layout.addWidget(self.baud_combo)
        control_layout.addWidget(self.btn_refresh_ports)
        control_layout.addWidget(self.btn_connect)
        control_layout.addWidget(self.btn_start)
        control_layout.addWidget(self.btn_save)
        control_layout.addWidget(self.btn_export)
        control_layout.addStretch()
        
        self.status_label = QLabel('状态: 未连接')
        control_layout.addWidget(self.status_label)
        left_layout.addLayout(control_layout)

        # 右侧面板
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        right_layout.setContentsMargins(10, 10, 10, 10)

        # 患者信息
        patient_group = QGroupBox('患者信息')
        patient_layout = QFormLayout()
        self.patient_id_edit = QLineEdit()
        self.patient_name_edit = QLineEdit()
        self.patient_age_edit = QLineEdit()
        self.patient_gender_combo = QComboBox()
        self.patient_gender_combo.addItems(['男', '女'])
        patient_layout.addRow('ID:', self.patient_id_edit)
        patient_layout.addRow('姓名:', self.patient_name_edit)
        patient_layout.addRow('年龄:', self.patient_age_edit)
        patient_layout.addRow('性别:', self.patient_gender_combo)
        btn_save_patient = QPushButton('保存患者信息')
        btn_save_patient.clicked.connect(self.save_patient_info)
        patient_layout.addRow(btn_save_patient)
        patient_group.setLayout(patient_layout)
        right_layout.addWidget(patient_group)

        # 实时参数
        param_group = QGroupBox('实时生理参数')
        param_layout = QGridLayout()
        self.param_labels = {}
        params = [
            ('心率:', 'hr_value', '0 bpm', 0, 0),
            ('PTT:', 'ptt_value', '0 ms', 0, 1),
            ('SpO₂:', 'spo2_value', '0%', 1, 0),
            ('脉率:', 'pr_value', '0 bpm', 1, 1),
            ('RMSSD:', 'rmssd_value', '0 ms', 2, 0),
            ('灌注指数:', 'pi_value', '0%', 2, 1),
            ('上升时间:', 'rt_value', '0 ms', 3, 0),
            ('幅值CV:', 'ampcv_value', '0%', 3, 1),
        ]
        for text, key, default, row, col in params:
            label = QLabel(text)
            value_label = QLabel(default)
            value_label.setStyleSheet('font-weight: bold; color: blue;')
            param_layout.addWidget(label, row, col * 2)
            param_layout.addWidget(value_label, row, col * 2 + 1)
            self.param_labels[key] = value_label
            
        # 风险等级显示
        risk_label = QLabel('风险等级:')
        self.risk_level_label = QLabel('1 - 正常')
        self.risk_level_label.setStyleSheet('font-weight: bold; color: green; font-size: 14px;')
        param_layout.addWidget(risk_label, 4, 0)
        param_layout.addWidget(self.risk_level_label, 4, 1, 1, 3)
        
        sqi_label = QLabel('信号质量:')
        self.sqi_bar = QProgressBar()
        self.sqi_bar.setRange(0, 100)
        self.sqi_bar.setValue(0)
        param_layout.addWidget(sqi_label, 5, 0)
        param_layout.addWidget(self.sqi_bar, 5, 1, 1, 3)
        
        param_group.setLayout(param_layout)
        right_layout.addWidget(param_group)

        # 预警提示
        alert_group = QGroupBox('预警提示')
        alert_layout = QVBoxLayout()
        self.alert_text = QTextEdit()
        self.alert_text.setReadOnly(True)
        self.alert_text.setMaximumHeight(60)
        self.alert_text.setStyleSheet('background-color: #ffeeee;')
        alert_layout.addWidget(self.alert_text)
        alert_group.setLayout(alert_layout)
        right_layout.addWidget(alert_group)

        # 串口发送状态
        send_status_group = QGroupBox('串口发送状态')
        send_status_layout = QVBoxLayout()
        self.send_status_text = QTextEdit()
        self.send_status_text.setReadOnly(True)
        self.send_status_text.setMaximumHeight(60)
        send_status_layout.addWidget(self.send_status_text)
        send_status_group.setLayout(send_status_layout)
        right_layout.addWidget(send_status_group)

        # 调试窗口
        debug_group = QGroupBox('串口/解析调试')
        debug_layout = QVBoxLayout()
        self.debug_text = QTextEdit()
        self.debug_text.setReadOnly(True)
        self.debug_text.setMaximumHeight(150)
        debug_layout.addWidget(self.debug_text)
        debug_group.setLayout(debug_layout)
        right_layout.addWidget(debug_group)

        # OH事件记录
        event_group = QGroupBox('OH事件记录')
        event_layout = QVBoxLayout()
        self.event_table = QTableWidget()
        self.event_table.setColumnCount(4)
        self.event_table.setHorizontalHeaderLabels(['时间', '风险等级', '原因', '状态'])
        self.event_table.horizontalHeader().setStretchLastSection(True)
        event_layout.addWidget(self.event_table)
        btn_clear_events = QPushButton('清除记录')
        btn_clear_events.clicked.connect(self.clear_events)
        event_layout.addWidget(btn_clear_events)
        event_group.setLayout(event_layout)
        right_layout.addWidget(event_group)

        # 分割器
        splitter = QSplitter(Qt.Horizontal)
        splitter.addWidget(left_widget)
        splitter.addWidget(right_widget)
        splitter.setSizes([930, 520])
        main_layout.addWidget(splitter)

        self.refresh_serial_ports()
        self.statusBar().showMessage('就绪')

    def init_timers(self):
        self.gui_timer = QTimer(self)
        self.gui_timer.timeout.connect(self.refresh_plots)
        self.gui_timer.start(GUI_REFRESH_MS)

        self.param_timer = QTimer(self)
        self.param_timer.timeout.connect(self.refresh_parameters)
        self.param_timer.start(PARAM_REFRESH_MS)

    def _set_full_view(self, plot_widget, time_axis, data, min_span=1.0):
        if len(time_axis) == 0 or len(data) == 0:
            return
        x_min = float(np.min(time_axis))
        x_max = float(np.max(time_axis))
        if x_min == x_max:
            x_min -= 1.0
            x_max += 0.1
        y_min = float(np.min(data))
        y_max = float(np.max(data))
        if y_min == y_max:
            pad = max(abs(y_min) * 0.1, min_span)
        else:
            pad = max((y_max - y_min) * 0.12, min_span)
        plot_widget.setXRange(x_min, x_max, padding=0)
        plot_widget.setYRange(y_min - pad, y_max + pad, padding=0)

    def append_debug(self, msg):
        now = datetime.now().strftime('%H:%M:%S')
        self.debug_text.append(f'[{now}] {msg}')
        doc = self.debug_text.document()
        while doc.blockCount() > MAX_DEBUG_LINES:
            cursor = self.debug_text.textCursor()
            cursor.movePosition(cursor.Start)
            cursor.select(cursor.BlockUnderCursor)
            cursor.removeSelectedText()
            cursor.deleteChar()
            
    def append_send_status(self, msg):
        """添加发送状态信息"""
        now = datetime.now().strftime('%H:%M:%S')
        self.send_status_text.append(f'[{now}] {msg}')
        # 滚动到底部
        scrollbar = self.send_status_text.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())

    def refresh_serial_ports(self):
        current_port = self.port_combo.currentData() if hasattr(self, 'port_combo') else None
        self.port_combo.clear()
        try:
            ports = list(serial.tools.list_ports.comports())
        except Exception as e:
            self.port_combo.addItem('串口读取失败', '')
            self.btn_connect.setEnabled(False)
            self.statusBar().showMessage(f'串口读取失败: {e}')
            return
        if not ports:
            self.port_combo.addItem('未发现串口', '')
            self.btn_connect.setEnabled(False)
            self.statusBar().showMessage('未发现可用串口')
            return
        restore_index = 0
        for i, port in enumerate(ports):
            display_name = f'{port.device} - {port.description}'
            self.port_combo.addItem(display_name, port.device)
            if current_port and port.device == current_port:
                restore_index = i
        self.port_combo.setCurrentIndex(restore_index)
        self.btn_connect.setEnabled(True)
        self.statusBar().showMessage(f'发现 {len(ports)} 个可用串口')

    def init_database(self):
        try:
            self.conn = sqlite3.connect('oh_screening.db')
            cursor = self.conn.cursor()
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS patients (
                    id TEXT PRIMARY KEY,
                    name TEXT,
                    age INTEGER,
                    gender TEXT,
                    create_time TIMESTAMP
                )
            """)
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS events (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    patient_id TEXT,
                    event_time TIMESTAMP,
                    risk_score INTEGER,
                    risk_level INTEGER,
                    reason TEXT,
                    ptt_mean REAL,
                    heart_rate REAL,
                    spo2 REAL,
                    waveform_file TEXT,
                    FOREIGN KEY (patient_id) REFERENCES patients(id)
                )
            """)
            self.conn.commit()
        except Exception as e:
            print(f'数据库初始化错误: {e}')

    def toggle_connection(self):
        if self.receiver and self.receiver.isRunning():
            self.receiver.disconnect()
            self.receiver.quit()
            self.receiver.wait(1000)
            self.btn_connect.setText('连接设备')
            self.btn_start.setEnabled(False)
            self.port_combo.setEnabled(True)
            self.baud_combo.setEnabled(True)
            self.btn_refresh_ports.setEnabled(True)
            self.status_label.setText('状态: 未连接')
            self.statusBar().showMessage('设备已断开')
            return

        self.refresh_serial_ports()
        selected_port = self.port_combo.currentData()
        if not selected_port:
            QMessageBox.warning(self, '提示', '请先选择可用串口')
            return

        baud = int(self.baud_combo.currentText())
        
        # 使用同一个串口类（同时负责接收和发送）
        self.receiver = SerialReceiverWithSend(port=selected_port, baudrate=baud)
        self.processor = RealTimeProcessor()
        
        # 将串口接收器引用设置给处理器（用于发送数据）
        self.processor.set_serial_receiver(self.receiver)
        
        # 连接信号
        self.receiver.data_received.connect(self.processor.on_data_received)
        self.receiver.connection_status.connect(self.on_connection_status)
        self.receiver.debug_message.connect(self.append_debug)
        self.receiver.send_status.connect(self.append_send_status)  # 发送状态
        
        self.processor.features_updated.connect(self.update_parameters)
        self.processor.oh_alert.connect(self.on_oh_alert)
        self.processor.risk_level_ready.connect(self.on_risk_level_ready)

        if self.receiver.connect():
            self.receiver.start()
            self.btn_connect.setText('断开设备')
            self.btn_start.setEnabled(True)
            self.port_combo.setEnabled(False)
            self.baud_combo.setEnabled(False)
            self.btn_refresh_ports.setEnabled(False)
            self.status_label.setText(f'状态: 已连接 {selected_port} (收发同口)')
            self.statusBar().showMessage(f'设备连接成功: {selected_port} @ {baud}')
        else:
            QMessageBox.warning(self, '连接失败', f'无法连接到串口设备: {selected_port}')

    def on_connection_status(self, connected):
        if not connected:
            self.btn_connect.setText('连接设备')
            self.btn_start.setEnabled(False)
            self.port_combo.setEnabled(True)
            self.baud_combo.setEnabled(True)
            self.btn_refresh_ports.setEnabled(True)
            self.status_label.setText('状态: 未连接')

    def start_monitoring(self):
        if not self.processor:
            QMessageBox.warning(self, '提示', '请先连接设备')
            return
        if not self.patient_info:
            reply = QMessageBox.question(self, '患者信息', '尚未保存患者信息，是否继续？', QMessageBox.Yes | QMessageBox.No)
            if reply == QMessageBox.No:
                return
        self.processor.monitoring_enabled = True
        self.btn_start.setText('监测中...')
        self.btn_start.setEnabled(False)
        self.statusBar().showMessage('正在监测...')
        self.append_debug('已开启特征分析与OH判定')

    def refresh_plots(self):
        if not self.processor:
            return
        display = self.processor.get_display_data()
        if display is None:
            return
        time_axis, ecg, ppg = display
        self.ecg_curve.setData(time_axis, ecg)
        self.ppg_curve.setData(time_axis, ppg)

        ecg_spots, ppg_spots = self.processor.get_peak_spots()
        self.ecg_r_peaks.setData(ecg_spots)
        self.ppg_peaks.setData(ppg_spots)

        now = time.time()
        if now - self.last_view_update_ts >= VIEW_REFRESH_MS / 1000.0:
            self._set_full_view(self.ecg_plot, time_axis, ecg)
            self._set_full_view(self.ppg_plot, time_axis, ppg)
            self.last_view_update_ts = now

    def refresh_parameters(self):
        if self.processor:
            self.update_parameters(self.processor.last_features)

    def update_parameters(self, features):
        if features.get('hrv_features'):
            hrv = features['hrv_features']
            self.param_labels['hr_value'].setText(f"{hrv.get('heart_rate', 0):.0f} bpm")
            self.param_labels['rmssd_value'].setText(f"{hrv.get('rmssd', 0):.1f} ms")
        if features.get('ptt_info'):
            ptt = features['ptt_info']
            self.param_labels['ptt_value'].setText(f"{ptt.get('ptt_mean', 0):.0f} ms")
        else:
            self.param_labels['ptt_value'].setText('0 ms')
        if features.get('ppg_features'):
            ppg = features['ppg_features']
            self.param_labels['pr_value'].setText(f"{ppg.get('pulse_rate', 0):.0f} bpm")
            self.param_labels['rt_value'].setText(f"{ppg.get('rise_time_mean', 0):.0f} ms")
            self.param_labels['ampcv_value'].setText(f"{ppg.get('ppg_amp_cv', 0):.1f}%")
        else:
            self.param_labels['pr_value'].setText('0 bpm')
            self.param_labels['rt_value'].setText('0 ms')
            self.param_labels['ampcv_value'].setText('0%')
        self.param_labels['spo2_value'].setText(f"{features.get('spo2', 0):.0f}%")
        self.param_labels['pi_value'].setText(f"{features.get('pi', 0):.2f}%")

        sqi = 0
        if self.processor:
            if len(self.processor.ecg_buffer) > self.processor.fs:
                sqi += 40
            if len(self.processor.ppg_red_buffer) > self.processor.fs:
                sqi += 30
            if features.get('has_ptt'):
                sqi += 30
        self.sqi_bar.setValue(min(max(int(sqi), 0), 100))
        
    def on_risk_level_ready(self, risk_level, reason):
        """处理风险等级就绪信号"""
        risk_names = {
            0: '数据不足',
            1: '正常',
            2: '轻度风险',
            3: '中度风险',
            4: '重度风险'
        }
        
        # 更新风险等级显示
        color_map = {
            0: 'gray',
            1: 'green',
            2: 'orange',
            3: 'red',
            4: 'darkred'
        }
        
        risk_name = risk_names.get(risk_level, f'等级{risk_level}')
        self.risk_level_label.setText(f'{risk_level} - {risk_name}')
        self.risk_level_label.setStyleSheet(f'font-weight: bold; color: {color_map.get(risk_level, "black")}; font-size: 14px;')
        
        # 添加到事件记录（仅当风险等级>=2时记录）
        if risk_level >= 2:
            row = self.event_table.rowCount()
            self.event_table.insertRow(row)
            self.event_table.setItem(row, 0, QTableWidgetItem(datetime.now().strftime('%H:%M:%S')))
            self.event_table.setItem(row, 1, QTableWidgetItem(str(risk_level)))
            self.event_table.setItem(row, 2, QTableWidgetItem(reason[:40]))
            self.event_table.setItem(row, 3, QTableWidgetItem('未处理'))
            
            # 保存到数据库
            if self.patient_info:
                try:
                    cursor = self.conn.cursor()
                    cursor.execute(
                        'INSERT INTO events (patient_id, event_time, risk_level, reason) VALUES (?, ?, ?, ?)',
                        (self.patient_info.get('id', ''), datetime.now(), risk_level, reason)
                    )
                    self.conn.commit()
                except Exception as e:
                    self.append_debug(f'事件保存错误: {e}')

    def on_oh_alert(self, is_oh, reason, score):
        if is_oh:
            self.alert_text.setStyleSheet('background-color: #ffcccc;')
            self.alert_text.setText(f'⚠️ OH预警 (评分:{score}): {reason}')
            QApplication.beep()
        else:
            self.alert_text.setStyleSheet('background-color: #eeffee;')
            self.alert_text.setText(f'正常: {reason}')

    def save_patient_info(self):
        self.patient_info = {
            'id': self.patient_id_edit.text(),
            'name': self.patient_name_edit.text(),
            'age': self.patient_age_edit.text(),
            'gender': self.patient_gender_combo.currentText(),
        }
        if self.patient_info['id']:
            try:
                cursor = self.conn.cursor()
                cursor.execute(
                    'INSERT OR REPLACE INTO patients (id, name, age, gender, create_time) VALUES (?, ?, ?, ?, ?)',
                    (
                        self.patient_info['id'],
                        self.patient_info['name'],
                        int(self.patient_info['age']) if self.patient_info['age'] else None,
                        self.patient_info['gender'],
                        datetime.now()
                    )
                )
                self.conn.commit()
                QMessageBox.information(self, '成功', '患者信息已保存')
            except Exception as e:
                QMessageBox.warning(self, '错误', f'保存失败: {e}')
        else:
            QMessageBox.warning(self, '提示', '请输入患者ID')

    def save_waveform(self):
        if not self.processor or len(self.processor.ecg_buffer) < max(20, self.processor.fs // 2):
            QMessageBox.warning(self, '提示', '数据不足')
            return
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        filename = f'waveform_{timestamp}.csv'
        data_len = min(len(self.processor.ecg_buffer), self.processor.fs * 10)
        df = pd.DataFrame({
            'timestamp': list(self.processor.timestamps)[-data_len:],
            'ecg': list(self.processor.ecg_buffer)[-data_len:],
            'ppg': list(self.processor.ppg_red_buffer)[-data_len:],
            'hr': list(self.processor.hr_buffer)[-data_len:],
            'spo2': list(self.processor.spo2_buffer)[-data_len:],
        })
        df.to_csv(filename, index=False)
        self.statusBar().showMessage(f'波形已保存至 {filename}')
        self.append_debug(f'已保存波形: {filename}')

    def export_report(self):
        filename, _ = QFileDialog.getSaveFileName(self, '保存报告', '', 'CSV文件 (*.csv)')
        if filename:
            df = pd.DataFrame(self.event_records)
            if df.empty:
                df = pd.DataFrame(columns=['time', 'risk_level', 'reason', 'status'])
            df.to_csv(filename, index=False)
            QMessageBox.information(self, '成功', f'报告已导出至 {filename}')

    def clear_events(self):
        self.event_table.setRowCount(0)
        self.event_records = []

    def closeEvent(self, event):
        try:
            if self.gui_timer.isActive():
                self.gui_timer.stop()
            if self.param_timer.isActive():
                self.param_timer.stop()
        except Exception:
            pass
        if self.receiver and self.receiver.isRunning():
            self.receiver.disconnect()
            self.receiver.quit()
            self.receiver.wait(1000)
        if hasattr(self, 'conn'):
            self.conn.close()
        event.accept()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())