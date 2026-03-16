"""
版本二：ECG（提R波）+ BP（提脉搏峰）→ 算PTT/波幅等 → 特征输入模型 → 判OH
功能：已有数据集文件验证算法准确性
使用数据：ecg, abp, marker（标注数据）
OH判定结果：与标注对比的准确率
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.signal import find_peaks, butter, filtfilt
from scipy.stats import describe
import wfdb
import os
import warnings
from sklearn.metrics import accuracy_score, precision_score, recall_score, f1_score, confusion_matrix
import seaborn as sns

warnings.filterwarnings('ignore')

# -------------------------- 配置参数 --------------------------
DATA_FOLDER = r"C:\Users\Administrator\Desktop\Sit-to-stand"
SAVE_RESULT_DIR = r"C:\Users\Administrator\Desktop\Sit-to-stand\analyse"
FS = 100  # 采样频率，根据实际数据集调整


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
    r_peaks, properties = find_peaks(
        ecg_integrated,
        distance=min_distance,
        height=threshold,
        prominence=threshold * 0.5
    )

    return r_peaks, ecg_filtered, ecg_integrated


def detect_bp_peaks(abp_signal, fs):
    """检测血压信号的脉搏波峰值"""
    # 带通滤波 (0.5-8 Hz)
    abp_filtered = bandpass_filter(abp_signal, 0.5, 8, fs)

    # 寻找收缩压峰值
    min_distance = int(0.4 * fs)
    height_threshold = np.percentile(abp_filtered, 70)

    # 寻找主峰（收缩压）
    systolic_peaks, _ = find_peaks(
        abp_filtered,
        distance=min_distance,
        height=height_threshold
    )

    # 寻找次峰（重搏波）
    if len(systolic_peaks) > 2:
        diastolic_peaks = []
        for i in range(len(systolic_peaks) - 1):
            start = systolic_peaks[i]
            end = systolic_peaks[i + 1]
            segment = abp_filtered[start:end]
            if len(segment) > 10:
                # 寻找重搏波（第二个小峰）
                local_peaks, _ = find_peaks(segment, distance=int(0.1 * fs))
                if len(local_peaks) > 1:
                    diastolic_peaks.append(start + local_peaks[1])

        return systolic_peaks, diastolic_peaks, abp_filtered

    return systolic_peaks, [], abp_filtered


def calculate_ptt_ecg_bp(ecg_signal, abp_signal, fs):
    """计算ECG R波到血压脉搏波的PTT"""
    # 检测R波
    r_peaks, ecg_filtered, _ = detect_r_peaks_ecg(ecg_signal, fs)

    # 检测血压波
    bp_peaks, _, abp_filtered = detect_bp_peaks(abp_signal, fs)

    if len(r_peaks) < 3 or len(bp_peaks) < 3:
        return None, False

    # 计算PTT：R波到下一个血压波的时间差
    ptt_values = []
    valid_pairs = []

    for r_peak in r_peaks[:50]:  # 限制数量以提高效率
        # 找到该R波之后的所有血压波
        subsequent_bp = bp_peaks[bp_peaks > r_peak]
        if len(subsequent_bp) > 0:
            ptt_ms = (subsequent_bp[0] - r_peak) / fs * 1000

            # 有效性检查
            if 50 < ptt_ms < 500:  # 合理范围
                ptt_values.append(ptt_ms)
                valid_pairs.append((r_peak, subsequent_bp[0]))

    if len(ptt_values) >= 5:
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


def calculate_bp_features(abp_signal, peaks, fs):
    """计算血压波形特征"""
    if len(peaks) < 3:
        return None

    amplitudes = abp_signal[peaks]
    systolic_mean = np.mean(amplitudes)
    systolic_std = np.std(amplitudes)

    # 计算脉压（需要找到波谷）
    pulse_pressures = []
    for i in range(len(peaks) - 1):
        peak1 = peaks[i]
        peak2 = peaks[i + 1]
        segment = abp_signal[peak1:peak2]
        if len(segment) > 0:
            diastolic = np.min(segment)
            pulse_pressure = abp_signal[peak1] - diastolic
            pulse_pressures.append(pulse_pressure)

    pp_mean = np.mean(pulse_pressures) if pulse_pressures else 0

    # 计算上升时间和下降时间
    rise_times = []
    for peak in peaks:
        # 寻找波谷
        search_window = int(0.3 * fs)
        start_idx = max(0, peak - search_window)
        segment = abp_signal[start_idx:peak]
        if len(segment) > 0:
            valley_idx = start_idx + np.argmin(segment)
            rise_time = (peak - valley_idx) / fs * 1000
            if 20 < rise_time < 300:
                rise_times.append(rise_time)

    rise_time_mean = np.mean(rise_times) if rise_times else 0

    return {
        "systolic_mean": systolic_mean,
        "systolic_std": systolic_std,
        "pulse_pressure_mean": pp_mean,
        "rise_time_mean": rise_time_mean,
        "peak_count": len(peaks)
    }


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
        pnn50 = nn50 / (len(rr_intervals) - 1) * 100
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


# -------------------------- OH判定模型（版本二） --------------------------
def oh_judgment_model_v2(features, abp_features=None):
    """
    版本二OH判定模型：基于ECG+BP特征
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

    # 3. 血压特征（如果有）
    if abp_features:
        systolic_mean = abp_features.get("systolic_mean", 0)
        pulse_pressure = abp_features.get("pulse_pressure_mean", 0)

        if systolic_mean < 90:  # 低血压
            risk_score += 2
            reasons.append(f"收缩压低 ({systolic_mean:.1f}mmHg)")

        if pulse_pressure < 25:  # 脉压过小
            risk_score += 1
            reasons.append(f"脉压小 ({pulse_pressure:.1f}mmHg)")

    # 判定规则
    if risk_score >= 3:
        return True, "; ".join(reasons)
    elif risk_score >= 2 and has_ptt:
        return True, f"中度风险: {'; '.join(reasons)}"
    else:
        return False, "正常" if risk_score == 0 else f"低风险: {risk_score}分"


