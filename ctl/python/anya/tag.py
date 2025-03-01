try:
    from .__tag import TAG
except ImportError:
    TAG = "Anya-???"

from .debug import *

def print_tag() -> str:
    info(TAG)
    info("made by john (@nyan_satan)")
    print()
