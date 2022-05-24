import board
import digitalio
import time
import ulab.numpy as np

arr = np.zeros(1024)

for i in range(1024):
    arr[i] += np.sin(i/10)
    arr[i] += np.sin(i/4)
    arr[i] += np.cos(i/3)

#    print((arr[i],))
#    time.sleep(.1)


N = 1024
T = 1/800
x = np.linspace(0.0, N*T, N)
y = 5*np.sin(100*np.pi*x) + 25*np.sin(120*np.pi*x) + 35*np.sin(90*np.pi*x)
yf = np.fft.fft(y)
xf = np.linspace(0.0, 1.0/(2.0*T), N//2)
print(max(yf[1]))
for i in range(N/8):
    print((2.0/N * abs(yf[1][i]),))
    time.sleep(.01)

led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

while True:
    led.value = not led.value
    time.sleep(1)
