"""
Process raw qstr file and output qstr data with length, hash and data bytes.

This script works with Python 2.6, 2.7, 3.3 and 3.4.
"""

from __future__ import print_function

import re
import sys

import collections
import gettext

sys.path.append("../../tools/huffman")

import huffman

# Python 2/3 compatibility:
#   - iterating through bytes is different
#   - codepoint2name lives in a different module
import platform
if platform.python_version_tuple()[0] == '2':
    bytes_cons = lambda val, enc=None: bytearray(val)
    from htmlentitydefs import codepoint2name
elif platform.python_version_tuple()[0] == '3':
    bytes_cons = bytes
    from html.entities import codepoint2name
# end compatibility code

codepoint2name[ord('-')] = 'hyphen';

# add some custom names to map characters that aren't in HTML
codepoint2name[ord(' ')] = 'space'
codepoint2name[ord('\'')] = 'squot'
codepoint2name[ord(',')] = 'comma'
codepoint2name[ord('.')] = 'dot'
codepoint2name[ord(':')] = 'colon'
codepoint2name[ord(';')] = 'semicolon'
codepoint2name[ord('/')] = 'slash'
codepoint2name[ord('%')] = 'percent'
codepoint2name[ord('#')] = 'hash'
codepoint2name[ord('(')] = 'paren_open'
codepoint2name[ord(')')] = 'paren_close'
codepoint2name[ord('[')] = 'bracket_open'
codepoint2name[ord(']')] = 'bracket_close'
codepoint2name[ord('{')] = 'brace_open'
codepoint2name[ord('}')] = 'brace_close'
codepoint2name[ord('*')] = 'star'
codepoint2name[ord('!')] = 'bang'
codepoint2name[ord('\\')] = 'backslash'
codepoint2name[ord('+')] = 'plus'
codepoint2name[ord('$')] = 'dollar'
codepoint2name[ord('=')] = 'equals'
codepoint2name[ord('?')] = 'question'
codepoint2name[ord('@')] = 'at_sign'
codepoint2name[ord('^')] = 'caret'
codepoint2name[ord('|')] = 'pipe'
codepoint2name[ord('~')] = 'tilde'

# this must match the equivalent function in qstr.c
def compute_hash(qstr, bytes_hash):
    hash = 5381
    for b in qstr:
        hash = (hash * 33) ^ b
    # Make sure that valid hash is never zero, zero means "hash not computed"
    return (hash & ((1 << (8 * bytes_hash)) - 1)) or 1

def translate(translation_file, i18ns):
    with open(translation_file, "rb") as f:
        table = gettext.GNUTranslations(f)

        return [(x, table.gettext(x)) for x in i18ns]

def qstr_escape(qst):
    def esc_char(m):
        c = ord(m.group(0))
        try:
            name = codepoint2name[c]
        except KeyError:
            name = '0x%02x' % c
        return "_" + name + '_'
    return re.sub(r'[^A-Za-z0-9_]', esc_char, qst)

def parse_input_headers(infiles):
    # read the qstrs in from the input files
    qcfgs = {}
    qstrs = {}
    i18ns = set()
    for infile in infiles:
        with open(infile, 'rt') as f:
            for line in f:
                line = line.strip()

                # is this a config line?
                match = re.match(r'^QCFG\((.+), (.+)\)', line)
                if match:
                    value = match.group(2)
                    if value[0] == '(' and value[-1] == ')':
                        # strip parenthesis from config value
                        value = value[1:-1]
                    qcfgs[match.group(1)] = value
                    continue


                match = re.match(r'^I18N\("(.*)"\)$', line)
                if match:
                    i18ns.add(match.group(1))
                    continue

                # is this a QSTR line?
                match = re.match(r'^Q\((.*)\)$', line)
                if not match:
                    continue

                # get the qstr value
                qstr = match.group(1)

                # special case to specify control characters
                if qstr == '\\n':
                    qstr = '\n'

                # work out the corresponding qstr name
                ident = qstr_escape(qstr)

                # don't add duplicates
                if ident in qstrs:
                    continue

                # add the qstr to the list, with order number to retain original order in file
                order = len(qstrs)
                # but put special method names like __add__ at the top of list, so
                # that their id's fit into a byte
                if ident == "":
                    # Sort empty qstr above all still
                    order = -200000
                elif ident.startswith("__"):
                    order -= 100000
                qstrs[ident] = (order, ident, qstr)

    if not qcfgs:
        sys.stderr.write("ERROR: Empty preprocessor output - check for errors above\n")
        sys.exit(1)

    return qcfgs, qstrs, i18ns

def make_bytes(cfg_bytes_len, cfg_bytes_hash, qstr):
    qbytes = bytes_cons(qstr, 'utf8')
    qlen = len(qbytes)
    qhash = compute_hash(qbytes, cfg_bytes_hash)
    if all(32 <= ord(c) <= 126 and c != '\\' and c != '"' for c in qstr):
        # qstr is all printable ASCII so render it as-is (for easier debugging)
        qdata = qstr
    else:
        # qstr contains non-printable codes so render entire thing as hex pairs
        qdata = ''.join(('\\x%02x' % b) for b in qbytes)
    if qlen >= (1 << (8 * cfg_bytes_len)):
        print('qstr is too long:', qstr)
        assert False
    qlen_str = ('\\x%02x' * cfg_bytes_len) % tuple(((qlen >> (8 * i)) & 0xff) for i in range(cfg_bytes_len))
    qhash_str = ('\\x%02x' * cfg_bytes_hash) % tuple(((qhash >> (8 * i)) & 0xff) for i in range(cfg_bytes_hash))
    return '(const byte*)"%s%s" "%s"' % (qhash_str, qlen_str, qdata)

