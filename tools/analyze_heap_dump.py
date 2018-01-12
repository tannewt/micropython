import binascii
import struct
import sys
import pygraphviz as pgv

BITS_PER_BYTE = 8
BLOCKS_PER_ATB = 4
BLOCKS_PER_FTB = 8
BYTES_PER_BLOCK = 16

AT_FREE = 0
AT_HEAD = 1
AT_TAIL = 2
AT_MARK = 3

MICROPY_QSTR_BYTES_IN_HASH = 1
MICROPY_QSTR_BYTES_IN_LEN = 1

MP_OBJ_NULL = 0
MP_OBJ_SENTINEL = 4

last_pool = 0x200031b0
heap_start = 0x20000406

tuple_type = 0x321c4
type_type = 0x32470
map_type = 0x311a4
dict_type = 0x30504
property_type = 0x31440
str_type = 0x32040
function_types = [0x30cd8, 0x30d14, 0x30d50, 0x30d8c, 0x30dc8, 0x30c9c] # make sure and add mp_type_fun_bc

pool_shift = heap_start % BYTES_PER_BLOCK

ownership_graph = pgv.AGraph(directed=True)

with open(sys.argv[1], "rb") as f:
    heap = f.read()

with open(sys.argv[2], "rb") as f:
    rom = f.read()

rom_start = 0x2000

total_byte_len = len(heap)
print("Total heap length:", total_byte_len)
atb_length = total_byte_len * BITS_PER_BYTE // (BITS_PER_BYTE + BITS_PER_BYTE * BLOCKS_PER_ATB // BLOCKS_PER_FTB + BITS_PER_BYTE * BLOCKS_PER_ATB * BYTES_PER_BLOCK)

print("ATB length:", atb_length)
pool_length = atb_length * BLOCKS_PER_ATB * BYTES_PER_BLOCK
print("Total allocatable:", pool_length)

gc_finaliser_table_byte_len = (atb_length * BLOCKS_PER_ATB + BLOCKS_PER_FTB - 1) // BLOCKS_PER_FTB
print("FTB length:", gc_finaliser_table_byte_len)

pool_start = heap_start + total_byte_len - pool_length - pool_shift
pool = heap[-pool_length-pool_shift:]

map_element_blocks = []
string_blocks = []
block_data = {}

longest_free = 0
current_free = 0
current_allocation = 0
total_free = 0
for i in range(atb_length):
    # Each atb byte is four blocks worth of info
    atb = heap[i]
    for j in range(4):
        block_state = (atb >> (j * 2)) & 0x3
        if block_state != AT_FREE and current_free > 0:
            print("{} bytes free".format(current_free * BYTES_PER_BLOCK))
            current_free = 0
        if block_state != AT_TAIL and current_allocation > 0:
            allocation_length = current_allocation * BYTES_PER_BLOCK
            end = (i * BLOCKS_PER_ATB + j) * BYTES_PER_BLOCK
            start = end - allocation_length
            address = pool_start + start
            if True or address == 0x20001a60:
                data = pool[start:end]
                print("0x{:x} {} bytes allocated - {} {}".format(address, allocation_length, binascii.hexlify(data[:4]), data))
                ownership_graph.add_node(address, label="0x{:08x}".format(address), shape="box", height=0.25*current_allocation, style="filled")
                potential_type = None
                node = ownership_graph.get_node(address)
                block_data[address] = data
                for k in range(len(data) // 4):
                    word = struct.unpack_from("<I", data, offset=(k * 4))[0]
                    if word < 0x00040000 and k == 0:
                        potential_type = word
                        if potential_type == dict_type:
                            node.attr["fillcolor"] = "red"
                        elif potential_type == property_type:
                            node.attr["fillcolor"] = "yellow"
                        elif potential_type in function_types:
                            node.attr["fillcolor"] = "green"
                        elif potential_type == map_type:
                            node.attr["fillcolor"] = "blue"
                        elif potential_type == type_type:
                            node.attr["fillcolor"] = "orange"
                        elif potential_type == tuple_type:
                            node.attr["fillcolor"] = "skyblue"
                        elif potential_type == str_type:
                            node.attr["fillcolor"] = "pink"
                        else:
                            print("unknown type", hex(potential_type))

                    if potential_type == str_type and k == 3:
                        string_blocks.append(word)


                    if potential_type == dict_type:
                        if k == 1:
                            node.attr["label"] += "used {}".format(word >> 3)
                        if k == 2:
                            print("alloc size", word)
                        if k == 3:
                            map_element_blocks.append(word)

                    if 0x20000000 < word < 0x20040000:
                        ownership_graph.add_edge(address, word)
                        #print("  0x{:08x}".format(word))


            current_allocation = 0
        if block_state == AT_FREE:
            current_free += 1
            total_free += 1
        elif block_state == AT_HEAD:
            current_allocation = 1
        elif block_state == AT_TAIL:
            current_allocation += 1
        longest_free = max(longest_free, current_free)
if current_free > 0:
    print("{} bytes free".format(current_free * BYTES_PER_BLOCK))

def is_qstr(obj):
    return obj & 0xff800007 == 0x00000006

def find_qstr(qstr_index):
    pool_ptr = last_pool
    if not is_qstr(qstr_index):
        return "object"
    qstr_index >>= 3
    while pool_ptr != 0:
        if pool_ptr in block_data:
            pool = block_data[pool_ptr]
            #print(pool)
            prev, total_prev_len, alloc, length = struct.unpack_from("<IIII", pool)
        else:
            rom_offset = pool_ptr - rom_start
            prev, total_prev_len, alloc, length = struct.unpack_from("<IIII", rom[rom_offset:rom_offset+32])
            pool = rom[rom_offset:rom_offset+length*4]
        #print(hex(prev), total_prev_len, alloc, length)
        #print(qstr_index)
        if qstr_index >= total_prev_len:
            offset = qstr_index - total_prev_len + 32
            start = struct.unpack_from("<I", pool, offset=offset)[0]
            if start < heap_start:
                start -= rom_start
                if start > len(rom):
                    return "more than rom: {:x}".format(start + rom_start)
                qstr_hash, qstr_len = struct.unpack("<BB", rom[start:start+2])
                return rom[start+2:start+2+qstr_len]
            else:
                if start > heap_start + len(heap):
                    return "out of range: {:x}".format(start)
                local = start - heap_start
                qstr_hash, qstr_len = struct.unpack("<BB", heap[local:local+2])
                return heap[local+2:local+2+qstr_len]

        pool_ptr = prev
    return "unknown"

def format(obj):
    if obj & 1 != 0:
        return obj >> 1
    if is_qstr(obj):
        return find_qstr(obj)
    else:
        return "0x{:08x}".format(obj)

#for block in sorted(map_element_blocks):
for block in [0x20001a60]:
    node = ownership_graph.get_node(block)
    node.attr["fillcolor"] = "purple"
    data = block_data[block]
    print("0x{:08x}".format(block))
    for i in range(len(data) // 8):
        key, value = struct.unpack_from("<II", data, offset=(i * 8))
        if key == MP_OBJ_NULL or key == MP_OBJ_SENTINEL:
            print("  <empty slot>")
        else:
            print("  {}, {}".format(format(key), format(value)))

for block in string_blocks:
    node = ownership_graph.get_node(block)
    node.attr["fillcolor"] = "hotpink"
    node.attr["label"] = block_data[block].decode('utf-8')

print("Total free space:", BYTES_PER_BLOCK * total_free)
print("Longest free space:", BYTES_PER_BLOCK * longest_free)

ownership_graph.layout(prog="dot")
ownership_graph.draw("heap.png")
