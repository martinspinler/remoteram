import os
import sys
import grpc
import importlib

import cocotbext.ofm.utils

try:
    import scapy.utils
    use_scapy = True
except:
    use_scapy = False

class __add_path():
    def __init__(self, path):
        self.path = path

    def __enter__(self):
        sys.path.insert(0, self.path)

    def __exit__(self, exc_type, exc_value, traceback):
        try:
            sys.path.remove(self.path)
        except ValueError:
            pass

from .protobuf import remoteram_pb2_grpc
from .protobuf import remoteram_pb2


class RAM(cocotbext.ofm.utils.RAM):
    def __init__(self, addr):
        self.addr = addr
        self.stub = None
        self._verbosity = 0

    def _connect(self, addr):
        self.channel = grpc.insecure_channel(addr)
        self.stub = remoteram_pb2_grpc.RemoteRamStub(self.channel)
        if self._verbosity:
            print("GRP RAM connected:", self.stub)

    #def w(self, addr, integer, val):
    #    self.wr(addr, list(integer.to_bytes(val, byteorder = 'little')))

    def w(self, addr, byte):
        if not self.stub: self._connect(self.addr)

        if self._verbosity:
            print("GRPC RAM write req:", hex(addr), list(byte))
            if self._verbosity > 1:
                scapy.utils.hexdump(list(byte)) if use_scapy else print(list(byte))
            sys.stdout.flush()
        r = remoteram_pb2.ram_req(type=0, addr=addr, data = bytes(byte), nbyte = len(byte))
        try:
            self.stub.Ram_req(r)
        except Exception as e:
            print(e)
        #print("Wr req done", addr)


    def r(self, addr, byte_count):
        if not self.stub: self._connect(self.addr)

        if self._verbosity:
            print("GRPC RAM read req:", hex(addr), byte_count)
            sys.stdout.flush()
        #return [0] * byte_count

        r = remoteram_pb2.ram_req(type=1, addr=addr, nbyte = byte_count, data=bytes([]))
        try:
            resp = self.stub.Ram_req(r)
        except Exception as e:
            print(e)
            sys.stdout.flush()
            return [0] * byte_count
        else:
            #print(" ".join([hex(x) for x in list(bytes(resp.data))]))
            #print("RD req done", addr, byte_count)
            if self._verbosity:
                print("GRPC RAM read resp:", hex(addr))
                if self._verbosity > 1:
                    scapy.utils.hexdump(list(bytes(resp.data))) if use_scapy else print(list(bytes(resp.data)))
                sys.stdout.flush()

            return list(bytes(resp.data))
