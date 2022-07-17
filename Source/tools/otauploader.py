from argparse import ArgumentParser
from urllib import request

parser = ArgumentParser(description="SGU OTA Uploader")
parser.add_argument("--destaddr", required=True, help="Destination address (URL)")
parser.add_argument("--input", required=True, help="Input filename")

args = parser.parse_args()

try:
    print(f"Starting to upload: '{args.input}', address: '{args.destaddr}'")
    # I guess we should add a wrapper to track upload progress...
    request.urlopen(args.destaddr, open(args.input, 'rb').read())
    print(f"Done!")
except Exception as e:
    print(f"Error: {e}")
