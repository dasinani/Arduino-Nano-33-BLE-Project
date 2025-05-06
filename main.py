import asyncio
from bleak import BleakScanner, BleakClient

# UUIDs for proximity, gesture, gyroscope, accelerometer, and microphone characteristics
PROXIMITY_SERVICE_UUID = "00000000-5EC4-4083-81CD-A10B8D5CF6EC"
PROXIMITY_CHARACTERISTIC_UUID = "00000001-5EC4-4083-81CD-A10B8D5CF6EC"
GESTURE_CHARACTERISTIC_UUID = "00000002-5EC4-4083-81CD-A10B8D5CF6EC"
GYRO_CHARACTERISTIC_UUID = "00000003-5EC4-4083-81CD-A10B8D5CF6EC"
ACCEL_CHARACTERISTIC_UUID = "00000004-5EC4-4083-81CD-A10B8D5CF6EC"
DEVICE_NAME = "ProximitySensor"  # BLE device name as set in the Arduino sketch

async def connect_to_device():
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover()

    target_device = None
    for device in devices:
        print(f"Found device: {device.name}, Address: {device.address}")
        if device.name == DEVICE_NAME:
            target_device = device
            break

    if not target_device:
        print(f"Device '{DEVICE_NAME}' not found. Make sure it's advertising and try again.")
        return

    print(f"Connecting to {DEVICE_NAME} at address {target_device.address}...")
    async with BleakClient(target_device.address) as client:
        print(f"Connected to {DEVICE_NAME}!")

        print("Discovering services and characteristics...")
        for service in client.services:
            print(f"Service: {service.uuid}")
            if service.uuid.lower() == PROXIMITY_SERVICE_UUID.lower():
                for char in service.characteristics:
                    print(f"  Characteristic: {char.uuid} - Properties: {char.properties}")

                    # Subscribe to proximity data
                    if char.uuid.lower() == PROXIMITY_CHARACTERISTIC_UUID.lower():
                        print("Subscribing to proximity notifications...")
                        def handle_proximity_data(sender, data):
                            print(f"Proximity: {data.decode('utf-8')}")
                        await client.start_notify(char.uuid, handle_proximity_data)

                    # Subscribe to gesture data
                    elif char.uuid.lower() == GESTURE_CHARACTERISTIC_UUID.lower():
                        print("Subscribing to gesture notifications...")
                        def handle_gesture_data(sender, data):
                            print(f"Gesture: {data.decode('utf-8')}")
                        await client.start_notify(char.uuid, handle_gesture_data)

                    # Subscribe to gyroscope data
                    elif char.uuid.lower() == GYRO_CHARACTERISTIC_UUID.lower():
                        print("Subscribing to gyroscope notifications...")
                        def handle_gyro_data(sender, data):
                            print(f"Gyroscope: {data.decode('utf-8')}")
                        await client.start_notify(char.uuid, handle_gyro_data)

                    # Subscribe to accelerometer data
                    elif char.uuid.lower() == ACCEL_CHARACTERISTIC_UUID.lower():
                        print("Subscribing to accelerometer notifications...")
                        def handle_accel_data(sender, data):
                            print(f"Accelerometer: {data.decode('utf-8')}")
                        await client.start_notify(char.uuid, handle_accel_data)


        print("Receiving data... Press Ctrl+C to exit.")
        try:
            while True:
                await asyncio.sleep(1)
        except KeyboardInterrupt:
            print("\nStopping notifications...")
            await client.stop_notify(PROXIMITY_CHARACTERISTIC_UUID)
            await client.stop_notify(GESTURE_CHARACTERISTIC_UUID)
            await client.stop_notify(GYRO_CHARACTERISTIC_UUID)
            await client.stop_notify(ACCEL_CHARACTERISTIC_UUID)
            print("Disconnected.")

if __name__ == "__main__":
    asyncio.run(connect_to_device())