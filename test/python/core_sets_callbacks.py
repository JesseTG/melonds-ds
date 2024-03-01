from sys import argv
from libretro import Core

core = Core(argv[1])

core.set_video_refresh(lambda data, width, height, pitch: None)
core.set_audio_sample(lambda left, right: None)
core.set_audio_sample_batch(lambda data, frames: 0)
core.set_input_poll(lambda: None)
core.set_input_state(lambda port, device, index, id: 0)
core.set_environment(lambda cmd, data: False)