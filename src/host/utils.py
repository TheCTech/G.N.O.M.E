import time
from logging import Filter

class Client():
    def __init__(self, id, uuid):
        self.id = id
        self.uuid = uuid
        self.x = 200
        self.y = 200

        self.initial_direction = 90
        self.direction = self.initial_direction
        self.target_direction = self.initial_direction
        
        self.auto_move = False
        self.knock_over = False
        self.armed = True

    def to_dict(self):
        return {
            "id": self.id,
            "uuid": self.uuid,
            "auto_move": self.auto_move,
            "target_direction": self.target_direction,
            "knock_over": self.knock_over,
            "armed": self.armed,
            "x": self.x,
            "y": self.y,
            "initial_direction": self.initial_direction,
            "direction": self.direction
        }

class Target():
    def __init__(self, x, y, priority):
        self.x = x
        self.y = y
        self.priority = priority
        self.created_at = time.time()
    
    def to_dict(self):
        return {
            "x": self.x,
            "y": self.y,
            "priority": self.priority
        }

class LoggingUvicornFilter(Filter):
    def filter(self, record):
        message = record.getMessage()
        if "GET /clients" in message or "GET /targets" in message:
            return False
        return True

def get_local_ip():
    import socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    s.connect(("8.8.8.8", 80))
    ip = s.getsockname()[0]
    s.close()

    return ip

def create_client(clients, id, uuid):
    clients[id] = Client(id, uuid)