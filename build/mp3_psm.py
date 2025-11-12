import numpy as np
import librosa
from pydub import AudioSegment


y, sr = librosa.load("1.mp3", sr=44100, mono=False)
if y.ndim == 2:
    y = y.T.reshape(-1)
pcm_data = (y * 32767).astype(np.int16)
pcm_data.tofile("1.pcm")
