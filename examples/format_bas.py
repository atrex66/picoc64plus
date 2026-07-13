import re
import sys

def format_basic_line(line: str) -> str:
    line = line.rstrip("\r\n")
    m = re.match(r"^(\d+)([ \t]*)(.*)$", line)
    if not m:
        return line.upper()

    line_no, _, code = m.groups()

    # Több szóköz/tab → egy szóköz a sorszám után
    code = re.sub(r"[ \t]+", " ", code).strip()

    return f"{line_no} {code.upper()}" if code else line_no

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Használat: {sys.argv[0]} bemenet.bas kimenet.bas")
        sys.exit(1)

    with open(sys.argv[1], "r", encoding="utf-8") as fin, \
         open(sys.argv[2], "w", encoding="utf-8", newline="\n") as fout:
        for line in fin:
            fout.write(format_basic_line(line) + "\n")

            