local preamble = dofile(os.getenv("PREAMBLE"))

local nds_firmware, nds_err = io.open(os.getenv("NDS_FIRMWARE"), "rb")
if nds_firmware == nil or nds_err ~= nil then
    print(get_logs())
    error("Failed to open NDS firmware: " .. nds_err)
end

local dsi_firmware, dsi_err = io.open(os.getenv("DSI_FIRMWARE"), "rb")
if dsi_firmware == nil or dsi_err ~= nil then
    print(get_logs())
    error("Failed to open DSi firmware: " .. dsi_err)
end

-- Need to check the size of the firmware files
-- because go-lua doesn't support file:read()
local nds_firmware_size = nds_firmware:seek("end")
local dsi_firmware_size = dsi_firmware:seek("end")

nds_firmware:close()
dsi_firmware:close()

if nds_firmware_size == nil then
    error("Failed to read NDS firmware")
end

if dsi_firmware_size == nil then
    error("Failed to read DSi firmware")
end

if nds_firmware_size == dsi_firmware_size then
    error("NDS firmware and DSI firmware are the same size, this test is pointless")
end

load_core(corepath)
load_game(rompath)

set_options_string(preamble.options_string)

for _ = 1, 30 do
    run()
end

local initial_mode = get_option("melonds_console_mode")

if initial_mode == nil then
    print(get_logs())
    error("melonds_console_mode is nil (is this version of emutest too old?)")
end

if initial_mode == "ds" then
    print("Switching from NDS to DSi mode")
    set_option("melonds_console_mode", "dsi")
else
    print("Switching from DSi to NDS mode")
    set_option("melonds_console_mode", "ds")
end

print("Resetting core")

reset()

for _ = 1, 30 do
    run()
end

nds_firmware = io.open(os.getenv("NDS_FIRMWARE"), "rb")
dsi_firmware = io.open(os.getenv("DSI_FIRMWARE"), "rb")

local nds_firmware_size_after, nds_err_after = nds_firmware:seek("end")
if nds_err_after ~= nil then
    print(get_logs())
    error("Failed to open NDS firmware: " .. nds_err_after)
end

local dsi_firmware_size_after, dsi_err_after = dsi_firmware:seek("end")
if dsi_err_after ~= nil then
    print(get_logs())
    error("Failed to open DSi firmware: " .. dsi_err_after)
end

nds_firmware:close()
dsi_firmware:close()

if nds_firmware_size_after == dsi_firmware_size_after then
    print(get_logs())
    error("Firmware was overwritten")
end

print(get_logs())