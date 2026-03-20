from fastapi import FastAPI, Query, Body, status
from fastapi.responses import FileResponse, PlainTextResponse, JSONResponse
from config import MAX_CLIENTS
from utils import get_local_ip, create_client, Client

app = FastAPI(title="G.N.O.M.E")

clients: dict[int, Client] = {} # id:Client

map_dimensions = [800, 600]

@app.get("/")
def root():
    return {"status": "Backend running"}

@app.get("/panel")
def panel():
    return FileResponse("webpage/index.html")

@app.get("/style.css")
def return_style():
    return FileResponse("webpage/style.css")

@app.get("/script.js")
def return_script():
    return FileResponse("webpage/script.js")

@app.get("/register")
def handle_register(uuid: str = Query(None), x: str = Query(None), y: str = Query(None)):
    if not uuid:
        return PlainTextResponse("Missing uuid", status.HTTP_422_UNPROCESSABLE_ENTITY)
    
    if not x or not y:
        return PlainTextResponse("Missing x or y", status.HTTP_422_UNPROCESSABLE_ENTITY)
    
    print(f"Client with uuid: {uuid} trying to connect")

    for client_id in clients.keys():
        if uuid == clients[client_id].uuid:
            print(f"Reconnected as C{client_id}")
            clients[client_id].x = x
            clients[client_id].y = y
            return PlainTextResponse(f"Reconnected as C{client_id}", status.HTTP_200_OK)
        
    for client_id in range(MAX_CLIENTS):
        try:
            if clients[client_id] == "":
                break
        except KeyError:
            break
    else:
        print("Rejected client: server full")
        return PlainTextResponse("Too many clients", status.HTTP_429_TOO_MANY_REQUESTS)

    print(f"Connected as C{client_id}")
    create_client(clients, client_id, uuid, x, y)
    return PlainTextResponse(f"Connected as C{client_id}", status.HTTP_200_OK)

@app.get("/clients")
def handle_clients_date_request():
    return JSONResponse(
        [{
            "id":client.id,
            "uuid":client.uuid,
            "auto_move":client.auto_move,
            "move_queue":client.move_queue,
            "knock_over": client.knock_over,
            "armed": client.armed,
            "x": client.x,
            "y": client.y
            } for client in clients.values()],
        status.HTTP_200_OK
    )

@app.put("/setMap")
def set_map(data: dict = Body()):
    global map_dimensions

    if "width" in data:
        map_dimensions[0] = int(data["width"])
    if "height" in data:
        map_dimensions[1] = int(data["height"])
    
    if map_dimensions[0] <= 0 or map_dimensions[1] <= 0:
        map_dimensions = [800, 600]

        print(f"Error while setting map size, returning to 800x600")
        return PlainTextResponse(f"Error while setting map size, returning to 800x600", status.HTTP_422_UNPROCESSABLE_ENTITY)
    
    print(f"Map resized to {map_dimensions[0]} x {map_dimensions[1]}")
    return PlainTextResponse(f"Map resized to {map_dimensions[0]} x {map_dimensions[1]}", status.HTTP_200_OK)

@app.get("/map")
def return_map_dimensions():
    return JSONResponse({"width":map_dimensions[0], "height":map_dimensions[1]}, status.HTTP_200_OK)

@app.put("/setUser")
def set_user_variable(data: dict = Body()):
    if not "variable" in data or not "id" in data or not "value" in data:
        return PlainTextResponse("Missing variable, id or value", status.HTTP_422_UNPROCESSABLE_ENTITY)
    
    try:
        value = data["value"]

        if data.get("addition", False):
            value += getattr(clients[data["id"]], data["variable"])

        setattr(clients[data["id"]], data["variable"], value)

        ### TODO: remove ###
        if getattr(clients[data["id"]], "knock_over") == True:
            setattr(clients[data["id"]], "knock_over", False)
            setattr(clients[data["id"]], "armed", False)
            print("PLaceholder logic knocked over gnome")

        return PlainTextResponse(f"Variable set successfully", status.HTTP_200_OK)
    except:
        return PlainTextResponse("Error assigning value, maybe variable name is wrong?", status.HTTP_422_UNPROCESSABLE_ENTITY)

if __name__ == "__main__":
    print(f"IP: {get_local_ip()}")
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)