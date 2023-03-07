#!/usr/bin/python3
# -*- coding: utf-8 -*-

import socket
import sys

class SocketClient:
    def __init__(self):
      pass

    def connect_to_server(self, cmd):
      # 常规tcp连接写法
      # server_address = ('127.0.0.1', 9999)
      # socket_family = socket.AF_INET
      # socket_type = socket.SOCK_STREAM

      # unix domain sockets 连接写法
      server_address = '/data/mysql/bfs.sock'
      socket_family = socket.AF_UNIX
      socket_type = socket.SOCK_STREAM

      # 其他代码完全一样
      sock = socket.socket(socket_family, socket_type)
      sock.connect(server_address)
      sock.sendall(cmd.encode())
      print(f"send requuest to server '{server_address}': {cmd}")
      data = sock.recv(1024)
      print(f"recv responce from server '{server_address}': {data.decode()}")
      sock.close()

if __name__ == "__main__":
    socket_client_obj = SocketClient()
    if len(sys.argv) != 2:
      print("usage: python3", sys.argv[0], "stub_name")
      sys.exit()
    cmd = sys.argv[1]
    socket_client_obj.connect_to_server(cmd)
    # socket_client_obj.connect_to_server("kill_now")