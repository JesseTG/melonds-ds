local preamble = dofile(os.getenv("PREAMBLE"))

set_options_string(preamble.options_string)

load_core(corepath)
load_game(rompath)
run()

local homebrew_sd_card, sdcard_err = io.open(preamble.core_save_dir .. "/dldi_sd_card.bin", "rb")
if homebrew_sd_card == nil or sdcard_err ~= nil then
    print(get_logs())
    error("Failed to open homebrew SD card: " .. sdcard_err)
end
