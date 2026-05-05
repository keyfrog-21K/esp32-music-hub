from fastapi import FastAPI
from fastapi.responses import FileResponse
import subprocess
import os
from PIL import Image
import io

app = FastAPI()

ARTWORK_PATH = os.path.join(os.path.dirname(__file__), "artwork.jpg")
LAST_TRACK = ""

def extract_artwork():
    temp_raw = os.path.join(os.path.dirname(__file__), "raw_artwork")
    script = f'''
    tell application "Music"
        if (count of artworks of current track) > 0 then
            set d to raw data of artwork 1 of current track
            set f to open for access POSIX file "{temp_raw}" with write permission
            set eof f to 0
            write d to f
            close access f
            return "success"
        else
            return "no_artwork"
        end if
    end tell
    '''
    try:
        result = subprocess.check_output(["osascript", "-e", script]).decode("utf-8").strip()
        if result == "success" and os.path.exists(temp_raw):
            with Image.open(temp_raw) as img:
                img = img.convert("RGB")
                # 사이드바 레이아웃을 위해 200x200으로 리사이즈
                img.thumbnail((200, 200))
                img.save(ARTWORK_PATH, "JPEG", quality=85)
            os.remove(temp_raw)
            return True
    except Exception as e:
        print(f"Artwork extraction error: {e}")
        if os.path.exists(temp_raw):
            os.remove(temp_raw)
    return False

def get_now_playing_info():
    global LAST_TRACK
    script = '''
    tell application "System Events"
        set processList to (name of every process)
    end tell
    if processList contains "Music" then
        tell application "Music"
            if player state is playing then
                set trackName to name of current track
                set artistName to artist of current track
                set albumName to album of current track
                return "playing|" & trackName & "|" & artistName & "|" & albumName
            else
                return "stopped|Not Playing||"
            end if
        end tell
    else
        return "stopped|Music app not running||"
    end if
    '''
    try:
        result = subprocess.check_output(["osascript", "-e", script]).decode("utf-8").strip()
        parts = result.split("|")
        if len(parts) >= 4:
            info = {"status": parts[0], "title": parts[1], "artist": parts[2], "album": parts[3]}
            current_track_id = f"{info['title']}-{info['artist']}"
            if info["status"] == "playing" and current_track_id != LAST_TRACK:
                if extract_artwork():
                    info["has_artwork"] = True
                    LAST_TRACK = current_track_id
                else:
                    info["has_artwork"] = False
            else:
                info["has_artwork"] = os.path.exists(ARTWORK_PATH)
            return info
    except Exception as e:
        print(f"Error: {e}")
    return {"status": "stopped", "title": "Error", "artist": "", "album": "", "has_artwork": False}

@app.get("/nowplaying")
async def now_playing():
    return get_now_playing_info()

@app.get("/artwork")
async def get_artwork():
    if os.path.exists(ARTWORK_PATH):
        return FileResponse(ARTWORK_PATH)
    return {"status": "error"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
