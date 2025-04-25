import numpy as np
import matplotlib.pyplot as plt

# Parameters
period = 1.0                   # total period of the waveform (seconds)
spike_width = 0.05             # width of each upward spike
plateau_width = 0.2            # width of the plateau between spikes
ramp_down_width = (period - spike_width - plateau_width)  # width of the downward ramp

samples_per_period = 1000
t = np.linspace(0, period, samples_per_period)
wave = np.zeros_like(t)

# Define time segments
spike_start = 0
spike_end = spike_start + spike_width
ramp_down_start = spike_end
ramp_down_end = ramp_down_start + ramp_down_width
plateau_start = ramp_down_end
plateau_end = plateau_start + plateau_width

# Generate waveform
for i in range(len(t)):
    if t[i] < spike_end:
        wave[i] = t[i] / spike_width  # sharp upward spike
    elif t[i] < ramp_down_end:
        wave[i] = 1.0 - ((t[i] - spike_end) / ramp_down_width)  # ramp down
    elif t[i] < plateau_end:
        wave[i] = 0.0  # plateau
    else:
        wave[i] = 0.0  # extra time stays flat if needed

# Repeat waveform over multiple periods
num_periods = 4
t_full = np.tile(t, num_periods) + np.repeat(np.arange(num_periods) * period, samples_per_period)
wave_full = np.tile(wave, num_periods)

# Plot
plt.figure(figsize=(10, 4))
plt.plot(t_full, wave_full)
plt.title("Repeating Triangle Wave with Plateau Between Spikes")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")
plt.grid(True)
plt.tight_layout()
plt.show()
