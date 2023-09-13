local SYSTEM_FILES = {"ARM7_BIOS", "ARM9_BIOS", "ARM7_DSI_BIOS", "ARM9_DSI_BIOS", "NDS_FIRMWARE", "DSI_FIRMWARE", "DSI_NAND"}
local emutest_dir = os.getenv("EMUTEST_HOME")
local system_dir = emutest_dir .. "/system"
local save_dir = emutest_dir .. "/savefiles"
local savestate_directory = emutest_dir .. "/states"
local core_system_dir = system_dir .. "/melonDS DS"

print("Test dir:", emutest_dir)
print("System dir:", system_dir)
print("Save dir:", save_dir)
print("Savestate dir:", savestate_directory)
print("Core system dir:", core_system_dir)

-- We can assume the presence of CMake,
-- otherwise how would we be running the test suite?
os.execute(string.format('cmake -E rm -rf "%s" "%s" "%s"', system_dir, save_dir, savestate_directory))
os.execute(string.format('cmake -E make_directory "%s" "%s" "%s"', core_system_dir, save_dir, savestate_directory))

for _, f in ipairs(SYSTEM_FILES) do
    local sysfile = os.getenv(f)
    if sysfile ~= nil and sysfile ~= "" then
        local sysname = os.getenv(f .. "_NAME")
        assert(sysname ~= nil and sysname ~= "")
        local cmd = string.format('cmake -E copy "%s" "%s/%s"', sysfile, core_system_dir, sysname)
        os.execute(cmd)
    end
end

local preamble = {}

preamble.options_string = os.getenv("RETRO_CORE_OPTIONS")

return preamble