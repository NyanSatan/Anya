try:
    from .__tag import TAG
except ImportError:
    TAG = "Anya-???"

def print_tag() -> str:
    print(TAG)
    print("made by john (@nyan_satan)")