def print_qstr_data(qcfgs, qstrs, i18ns):
    # get config variables
    cfg_bytes_len = int(qcfgs['BYTES_IN_LEN'])
    cfg_bytes_hash = int(qcfgs['BYTES_IN_HASH'])

    # print out the starter of the generated C header file
    print('// This file was automatically generated by makeqstrdata.py')
    print('')

    # add NULL qstr with no hash or data
    print('QDEF(MP_QSTR_NULL, (const byte*)"%s%s" "")' % ('\\x00' * cfg_bytes_hash, '\\x00' * cfg_bytes_len))

    all_strings = [x[0] for x in i18ns]

    # go through each qstr and print it out
    for order, ident, qstr in sorted(qstrs.values(), key=lambda x: x[0]):
        qbytes = make_bytes(cfg_bytes_len, cfg_bytes_hash, qstr)
        print('QDEF(MP_QSTR_%s, %s)' % (ident, qbytes))
        all_strings.append(qstr)

    all_strings_concat = "".join(all_strings).encode("utf-8")
    counts = collections.Counter(all_strings_concat)
    # add other values
    for i in range(256):
        if i not in counts:
            counts[i] = 0
    cb = huffman.codebook(counts.items())
    values = bytearray()
    length_count = {}
    renumbered = 0
    last_l = None
    canonical = {}
    for ch, code in sorted(cb.items(), key=lambda x: (len(x[1]), x[0])):
        values.append(ch)
        l = len(code)
        if l not in length_count:
            length_count[l] = 0
        length_count[l] += 1
        if last_l:
            renumbered <<= (l - last_l)
        canonical[ch] = '{0:0{width}b}'.format(renumbered, width=l)
        print("//", ch, chr(ch), counts[ch], canonical[ch])
        renumbered += 1
        last_l = l
    print("//", values, length_count)

    lengths = bytearray()
    for i in range(1, max(length_count) + 1):
        lengths.append(length_count.get(i, 0))
    print("//", lengths)

    def cheat_compress(s):
        enc = ""
        for c in s:
            if c in canonical:
                enc += canonical[c]
            else:
                raise RuntimeError()
                # enc += canonical[verbatim]
                # pad = 8 - (len(enc) % 8)
                # enc += "0" * pad
                # enc += '{:08b}'.format(c)
        i = 0
        benc = bytearray()
        while i < len(enc):
            pad = 8 - (len(enc) - i)
            b = enc[i:i+8] + "0" * pad
            benc.append(int(b, 2))
            i += 8
        return benc

    def decompress(l, encoded):
        #print(l, encoded)
        dec = bytearray(l)
        this_byte = 0
        this_bit = 7
        b = encoded[this_byte]
        is_verbatim = 0
        for i in range(l):
            if is_verbatim:
                dec[i] = b
                b = encoded[this_byte]
                is_verbatim -= 1

            bits = 0
            bit_length = 0
            max_code = lengths[0]
            searched_length = lengths[0]
            while True:
                bits <<= 1
                if 0x80 & b:
                    bits |= 1

                b <<= 1
                bit_length += 1
                if this_bit == 0:
                    this_bit = 7
                    this_byte += 1
                    if this_byte < len(encoded):
                        b = encoded[this_byte]
                else:
                    this_bit -= 1
                #print(bit_length, bin(bits), bits, max_code)
                if max_code > 0 and bits < max_code:
                    break
                max_code = (max_code << 1) + lengths[bit_length]
                searched_length += lengths[bit_length]
            v = values[searched_length + bits - max_code]

            # if v == verbatim:
            #     while this_bit > 0:
            #         is_verbatim = is_verbatim << 1
            #         if 0x80 & b:
            #             is_verbatim |= 1
            #         b <<= 1
            #         if this_bit == 0:
            #             this_bit = 7
            #             this_byte += 1
            #             b = encoded[this_byte]
            #         else:
            #             this_bit -= 1
            #     is_verbatim += 1
            #     print("v!", is_verbatim)
            #print(v, chr(v))
            dec[i] = v
        return dec

    compressed = 0
    uncompressed = 0
    for s in all_strings:
    #for s in ["\n"]:
        if not s:
            continue
        this_size = 0
        rle = ""
        for c in s.encode("utf-8"):
            c_size = len(cb.get(c, ""))
            this_size += c_size
            if c_size > 8:
                rle += str(c_size - 8)
            else:
                rle += "0"
        extra = 0
        if this_size % 8 > 0:
            extra = 1
        cc = cheat_compress(s.encode("utf-8"))
        # this_size // 8 + extra, this_size % 8, rle,
        print("//", len(s), len(cc), s, [ord(x) for x in s], cc)
        print("//", decompress(len(s), cc))
        compressed += len(cc)
        uncompressed += len(s)

    print("// {} bytes".format(uncompressed))
    print("// {} compressed bytes".format(compressed))
    # for original, translation in i18ns:
    #     print("TRANSLATION(\"{}\", \"{}\")".format(original, translation))

def do_work(translation_file, infiles):
    qcfgs, qstrs, i18ns = parse_input_headers(infiles)
    translations = translate(translation_file, i18ns)
    print_qstr_data(qcfgs, qstrs, translations)

if __name__ == "__main__":
    do_work(sys.argv[1], sys.argv[2:])
