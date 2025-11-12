import numpy as np
import matplotlib.pyplot as plt

# Чтение переданного сигнала (scatter plot)
transmitted_data = np.loadtxt('bpsk_scatter_data.csv', delimiter=',')
I_tx = transmitted_data[:, 0]
Q_tx = transmitted_data[:, 1]

# Чтение принятого сигнала
received_data = np.fromfile('received_bpsk.raw', dtype=np.int16)
I_rx = received_data[0::2]  # Четные - I
Q_rx = received_data[1::2]  # Нечетные - Q

# Визуализация
plt.figure(figsize=(15, 5))

# Переданный сигнал
plt.subplot(1, 3, 1)
plt.scatter(I_tx, Q_tx, alpha=0.6, s=10)
plt.title('Переданный BPSK сигнал')
plt.xlabel('I')
plt.ylabel('Q')
plt.grid(True)
plt.axis('equal')

# Принятый сигнал
plt.subplot(1, 3, 2)
plt.scatter(I_rx, Q_rx, alpha=0.6, s=10)
plt.title('Принятый BPSK сигнал')
plt.xlabel('I')
plt.ylabel('Q')
plt.grid(True)
plt.axis('equal')

# Сравнение
plt.subplot(1, 3, 3)
plt.scatter(I_tx[:1000], Q_tx[:1000], alpha=0.6, s=10, label='Переданный')
plt.scatter(I_rx[:1000], Q_rx[:1000], alpha=0.6, s=10, label='Принятый')
plt.title('Сравнение')
plt.xlabel('I')
plt.ylabel('Q')
plt.grid(True)
plt.legend()

plt.tight_layout()
plt.show()