import numpy as np
import matplotlib.pyplot as plt

# Открываем файл для чтения принятых данных
name = "txdata.pcm"

# Читаем все данные как int16
data = np.fromfile(name, dtype=np.int16)

# Разделяем на I и Q компоненты
# Формат: I0, Q0, I1, Q1, I2, Q2, ...
real = data[0::2]  # Четные элементы - I (real)
imag = data[1::2]  # Нечетные элементы - Q (imag)

# Создаем массив индексов
count = np.arange(len(real))

# Построение графиков
plt.figure(figsize=(12, 6))
plt.plot(count, imag, color='red', label='Imag (Q)', linewidth=1, marker='o', markersize=2)
plt.plot(count, real, color='blue', label='Real (I)', linewidth=1, marker='s', markersize=2)
plt.xlabel('Samples')
plt.ylabel('Amplitude')
plt.title('I/Q Samples from PlutoSDR')
plt.legend()
plt.grid(True)
plt.show()