# -------------------------- 数据集验证 --------------------------
class DatasetValidator:
    def __init__(self, data_folder, save_dir, fs=100):
        self.data_folder = data_folder
        self.save_dir = save_dir
        self.fs = fs

        # 创建保存目录
        os.makedirs(save_dir, exist_ok=True)

        # 结果存储
        self.all_results = []
        self.performance_metrics = {}

    def load_record(self, record_name):
        """加载WFDB记录"""
        try:
            record_path = os.path.join(self.data_folder, record_name)
            record = wfdb.rdrecord(record_path)

            # 获取信号索引
            sig_names = [name.lower() for name in record.sig_name]
            sig_data = record.p_signal

            # 提取需要的信号
            signals = {}

            # ECG信号
            if 'ecg' in sig_names:
                ecg_idx = sig_names.index('ecg')
                signals['ecg'] = sig_data[:, ecg_idx]

            # ABP信号
            if 'abp' in sig_names:
                abp_idx = sig_names.index('abp')
                signals['abp'] = sig_data[:, abp_idx]

            # Marker信号（如果有）
            if 'marker' in sig_names:
                marker_idx = sig_names.index('marker')
                signals['marker'] = sig_data[:, marker_idx]

            return signals, True

        except Exception as e:
            print(f"❌ 加载记录失败 {record_name}: {e}")
            return None, False

    def extract_features(self, signals):
        """从信号中提取特征"""
        features = {}

        if 'ecg' not in signals or 'abp' not in signals:
            return None

        ecg_signal = signals['ecg']
        abp_signal = signals['abp']

        # 1. 计算PTT
        ptt_info, has_ptt = calculate_ptt_ecg_bp(ecg_signal, abp_signal, self.fs)
        features["ptt_info"] = ptt_info
        features["has_ptt"] = has_ptt

        # 2. 提取ECG特征（HRV）
        r_peaks, _, _ = detect_r_peaks_ecg(ecg_signal, self.fs)
        hrv_features = calculate_hrv_features(r_peaks, self.fs)
        features["hrv_features"] = hrv_features

        # 3. 提取血压特征
        bp_peaks, _, _ = detect_bp_peaks(abp_signal, self.fs)
        bp_features = calculate_bp_features(abp_signal, bp_peaks, self.fs)
        features["bp_features"] = bp_features

        return features

    def get_ground_truth(self, signals, record_name):
        """
        获取真实标签
        这里需要根据实际标注方式调整
        假设marker信号中1表示OH事件
        """
        if 'marker' in signals:
            marker = signals['marker']
            # 简单阈值检测标注
            marker_peaks, _ = find_peaks(marker, height=0.5, distance=self.fs * 10)
            has_oh_event = len(marker_peaks) > 0

            # 或者可以计算OH事件的比例
            oh_ratio = np.mean(marker > 0.5)
            has_oh_event = oh_ratio > 0.01  # 1%的标注点

            return has_oh_event, oh_ratio
        else:
            # 如果没有标注，根据文件名或规则判断
            # 这里需要根据实际数据集的命名规则调整
            if "OH" in record_name.upper() or "HYPOTENSION" in record_name.upper():
                return True, 1.0
            else:
                return False, 0.0

    def validate_record(self, record_name):
        """验证单个记录"""
        print(f"🔍 分析: {record_name}", end="")

        # 加载数据
        signals, success = self.load_record(record_name)
        if not success:
            return None

        # 提取特征
        features = self.extract_features(signals)
        if not features:
            print(" - ❌ 特征提取失败")
            return None

        # OH判定
        is_oh_positive, reason = oh_judgment_model_v2(
            features,
            features.get("bp_features")
        )

        # 获取真实标签
        true_oh_status, oh_confidence = self.get_ground_truth(signals, record_name)

        # 保存结果
        result = {
            "record_name": record_name,
            "predicted_oh": is_oh_positive,
            "true_oh": true_oh_status,
            "oh_reason": reason,
            "oh_confidence": oh_confidence,
            "ptt_mean": features.get("ptt_info", {}).get("ptt_mean", 0) if features.get("has_ptt") else 0,
            "has_ptt": features.get("has_ptt", False),
            "heart_rate": features.get("hrv_features", {}).get("heart_rate", 0),
            "systolic_mean": features.get("bp_features", {}).get("systolic_mean", 0)
        }

        status = "✅" if is_oh_positive == true_oh_status else "❌"
        print(f" - {status} 预测:{is_oh_positive}, 真实:{true_oh_status}")

        return result

    def validate_all_records(self):
        """验证所有记录"""
        print("=" * 80)
        print("🔬 版本二：ECG+BP OH判定算法验证")
        print(f"📂 数据路径: {self.data_folder}")
        print("=" * 80)

        # 获取所有dat文件
        file_list = sorted([f for f in os.listdir(self.data_folder) if f.endswith('.dat')])

        print(f"📌 共找到 {len(file_list)} 个测试文件")

        for dat_file in file_list:
            record_name = dat_file[:-4]  # 移除.dat后缀
            result = self.validate_record(record_name)
            if result:
                self.all_results.append(result)

        # 计算性能指标
        self.calculate_metrics()

        # 保存结果
        self.save_results()

        # 可视化
        self.visualize_results()

        return self.all_results

    def calculate_metrics(self):
        """计算性能指标"""
        if not self.all_results:
            return

        df = pd.DataFrame(self.all_results)

        # 基本统计
        y_true = df['true_oh'].astype(int).values
        y_pred = df['predicted_oh'].astype(int).values

        # 性能指标
        accuracy = accuracy_score(y_true, y_pred)
        precision = precision_score(y_true, y_pred, zero_division=0)
        recall = recall_score(y_true, y_pred, zero_division=0)
        f1 = f1_score(y_true, y_pred, zero_division=0)

        # 混淆矩阵
        cm = confusion_matrix(y_true, y_pred)

        self.performance_metrics = {
            "accuracy": accuracy,
            "precision": precision,
            "recall": recall,
            "f1_score": f1,
            "confusion_matrix": cm,
            "total_samples": len(df),
            "oh_positive_rate": np.mean(y_true),
            "predicted_oh_rate": np.mean(y_pred)
        }

    def save_results(self):
        """保存结果到文件"""
        if not self.all_results:
            print("⚠️ 无结果可保存")
            return

        df = pd.DataFrame(self.all_results)

        # 保存详细结果
        detail_path = os.path.join(self.save_dir, "oh_validation_v2_details.csv")
        df.to_csv(detail_path, index=False, encoding='utf-8-sig')

        # 保存性能指标
        metrics_path = os.path.join(self.save_dir, "oh_validation_v2_metrics.txt")
        with open(metrics_path, 'w', encoding='utf-8') as f:
            f.write("=" * 60 + "\n")
            f.write("ECG+BP OH判定算法验证结果\n")
            f.write("=" * 60 + "\n\n")

            f.write(f"总样本数: {self.performance_metrics.get('total_samples', 0)}\n")
            f.write(f"真实OH阳性率: {self.performance_metrics.get('oh_positive_rate', 0):.2%}\n")
            f.write(f"预测OH阳性率: {self.performance_metrics.get('predicted_oh_rate', 0):.2%}\n\n")

            f.write("性能指标:\n")
            f.write(f"  准确率: {self.performance_metrics.get('accuracy', 0):.4f}\n")
            f.write(f"  精确率: {self.performance_metrics.get('precision', 0):.4f}\n")
            f.write(f"  召回率: {self.performance_metrics.get('recall', 0):.4f}\n")

        print(f"✅ 详细结果已保存至: {detail_path}")
        print(f"✅ 性能指标已保存至: {metrics_path}")

    def visualize_results(self):
        """可视化验证结果"""
        if not self.all_results:
            return

        df = pd.DataFrame(self.all_results)

        fig, axes = plt.subplots(2, 3, figsize=(15, 10))

        # 1. 混淆矩阵
        cm = self.performance_metrics.get("confusion_matrix")
        if cm is not None:
            sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', ax=axes[0, 0])
            axes[0, 0].set_title('混淆矩阵')
            axes[0, 0].set_xlabel('预测标签')
            axes[0, 0].set_ylabel('真实标签')
            axes[0, 0].set_xticklabels(['阴性', '阳性'])
            axes[0, 0].set_yticklabels(['阴性', '阳性'])

        # 2. 性能指标柱状图
        metrics = ['准确率', '精确率', '召回率', 'F1分数']
        values = [
            self.performance_metrics.get('accuracy', 0),
            self.performance_metrics.get('precision', 0),
            self.performance_metrics.get('recall', 0),
            self.performance_metrics.get('f1_score', 0)
        ]
        axes[0, 1].bar(metrics, values, color=['skyblue', 'lightgreen', 'lightcoral', 'gold'])
        axes[0, 1].set_ylim(0, 1)
        axes[0, 1].set_title('性能指标')
        axes[0, 1].set_ylabel('分数')
        for i, v in enumerate(values):
            axes[0, 1].text(i, v + 0.02, f'{v:.3f}', ha='center')

        # 3. PTT分布
        if 'ptt_mean' in df.columns and df['ptt_mean'].max() > 0:
            oh_positive = df[df['predicted_oh'] == True]['ptt_mean']
            oh_negative = df[df['predicted_oh'] == False]['ptt_mean']

            axes[0, 2].boxplot([oh_negative, oh_positive], labels=['阴性', '阳性'])
            axes[0, 2].set_title('PTT分布比较')
            axes[0, 2].set_ylabel('PTT (ms)')
            axes[0, 2].axhline(y=150, color='r', linestyle='--', alpha=0.5)
            axes[0, 2].axhline(y=280, color='r', linestyle='--', alpha=0.5)
            axes[0, 2].grid(True, alpha=0.3)

        # 4. 心率分布
        if 'heart_rate' in df.columns:
            axes[1, 0].hist(df['heart_rate'], bins=20, alpha=0.7, color='skyblue', edgecolor='black')
            axes[1, 0].axvline(x=50, color='r', linestyle='--', alpha=0.5, label='下限')
            axes[1, 0].axvline(x=100, color='r', linestyle='--', alpha=0.5, label='上限')
            axes[1, 0].set_xlabel('心率 (bpm)')
            axes[1, 0].set_ylabel('频数')
            axes[1, 0].set_title('心率分布')
            axes[1, 0].legend()
            axes[1, 0].grid(True, alpha=0.3)

        # 5. 预测vs真实
        correct = df[df['predicted_oh'] == df['true_oh']]
        incorrect = df[df['predicted_oh'] != df['true_oh']]

        axes[1, 1].scatter(correct.index, [1] * len(correct), color='green', label='正确', alpha=0.6, s=50)
        axes[1, 1].scatter(incorrect.index, [1] * len(incorrect), color='red', label='错误', alpha=0.6, s=50)
        axes[1, 1].set_xlabel('样本索引')
        axes[1, 1].set_yticks([1])
        axes[1, 1].set_yticklabels([''])
        axes[1, 1].set_title('预测结果（绿:正确, 红:错误）')
        axes[1, 1].legend()
        axes[1, 1].grid(True, alpha=0.3)

        # 6. 特征相关性热图
        numeric_cols = ['ptt_mean', 'heart_rate', 'systolic_mean', 'oh_confidence']
        numeric_cols = [col for col in numeric_cols if col in df.columns]

        if len(numeric_cols) > 1:
            corr_matrix = df[numeric_cols].corr()
            sns.heatmap(corr_matrix, annot=True, cmap='coolwarm', center=0,
                        square=True, ax=axes[1, 2], fmt='.2f')
            axes[1, 2].set_title('特征相关性热图')

        plt.suptitle('版本二：ECG+BP OH判定算法验证结果', fontsize=16, y=1.02)
        plt.tight_layout()

        # 保存图片
        plot_path = os.path.join(self.save_dir, "oh_validation_v2_plots.png")
        plt.savefig(plot_path, dpi=150, bbox_inches='tight')
        print(f"✅ 可视化图表已保存至: {plot_path}")

        plt.show()


# -------------------------- 主程序 --------------------------
if __name__ == "__main__":
    # 初始化验证器
    validator = DatasetValidator(
        data_folder=DATA_FOLDER,
        save_dir=SAVE_RESULT_DIR,
        fs=FS
    )

    # 执行验证
    results = validator.validate_all_records()

    # 打印总结报告
    print("\n" + "=" * 80)
    print("📊 版本二验证总结报告")
    print("=" * 80)

    metrics = validator.performance_metrics
    print(f"🔬 验证样本数: {metrics.get('total_samples', 0)}")
    print(f"🎯 准确率: {metrics.get('accuracy', 0):.2%}")


    # 显示混淆矩阵
    cm = metrics.get('confusion_matrix')
    if cm is not None:
        print("\n混淆矩阵:")
        print(f"        预测阴性   预测阳性")
        print(f"     {cm[0, 0]:^8}    {cm[0, 1]:^8}")

    print("\n" + "=" * 80)