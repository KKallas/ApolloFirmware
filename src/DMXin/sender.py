import time
import serial

def send_dmx_packet(cycleTime, lowVal, highVal):
    dmxDataSize = 512
    startCode = 0x00


    # Initialize the serial port
    serial_port = serial.Serial('COM16', baudrate=250000, bytesize=8, parity='N', stopbits=2, timeout=0)

    

    # Send the DMX packet
    try:
        while True:
            # Create the DMX packet data
            dmx_packet = [lowVal] * (dmxDataSize - 1)  # -1 to account for Start Code
            dmx_packet.insert(0, startCode)
            # Start the Break and MAB
            serial_port.write(b'\x00')  # Break
            serial_port.write(b'\x01')  # MAB

            # Send the Start Code and DMX data
            serial_port.write(bytes(dmx_packet))

            # Wait for one DMX cycle
            time.sleep(cycleTime)
            
            # Create the DMX packet data
            dmx_packet = [highVal] * (dmxDataSize - 1)  # -1 to account for Start Code
            dmx_packet.insert(0, startCode)
            # Start the Break and MAB
            serial_port.write(b'\x00')  # Break
            serial_port.write(b'\x01')  # MAB

            # Send the Start Code and DMX data
            serial_port.write(bytes(dmx_packet))

            # Wait for one DMX cycle
            time.sleep(cycleTime)

    except KeyboardInterrupt:
        # Stop the DMX transmission on keyboard interrupt (Ctrl+C)
        serial_port.close()

if __name__ == "__main__":
    cycleTime = 1  # Time for one DMX cycle in seconds (4 µs per bit)
    lowVal = 0x00  # Low value (0x00) for the DMX data frame
    highVal = 0xFF  # High value (0xFF) for the DMX data frame

    send_dmx_packet(cycleTime, lowVal, highVal)
