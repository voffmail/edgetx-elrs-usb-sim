import argparse
import struct

MAGIC_START0 = 0x0A324655
MAGIC_START1 = 0x9E5D5157
MAGIC_END = 0x0AB16F30
FLAG_FAMILY_ID = 0x00002000


def main():
    parser = argparse.ArgumentParser(description="Convert a flash image to UF2")
    parser.add_argument("input")
    parser.add_argument("output")
    parser.add_argument("--family", type=lambda value: int(value, 0), default=0xE48BFF59)
    parser.add_argument("--base", type=lambda value: int(value, 0), default=0x10000000)
    parser.add_argument("--page", type=lambda value: int(value, 0), default=256)
    args = parser.parse_args()

    with open(args.input, "rb") as source:
        data = source.read()

    pages = (len(data) + args.page - 1) // args.page

    with open(args.output, "wb") as target:
        for index in range(pages):
            chunk = data[index * args.page:(index + 1) * args.page]
            payload = chunk.ljust(args.page, b"\xff")

            block = bytearray(512)
            struct.pack_into(
                "<IIIIIIII",
                block,
                0,
                MAGIC_START0,
                MAGIC_START1,
                FLAG_FAMILY_ID,
                args.base + index * args.page,
                args.page,
                index,
                pages,
                args.family,
            )
            block[32:32 + len(payload)] = payload
            struct.pack_into("<I", block, 508, MAGIC_END)
            target.write(block)


if __name__ == "__main__":
    main()
