from argparse import ArgumentParser
from pathlib import Path
import gzip, math, re, os

parser = ArgumentParser(description="SGU Embedded Gen")
parser.add_argument("-i", "--inputpath", required=True, help="Input path (files to embed)")
parser.add_argument("-o", "--outputcodepath", required=True, help="Output path for code")
parser.add_argument("-c", "--compress", default=None, help="Input compression configuration file")

args = parser.parse_args()

def GetSizeString(size):
    unit = int(math.log2(size) / 10) if size else 0
    size /= math.pow(2, unit * 10)
    return f"{round(size)} {' KMGTPE'[unit]}B"

def BinToHexArray(data):
    hexdata = "\\x" + data.hex("_", 1).replace("_", "\\x")
    output = []
    pos = 0
    while pos < len(hexdata):
        output.append(f'"{hexdata[pos:pos+400]}"')
        pos += 400
    return "\n".join(output)

class PackedFile:
    def __init__(self, inputpath, path, compress=False):
        # Relative path should remove the input path and keep the relative part
        relpath = os.path.relpath(str(path).replace("\\", "/"), inputpath.replace("\\", "/")).replace("\\", "/")

        self.path = Path(path)  # Path object
        self.relpath = relpath  # Relative normalized path
        self.keyname = "EF_EFILE_%s" % re.sub("[^a-zA-Z0-9]", "_", self.relpath).upper()
        self.blob = self.path.open("rb").read()  # Binary data including padding
        self.size = len(self.blob)  # real size before compression/padding
        self.blob += b"\x00"  # End marker
        self.compressed = False
        if compress:
            try:
                self.blob = gzip.compress(self.blob, 9)
                self.compressed = True
            except:
                print(f"WARNING: Failed to compress {self.relpath} !")
        self.blob += b"\x00" * (4 - (len(self.blob) % 4))  # alignment
        self.flags = 'EF_EFLAGS_GZip' if self.compressed else 'EF_EFLAGS_None'

try:
    diInput = Path(args.inputpath)
    if not diInput.exists():
        raise Exception("Input path doesn't exists")

    diOutputCodePath = Path(args.outputcodepath)
    if not diOutputCodePath.exists():
        raise Exception("Output path doesn't exists")

    compressConfig = Path(args.compress) if args.compress else None
    if compressConfig and not compressConfig.exists():
        raise Exception("No GZIP config file provided")

    # Scan for files
    compressFiles = [Path(file) for file in compressConfig.read_text().splitlines()] if compressConfig else []
    files = []

    # print(compressFiles)

    for file in diInput.rglob("*.*"):
        myfile = PackedFile(args.inputpath, file, len([p for p in [file, *file.parents] if p in compressFiles]) > 0)
        files.append(myfile)
        print(f"Adding file: {myfile.relpath} {'(compressed)' if myfile.compressed else ''}")

    fileTXT = diOutputCodePath / "EmbeddedFiles.txt"
    print(f"Export list: {fileTXT}")
    fileTXT.write_text("\n".join(file.relpath for file in files))

    fileB = diOutputCodePath / "EmbeddedFiles.bin"
    print(f"Generating file: {fileB}")
    fileB.write_bytes(b"".join(file.blob for file in files))

    fileH = diOutputCodePath / "EmbeddedFiles.h"
    print(f"Generating file: {fileH}")
    with fileH.open("w") as fp:
        fp.write("#ifndef _EMBEDDEDFILES_H_\n")
        fp.write("#define _EMBEDDEDFILES_H_\n")
        fp.write("\n")
        fp.write("#include <stdint.h>\n")
        fp.write("\n")
        fp.write("typedef enum\n")
        fp.write("{\n")
        fp.write("    EF_EFLAGS_None = 0,\n")
        fp.write("    EF_EFLAGS_GZip = 1,\n")
        fp.write("} EF_EFLAGS;\n")
        fp.write("\n")
        fp.write("typedef struct\n")
        fp.write("{\n")
        fp.write("    const char* strFilename;\n")
        fp.write("    uint32_t u32Length;\n")
        fp.write("    EF_EFLAGS eFlags;\n")
        fp.write("    const uint8_t* pu8StartAddr;\n")
        fp.write("} EF_SFile;\n")
        fp.write("\n")
        fp.write("typedef enum\n")
        fp.write("{\n")
        for file in files:
            fp.write(f"    {file.keyname} = {files.index(file)},    /*!< @brief File: {file.relpath} (size: {GetSizeString(file.size)}) */\n")
        fp.write(f"    EF_EFILE_COUNT = {len(files)}\n")
        fp.write("} EF_EFILE;\n")
        fp.write("\n")
        fp.write("/*! @brief Check if compressed flag is active */\n")
        fp.write("#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)\n")
        fp.write("\n")
        fp.write("extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];\n")
        fp.write("extern const uint8_t EF_g_u8Blobs[];\n")
        fp.write("\n")
        fp.write("#endif\n")

    fileC = diOutputCodePath / "EmbeddedFiles.c"
    print(f"Generating file: {fileC}")
    with fileC.open("w") as fp:
        bigBlobs = b"".join(file.blob for file in files)
        fp.write('#include "EmbeddedFiles.h"\n')
        fp.write("\n")
        fp.write(f"/*! @brief Total size: {GetSizeString(sum(file.size for file in files))}, Packed size: {GetSizeString(len(bigBlobs))} */\n")
        fp.write("const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = \n")
        fp.write("{\n")
        offset = 0
        for file in files:
            fp.write(f"    [{file.keyname}] = {{ \"{file.relpath}\", {file.size}, {file.flags}, &EF_g_u8Blobs[{offset}]}},")
            fp.write(f"/* packed size: {GetSizeString(len(file.blob))}, original size: {GetSizeString(file.size)} */")
            fp.write("\n")
            offset += len(file.blob)
        fp.write("};\n")
        fp.write("\n")
        fp.write(f"const uint8_t EF_g_u8Blobs[] = \n{BinToHexArray(bigBlobs)};\n")

except Exception as e:
    # raise (e)
    print(f"Error: {e}")
