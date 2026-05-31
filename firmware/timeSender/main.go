// This scrip was used to update the time on the device over USB before I had a way of setting the time using the buttons.

package main

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"os"
	"strconv"
	"strings"
	"time"

	"go.bug.st/serial"
)

const (
	startByte = 0x5C
	baud      = 115200
)

// CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection, no final XOR)
func crc16CCITTFALSE(data []byte) uint16 {
	var crc uint16 = 0xFFFF
	for _, b := range data {
		crc ^= uint16(b) << 8
		for i := 0; i < 8; i++ {
			if crc&0x8000 != 0 {
				crc = (crc << 1) ^ 0x1021
			} else {
				crc <<= 1
			}
		}
	}
	return crc
}

func choosePort(ports []string) (string, error) {
	switch len(ports) {
	case 0:
		return "", fmt.Errorf("no serial ports found")
	case 1:
		fmt.Printf("Using port: %s\n", ports[0])
		return ports[0], nil
	default:
		fmt.Println("Detected serial ports:")
		for i, p := range ports {
			fmt.Printf("  [%d] %s\n", i+1, p)
		}
		reader := bufio.NewReader(os.Stdin)
		for {
			fmt.Printf("Select a port by number (1-%d) or type a name: ", len(ports))
			input, _ := reader.ReadString('\n')
			input = strings.TrimSpace(input)
			if n, err := strconv.Atoi(input); err == nil {
				if n >= 1 && n <= len(ports) {
					return ports[n-1], nil
				}
			} else if input != "" {
				return input, nil
			}
			fmt.Println("Invalid selection. Try again.")
		}
	}
}

func main() {
	ports, err := serial.GetPortsList()
	if err != nil {
		log.Fatalf("failed to list serial ports: %v", err)
	}
	portName, err := choosePort(ports)
	if err != nil {
		log.Fatal(err)
	}

	mode := &serial.Mode{
		BaudRate:    baud,
		DataBits:    8,
		Parity:      serial.NoParity,
		StopBits:    serial.OneStopBit,
	//	ReadTimeout: 5 * time.Second, // upper bound for waiting on Arduino reply
	}
	port, err := serial.Open(portName, mode)
	err = port.SetReadTimeout(time.Second * 1)
	if err != nil {
		log.Fatalf("failed to open %s: %v", portName, err)
	}
	defer port.Close()

	// port.SetDTR(false)
	// port.SetRTS(false)

	// Wait for ESP32-S3 reboot
	fmt.Println("Waiting for device to initialize...")
	time.Sleep(2000 * time.Millisecond)

	// Drain and print any startup output
	drainSerial(port, 1*time.Second)

	fmt.Println("starting send");

	// build and transmit frame
	now := time.Now()
	frame := []byte{
		startByte,
		byte(now.Hour()),
		byte(now.Minute()),
		byte(now.Second()),
	}
	crc := crc16CCITTFALSE(frame)
	frame = append(frame, byte(crc>>8), byte(crc))

	n, err := port.Write(frame)
	if err != nil {
		log.Fatalf("write error: %v", err)
	}
	if n != len(frame) {
		log.Fatalf("partial write: wrote %d of %d bytes", n, len(frame))
	}
	fmt.Printf("Sent %d-byte frame.\n", n)

	// read Arduino acknowledgement
	reader := bufio.NewReader(port)
	line, err := reader.ReadString('\n') // blocks up to ReadTimeout
	switch {
	case err == io.EOF:
		fmt.Println("No response (read timed out).")
	case err != nil:
		fmt.Printf("Read error: %v\n", err)
	default:
		fmt.Printf("Templog: %s", line)
	}
}

func drainSerial(port serial.Port, duration time.Duration) {
	deadline := time.Now().Add(duration)
	buf := make([]byte, 256)

	for time.Now().Before(deadline) {
		n, err := port.Read(buf)
		if err != nil && err != io.EOF {
			fmt.Printf("Read error during drain: %v\n", err)
			break
		}
		if n > 0 {
			fmt.Print(string(buf[:n]))
		} else {
			time.Sleep(50 * time.Millisecond)
		}
	}
}