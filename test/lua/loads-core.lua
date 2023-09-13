local preamble = dofile(os.getenv("PREAMBLE"))
load_core(corepath)
load_game(rompath)

for _ = 1, 30 do
    run()
end

unload_game()

print(get_logs())