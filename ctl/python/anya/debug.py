# this file is taken from Eva

import time

NONE = ""
RED = "\u001b[31m"
GREEN = "\u001b[32m"
YELLOW = "\u001b[33m"
CYAN = "\u001b[36m"
PURPLE = "\u001b[38;5;57m"
BOLD = "\u001b[1m"
RESET = "\u001b[0m"

start_time = 0.0
debug_enabled = False

def debug_early_init(debug: bool):
    global start_time, debug_enabled

    start_time = time.time()   
    debug_enabled = debug

class Logger:
    def __init__(self, name: str, enabled: bool = True) -> None:
        self._name = name.upper()
        self.enabled = enabled

    def _log(self, color: str, args: list, kwargs: dict):
        if self.enabled:
            print((BOLD + color + "[%6f][%8s]") % (time.time() - start_time, self._name), end=" ")
            print(*args, **kwargs, end=" ")
            print(RESET)

    def debug(self, *args, **kwargs):
        if debug_enabled:
            self._log(PURPLE, args, kwargs)

    def success(self, *args, **kwargs):
        self._log(GREEN, args, kwargs)

    def warning(self, *args, **kwargs):
        self._log(YELLOW, args, kwargs)

    def error(self, *args, **kwargs):
        self._log(RED, args, kwargs)

    def info(self, *args, **kwargs):
        self._log(NONE, args, kwargs)
