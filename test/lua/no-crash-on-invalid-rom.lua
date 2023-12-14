local preamble = dofile(os.getenv("PREAMBLE"))

set_options_string(preamble.options_string)

load_core(corepath)
print(get_logs())
load_game(rompath)
