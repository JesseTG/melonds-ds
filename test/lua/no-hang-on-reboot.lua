local preamble = dofile(os.getenv("PREAMBLE"))

set_options_string(preamble.options_string)

load_core(corepath)
load_game(rompath)

print("Loaded core and ROM")

run()

print("Ran first frame")

-- Take the CRC of whatever a blank white screen looks like
local blank_white_crc = get_fb_crc()

for _ = 1, 300 do
    run()
end

print("Ran for 300 frames")

-- If the CRC is still the same, then the screen is still white
local non_blank_crc = get_fb_crc()
if (non_blank_crc == blank_white_crc) then
    print(get_logs())
    error("Screen is still white after 300 frames")
end

print("About to reset core")
reset()

print("Reset core")
print("About to run for 300 more frames")

for i = 1, 300 do
    run()
end

print("Ran for 300 frames after reset")

local reset_crc = get_fb_crc()
if (reset_crc == blank_white_crc) then
    print(get_logs())
    error("Emulator is stuck on blank white screen")
end

unload_game()

print("Unloaded game")

print(get_logs())