local preamble = dofile(os.getenv("PREAMBLE"))

set_options_string(preamble.options_string)

load_core(corepath)
load_game(rompath)

run()

-- Take the CRC of whatever a blank white screen looks like
local blank_white_crc = get_fb_crc()

for _ = 1, 30 do
    run()
end

-- If the CRC is still the same, then the screen is still white
for _ = 1, 300 do
    run()
end

local non_blank_crc = get_fb_crc()
if (non_blank_crc == blank_white_crc) then
    print(get_logs())
    error("Screen is still white after 330 frames")
end

reset()

for _ = 1, 300 do
    run()
end

local reset_crc = get_fb_crc()
if (reset_crc == blank_white_crc) then
    print(get_logs())
    error("Emulator is stuck on blank white screen")
end

unload_game()

print(get_logs())