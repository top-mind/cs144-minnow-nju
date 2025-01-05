from matplotlib import gridspec
import matplotlib.pyplot as plt
from scipy.stats import pearsonr
import numpy as np
from datetime import datetime
import matplotlib.dates as mdates
import argparse

parser = argparse.ArgumentParser(description='Check4')
parser.add_argument('file', type=str, help='file path')
args = parser.parse_args()

# 读取RTT数据
icmp_seq = []
timestamp = []
rtt_time = []
ip = ''

with open(args.file, 'r') as f:
    for line in f:
        if 'icmp_seq=' in line and 'time=' in line:
            seq = int(line.split('icmp_seq=')[1].split(' ')[0])
            rtt = int(line.split('time=')[1].split(' ms')[0])
            tm = float(line.split('[')[1].split(']')[0])
            if (ip == ''):
                ip = line.split(' ')[4].split(':')[0]
            timestamp.append(datetime.fromtimestamp(tm))
            icmp_seq.append(seq)
            rtt_time.append(rtt)

total = icmp_seq[-1]
receive = len(icmp_seq)
loss = total - receive

print(f'1. total {total}, receive {receive}, delivery rate {receive/total:.6f}')

current_success = 1
longest_success = 1
longest_loss = icmp_seq[0] - 1

for i in range(1, len(icmp_seq)):
    gap = icmp_seq[i] - icmp_seq[i-1]
    if gap > 1:
        current_success = 1
        longest_loss = max(longest_loss, gap - 1)
    else:
        current_success += 1
        longest_success = max(longest_success, current_success)

print(f'2. longest continuous successful ping: {longest_success}')
print(f'3. longest burst of losses: {longest_loss}')

block_loss = 1 if icmp_seq[0] != 1 else 0
block_not_success = 0

for i in range(1, len(icmp_seq)):
    if icmp_seq[i] - icmp_seq[i-1] > 1:
        block_loss += 1
        block_not_success += 1

print(f'4.1 Pr(receive(N+1) | receive(N)) = {(receive - 1 - block_not_success) / (receive - 1):.6f}')
if block_loss == 0:
    print(f'4.2 Pr(receive(N+1) | loss(N)) = NaN')
else:
    print(f'4.2 Pr(receive(N+1) | loss(N)) = {block_loss / loss:.6f}')

min_rtt = min(rtt_time)
max_rtt = max(rtt_time)

print(f'5. min RTT: {min_rtt} ms')
print(f'6. max RTT: {max_rtt} ms')

# 创建图表
fig = plt.figure(figsize=(10, 10))
gs = gridspec.GridSpec(2, 2, height_ratios=[1, 1])

ax1 = fig.add_subplot(gs[0, :])
ax2 = fig.add_subplot(gs[1, 0])
ax3 = fig.add_subplot(gs[1, 1])

# RTT 图表
ax1.plot(timestamp, rtt_time)
ax1.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
ax1.set_xlabel('time')
ax1.set_ylabel('rtt(ms)')
ax1.set_title(f'RTT of {ip} on {timestamp[0].strftime("%Y-%m-%d")}')

# RTT相关性图

x = rtt_time[:-1]  # 第N次ping的RTT
y = rtt_time[1:]   # 第N+1次ping的RTT

pearson_corr = pearsonr(x, y)
print(f'9. Pearson correlation coefficient: {pearson_corr[0]:.6f}')

ax3.scatter(x, y, s=1)
ax3.set_xlabel('RTT of ping #N (ms)')
ax3.set_ylabel('RTT of ping #N+1 (ms)')
ax3.set_title('9. Correlation between RTT of ping #N and RTT of ping #N+1')
ax3.set_aspect('equal', adjustable='box')
ax3.plot(x, np.poly1d(np.polyfit(x, y, 1))(x), color='red', label = 'Linear regression')

arr_time = np.array(rtt_time)

# plot_acf(arr_time, ax4)
# plot_pacf(arr_time, ax5)
# sm.qqplot(arr_time)
# rtt_time.sort()

# histogram of RTT
ax2.hist(rtt_time, bins=max_rtt - min_rtt + 1, color='blue', alpha=0.7)
ax2.set_xlabel('rtt (ms)')
ax2.set_ylabel('count')
ax2.set_title('8. Histogram of RTT')

# 调整布局并显示图表
plt.tight_layout()

plt.show()