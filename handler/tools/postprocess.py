import sys

PLACEHOLDER = "ANYA_TAG_PLACEHOLDER"
MAX_LEN = 96

def main():
    try:
        _, binary, replacement = sys.argv
    except IndexError:
        print("usage: %s <binary> <replacement>" % sys.argv[0])
        exit(-1)

    p = PLACEHOLDER.encode("ascii")
    b = replacement.encode("ascii")

    if len(b) > MAX_LEN:
        print("replacement is too long")
        exit(-1)

    with open(binary, "rb") as f:
        buf = bytearray(f.read())

    idx = buf.find(p)
    if idx == -1:
        print("placeholder string wasn't found")
        exit(-1)

    buf[idx:idx+len(b)] = b

    with open(binary, "wb") as f:
        f.write(buf)

if __name__ == "__main__":
    main()
