load_core(corepath)
load_game(rompath)

for _ = 1, 60 do
    run()
end

print(get_logs())