local preamble = dofile(os.getenv("PREAMBLE"))

set_options_string(preamble.options_string)

load_core(corepath)
load_game(rompath)
run()

local cmake = os.getenv("CMAKE")
local ok, reason, code = os.execute(string.format('"%s" -E chdir "%s" true', cmake, preamble.core_save_dir .. "/dldi_sd_card"))
if ok ~= true then
    print(get_logs())
    error("Failed to open homebrew SD card sync directory")
end
