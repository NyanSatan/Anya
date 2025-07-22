import sys
import time

NONE   = ""
RED    = "\u001b[31m"
GREEN  = "\u001b[32m"
YELLOW = "\u001b[33m"
CYAN   = "\u001b[36m"
PURPLE = "\u001b[38;5;57m"
BOLD   = "\u001b[1m"
RESET  = "\u001b[0m"

start_time = 0.0
debug_enabled = False

def anya_debug_init(debug: bool):
    global start_time, debug_enabled

    start_time = time.time()   
    debug_enabled = debug

def _log(color: str, args: tuple, kwargs: dict):
    print(color, end="", file=sys.stderr)
    print(*args, **kwargs, end="", file=sys.stderr)
    print(RESET, file=sys.stderr)

def debug(*args, **kwargs):
    if debug_enabled:
        _log(PURPLE+BOLD, args, kwargs)

def success(*args, **kwargs):
    _log(GREEN+BOLD, args, kwargs)

def warning(*args, **kwargs):
    _log(YELLOW+BOLD, args, kwargs)

def error(*args, **kwargs):
    _log(RED+BOLD, args, kwargs)

def info(*args, **kwargs):
    _log(BOLD, args, kwargs)

class Logger:
    def __init__(self, name: str) -> None:
        self._name = name.upper()

    def _log(self, outfunc: callable, args, kwargs):
        prefix = "[%.3f][%6s]" % (time.time() - start_time, self._name)
        outfunc(*(prefix, *args,), **kwargs)

    def debug(self, *args, **kwargs):
        self._log(debug, args, kwargs)

    def success(self, *args, **kwargs):
        self._log(success, args, kwargs)

    def warning(self, *args, **kwargs):
        self._log(warning, args, kwargs)

    def error(self, *args, **kwargs):
        self._log(error, args, kwargs)

    def info(self, *args, **kwargs):
        self._log(info, args, kwargs)
