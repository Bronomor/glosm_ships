import asyncio
import websockets
import json
from datetime import datetime, timezone
import sys
from threading import Thread, Event
import time

event = Event()


def create_ship_obj(message):
    ship_details = dict()
    ship_details['Latitude']  = message['Latitude']
    ship_details['Longitude'] = message['Longitude']
    ship_details['Cog'] = message['Cog']
    ship_details['Sog'] = message['Sog']

    return ship_details

def write_json_to_file(file_name):
    pass

async def connect_ais_stream(lower_lat, lower_lon, upper_lat, upper_lon):

    ship_positions = dict()

    async with websockets.connect("wss://stream.aisstream.io/v0/stream") as websocket:
        subscribe_message = {"APIKey": "30ae44f39186f22590ebef9aaff9434abc2d50d9", "BoundingBoxes": [[[lower_lat, lower_lon], [upper_lat, upper_lon]]]}

        subscribe_message_json = json.dumps(subscribe_message)
        await websocket.send(subscribe_message_json)

        async for message_json in websocket:

            if event.is_set():
                break;

            message = json.loads(message_json)
            message_type = message["MessageType"]

            print(message_type)

            if message_type == "PositionReport":
                ais_message = message['Message']['PositionReport']
                ship_positions[ais_message['UserID']] = create_ship_obj(ais_message)

            with open('../../testdata/positions.json', 'w') as fp:
                json.dump(ship_positions, fp, indent=4) 


def task(mLower_lat, mLower_lon, mUpper_lat, mUpper_lon ):
    asyncio.run(asyncio.run(connect_ais_stream(mLower_lat, mLower_lon, mUpper_lat, mUpper_lon)))

if __name__ == "__main__":
    mLower_lat = float(sys.argv[1])
    mLower_lon = float(sys.argv[2])
    mUpper_lat = float(sys.argv[3])
    mUpper_lon = float(sys.argv[4])

    thread = Thread(target=task, args=(mLower_lat, mLower_lon, mUpper_lat, mUpper_lon), daemon=True)
    thread.start()

    time.sleep(60)
    event.set()


# [[[64, 11], [65, 12]]]
# [[[64, 11], [64.5, 11.5]]]
# [[[64, 11], [65, 12]]]

# duzo statkow [[[64.52, 11.2], [65, 11.68]]]