class Client():
    def __init__(self, id, uuid, x, y):
        self.id = id
        self.uuid = uuid
        self.x = x
        self.y = y

        self.auto_move = False
        self.move_queue = 0
        self.knock_over = False
        self.armed = True

def get_local_ip():
    import socket
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    s.connect(("8.8.8.8", 80))
    ip = s.getsockname()[0]
    s.close()

    return ip

def create_client(clients, id, uuid, x, y):
    clients[id] = Client(id, uuid, x, y)